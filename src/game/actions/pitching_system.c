#include "actions/pitching_system.h"
#include "rules_pure/rules_strikes.h"
#include "common_logic.h"
#include "vector_math.h"
#include "base_logic.h"
#include "base_control.h"

#define ANIMATION_FREQUENCY 3

void reset_pitching_system(MatchSession* match)
{
    match->pendingActionState.catchingAction = (CatchingAction){0};
}

// Can a windup legally begin right now? (Was the precondition block of the old start_pitch.)
//   i)   the pitcher (home catcher) holds the ball
//   ii)  no pitch already in flight
//   iii) the batter is ready
//   iv)  no throw is going on
//   v)   the pitcher is close enough to the pitching plate
//   vi)  no free-walk decision is pending
//   vii) §18(1): the batter has not already spent his three correct pitches. A batter who has is
//        permanently a runner — "ei ole enää lyöntivuorossa" — so there is nobody to pitch to. Stated
//        here rather than left to the side effect of the forced run, which held only for period < 4.
static int can_begin_windup(const MatchSession* match, int correct_pitches_received)
{
    return !batter_has_become_runner_permanently(correct_pitches_received) &&
           match->pII.hasBallIndex == match->pII.catcherOnBaseIndex[0] && match->pRAI.pitch_state == PITCH_STAGE_NONE &&
           match->pRAI.batter_ready == 1 && match->pendingActionState.catchingAction.kind != CATCHING_ACTION_THROWING &&
           match->playerInfo[match->pII.catcherOnBaseIndex[0]].cTPI.isNearHomeLocation == 1 &&
           match->flowControl.waitingForFreeWalkDecision == 0;
}

int pitch_is_being_delivered(const MatchSession* match)
{
    return match->pendingActionState.catchingAction.kind == CATCHING_ACTION_PITCHING ||
           match->pRAI.pitch_state == PITCH_STAGE_AIRBORNE;
}

// Begin the engine-owned windup for a toss of `power`: set up the pitcher and the ball, start the
// deterministic clock. This is pure timing setup — the animation model is set downstream for the
// renderer, but nothing here (or later) reads animation to drive timing.
static void begin_windup(MatchSession* match, float power)
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
    match->pendingActionState.catchingAction =
        (CatchingAction){.kind = CATCHING_ACTION_PITCHING, .timer = 0, .as.pitch = {.power = power}};
}

// Release the real pitch — the outcome is a pure function of the DECLARED values (never a live meter):
// the ball leaves with velocity pitch_velocity_from_aim(power, direction).
static void release_pitch(MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions)
{
    int pitcher = match->pII.hasBallIndex;
    const PitchWindup* pitch = &match->pendingActionState.catchingAction.as.pitch;
    Vector3D v = pitch_velocity_from_aim(pitch->power, pitch->direction);
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

    match->pendingActionState.catchingAction = (CatchingAction){0};
}

// Valesyöttö (a fake): the windup ran out and nobody aimed it. No real pitch — the pitcher keeps the
// ball, which returns to their position; no ball/strike is counted.
static void fake_pitch(MatchSession* match)
{
    int pitcher = match->pII.hasBallIndex;

    match->pRAI.pitch_state = PITCH_STAGE_NONE;
    if (pitcher != -1) {
        match->ballInfo.location.x = match->playerInfo[pitcher].tPI.location.x;
        match->ballInfo.location.z = match->playerInfo[pitcher].tPI.location.z;
        match->playerInfo[pitcher].cPI.model = PLAYER_ANIM_STAND_WITH_BALL;
    }
    match->pendingActionState.catchingAction = (CatchingAction){0};
}

// The pitch actualizer — the single engine entry point. A declared power begins the windup if it can
// legally begin; an aim, whenever it arrives, is held on the windup until release; and the engine's clock
// decides the rest: aimed when the windup ends → a real pitch, unaimed → valesyöttö. Timing is the master;
// the animation only follows.
void update_pitch_actualization(
    MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions,
    int correct_pitches_received, const PitchValues* declared
)
{
    CatchingAction* action = &match->pendingActionState.catchingAction;

    if (declared->powerDeclared && action->kind == CATCHING_ACTION_NONE) {
        if (!can_begin_windup(match, correct_pitches_received)) return;
        begin_windup(match, declared->power);
        if (declared->aimDeclared) {
            action->as.pitch.aimed = 1;
            action->as.pitch.direction = declared->direction;
        }
        return; // never begin and resolve in the same frame — the windup is at least one tick
    }

    // The windup is tracked by the catching action, NOT by pitch_state: pitch_state is a broad "ball in
    // flight" signal other stages reset (consolidation zeroes it while a stale pitchResult lingers), so
    // gating on it would kill a fresh windup before it could release.
    if (action->kind == CATCHING_ACTION_PITCHING) {
        if (declared->aimDeclared) {
            action->as.pitch.aimed = 1;
            action->as.pitch.direction = declared->direction;
        }
        action->timer++;
        if (action->timer >= pitch_windup_total_frames(action->as.pitch.power)) {
            if (action->as.pitch.aimed) {
                release_pitch(match, referee, fieldPositions);
            } else {
                fake_pitch(match);
            }
        }
    }
}
