#include "actions/pitching_system.h"
#include "common_logic.h"
#include "vector_math.h"
#include "base_logic.h"
#include "base_control.h"

#define ANIMATION_FREQUENCY 3

// Total windup frames for a declared toss power: a fixed crouch + a power-scaled crouch-hold + a fixed
// rise. Higher pitches hold the crouch longer (≈2s low → 3s high). See the header constants.
int pitch_windup_total_frames(float power)
{
    if (power < 0.0f) power = 0.0f;
    if (power > 1.0f) power = 1.0f;
    return PITCH_WINDUP_DOWN_FRAMES + (int)(power * PITCH_WINDUP_HOLD_MAX) + PITCH_WINDUP_UP_FRAMES;
}

void reset_pitching_system(MatchSession* match)
{
    match->pendingActionState.pitchActualization.timer = 0;
    match->aF.cTAF.pitch.phase = PITCH_DECL_IDLE;
    match->aF.cTAF.pitch.power = 0.0f;
    match->aF.cTAF.pitch.direction = 0.0f;
    match->aiState.batterReadyTimer = -1;
}

// Can a windup legally begin right now? (Was the precondition block of the old start_pitch.)
//   i)   the pitcher (home catcher) holds the ball
//   ii)  no pitch already in flight
//   iii) the batter is ready
//   iv)  no throw is going on (a pitch may be cancelled for a throw, never the reverse)
//   v)   the pitcher is close enough to the pitching plate
//   vi)  no free-walk decision is pending
static int can_begin_windup(const MatchSession* match)
{
    return match->pII.hasBallIndex == match->pII.catcherOnBaseIndex[0] && match->pRAI.pitch_state == PITCH_STAGE_NONE &&
           match->pRAI.batter_ready == 1 && match->pendingActionState.throw_going_on == 0 &&
           match->playerInfo[match->pII.catcherOnBaseIndex[0]].cTPI.isNearHomeLocation == 1 &&
           match->flowControl.waitingForFreeWalkDecision == 0;
}

// Begin the engine-owned windup: set up the pitcher and the ball, start the deterministic clock. This is
// pure timing setup — the animation model is set downstream for the renderer, but nothing here (or later)
// reads animation to drive timing.
static void begin_windup(MatchSession* match)
{
    int pitcher = match->pII.hasBallIndex;

    if (match->playerInfo[pitcher].cPI.moving == 1) {
        stop_movement(match->playerInfo, pitcher);
    }
    // crouch animation (renderer-facing; downstream of the clock, never an input to it)
    match->playerInfo[pitcher].cPI.model = PLAYER_ANIM_PITCH_WINDUP;
    match->playerInfo[pitcher].cPI.animationStage = 0;
    match->playerInfo[pitcher].cPI.animationStageCount = PITCH_DOWN_MAX;
    match->playerInfo[pitcher].cPI.animationFrequency = ANIMATION_FREQUENCY;
    // force the pitcher exactly onto the pitching spot (a windup may begin from slightly off)
    match->playerInfo[pitcher].tPI.location.x = match->playerInfo[pitcher].tPI.homeLocation.x;
    match->playerInfo[pitcher].tPI.location.z = match->playerInfo[pitcher].tPI.homeLocation.z;
    match->playerInfo[pitcher].cPI.lastLastLocationUpdate = 1;
    // face the plate
    match->playerInfo[pitcher].tPI.orientation.x = -1.0f;
    match->playerInfo[pitcher].tPI.orientation.z = 0.0f;
    // the ball rises from the centre of the plate
    vec3_set_xz(&(match->ballInfo.location), 0.0f, 0.0f);

    match->pRAI.pitch_state = PITCH_STAGE_WINDUP;
    match->pendingActionState.current_catching_action = CATCHING_ACTION_PITCHING;
    match->pendingActionState.pitchActualization.timer = 0;
}

// Release the real pitch — the outcome is a pure function of the DECLARED aim (never a live meter): the
// ball leaves with velocity pitch_velocity_from_aim(power, direction). Preserves every legacy
// release_pitch side effect (pitchReleased event, runner commits, batter_can_advance, ...).
static void release_pitch(MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions)
{
    int pitcher = match->pII.hasBallIndex;
    PitchDeclaration* decl = &match->aF.cTAF.pitch;
    Vector3D v = pitch_velocity_from_aim(decl->power, decl->direction);
    Vector3D target;
    int i;

    match->ballInfo.visible = 1;
    match->ballInfo.moving = 1;
    match->ballInfo.currentFlightHasHitGround = 0;
    match->ballInfo.onGround = 0;
    match->ballInfo.hitsGroundToUnWound = 0;
    vec3_set_xyz(&(match->ballInfo.velocity), v.x, v.y, v.z);

    // nudge the pitcher aside as the ball leaves
    target.x = match->playerInfo[pitcher].tPI.location.x + PITCHER_MOVE_AWAY_OFFSET;
    target.z = match->playerInfo[pitcher].tPI.location.z;
    move_to_target(match->playerInfo, pitcher, &target);

    match->pII.lastHadBallIndex = pitcher; // pitcher won't catch its own pitch before it grounds
    match->pII.hasBallIndex = -1;
    match->pRAI.pitch_state = PITCH_STAGE_AIRBORNE;
    match->pendingActionState.run_bat_flag = 0;
    match->pRAI.batter_can_advance = 1;
    match->aiState.aiWrongPitch = 0;
    match->cameraState.homeRunCameraFlag = 0;
    match->gameEvents.pitchReleased = 1;
    // release animation (downstream): the windup arc (render-side, driven by the windup clock) has already
    // played the full down→up motion and lands on the final pitch_up frame exactly now — so at release we
    // just HOLD that follow-through pose (animationStage pinned to the last frame; update_models holds a
    // non-loop at its end) until movement takes over. This replaces the old bug where the up-swing only
    // fired here, after the ball had already left.
    match->playerInfo[pitcher].cPI.model = PLAYER_ANIM_PITCH_THROW;
    match->playerInfo[pitcher].cPI.animationFrequency = ANIMATION_FREQUENCY;
    match->playerInfo[pitcher].cPI.animationStageCount = PITCH_UP_MAX;
    match->playerInfo[pitcher].cPI.animationStage = PITCH_UP_MAX * ANIMATION_FREQUENCY - 1;

    // commit any base runners that armed to break on the pitch
    for (i = 1; i < BASE_COUNT; i++) {
        if (match->pRAI.will_start_running[i] == 1) {
            int index = get_base_controller(match, referee, (BaseID)i);
            match->pRAI.will_start_running[i] = 0;
            if (index != -1) {
                run_to_next_base(match, fieldPositions, index, (BaseID)i);
            }
        }
    }

    // resolution: clear the declaration (consumer-clears) and the dance state
    match->pendingActionState.current_catching_action = CATCHING_ACTION_NONE;
    match->pendingActionState.pitchActualization.timer = 0;
    decl->phase = PITCH_DECL_IDLE;
    decl->power = 0.0f;
    decl->direction = 0.0f;
}

// Valesyöttö (a fake): the pitcher declined the aim. No real pitch — the pitcher keeps the ball, which
// returns to their position; no ball/strike is counted. (Minimal first definition; committed-runner
// exposure refined in the graphical pass.)
static void fake_pitch(MatchSession* match)
{
    int pitcher = match->pII.hasBallIndex;
    PitchDeclaration* decl = &match->aF.cTAF.pitch;

    match->pRAI.pitch_state = PITCH_STAGE_NONE;
    match->pendingActionState.current_catching_action = CATCHING_ACTION_NONE;
    if (pitcher != -1) {
        match->ballInfo.location.x = match->playerInfo[pitcher].tPI.location.x;
        match->ballInfo.location.z = match->playerInfo[pitcher].tPI.location.z;
        match->playerInfo[pitcher].cPI.model = PLAYER_ANIM_STAND_WITH_BALL;
    }
    match->pendingActionState.pitchActualization.timer = 0;
    decl->phase = PITCH_DECL_IDLE;
    decl->power = 0.0f;
    decl->direction = 0.0f;
}

// The pitch actualizer — the single engine entry point (replaces start_pitch / continue_pitch /
// release_pitch / update_pitching_meter). Reads the producer's phased declaration, runs the deterministic
// windup clock, and resolves: AIMED at the windup end → real pitch; FAKE (or never aimed in time) →
// valesyöttö. Timing is the master; the animation only follows.
void update_pitch_actualization(MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions)
{
    PitchDeclaration* decl = &match->aF.cTAF.pitch;
    PendingActionState* pas = &match->pendingActionState;

    // Begin a windup the first frame a producer has declared anything (phase left IDLE) while no catching
    // action is in progress — if it is legal; otherwise drop the declaration.
    if (decl->phase != PITCH_DECL_IDLE && pas->current_catching_action == CATCHING_ACTION_NONE) {
        if (can_begin_windup(match)) {
            begin_windup(match);
        } else {
            decl->phase = PITCH_DECL_IDLE;
            decl->power = 0.0f;
            decl->direction = 0.0f;
        }
        return; // never begin and resolve in the same frame — the windup is at least one tick
    }

    // The windup is tracked by the engine mutex (current_catching_action), NOT by pitch_state: pitch_state
    // is a broad "ball in flight" signal other stages reset (e.g. consolidation zeroes it while a stale
    // pitchResult lingers), so gating on it would kill a fresh windup before it could release. cca is
    // engine-owned and untouched by them.
    if (pas->current_catching_action == CATCHING_ACTION_PITCHING) {
        pas->pitchActualization.timer++;

        if (decl->phase == PITCH_DECL_FAKE) {
            fake_pitch(match); // explicit early fake (strategic — e.g. against leading runners)
        } else if (pas->pitchActualization.timer >= pitch_windup_total_frames(decl->power)) {
            if (decl->phase == PITCH_DECL_AIMED) {
                release_pitch(match, referee, fieldPositions); // committed aim → real pitch
            } else {
                fake_pitch(match); // windup elapsed without an aim → valesyöttö
            }
        }
        // else: still winding up (WINDUP/POWER) — waiting for the producer to aim.
    }
}
