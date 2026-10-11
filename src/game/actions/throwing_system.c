#include "throwing_system.h"
#include "execute_actions.h"
#include "common_logic.h"
#include "vector_math.h"
#include <math.h>

#define THROW_POWER_CONSTANT 0.65f
#define THROW_DISTANCE_CONSTANT 0.0012f

#define DROP_BALL_CONSTANT 0.02f

#define THROW_ANIMATION_FREQUENCY 3

// Windup sizing: how long the physical windup for a declared power lasts. Once the power is known the engine
// releases when its clock reaches this — for BOTH producers. A human's clock starts earlier (with the
// target, running while it picks power on the client charge widget), so by the time its power arrives the
// windup has usually elapsed → immediate release; the client never reads this function or the clock.
int throw_windup_total_frames(float power)
{
    if (power < THROW_POWER_MIN) power = THROW_POWER_MIN;
    if (power > 1.0f) power = 1.0f;
    float t = (power - THROW_POWER_MIN) / (1.0f - THROW_POWER_MIN);
    return THROW_WINDUP_MIN_FRAMES + (int)(t * (THROW_WINDUP_MAX_FRAMES - THROW_WINDUP_MIN_FRAMES));
}

Vector3D throw_target_point(const FieldPositions* fieldPositions, BaseID base)
{
    switch (base) {
    case BASE_FIRST:
        return fieldPositions->firstBase;
    case BASE_SECOND:
        return fieldPositions->secondBase;
    case BASE_THIRD:
        return fieldPositions->thirdBase;
    case BASE_HOME:
    default:
        return fieldPositions->pitcher;
    }
}

// Begin the engine-owned windup for a throw toward `base`: latch the line to the target, stop and orient
// the thrower, and start the deterministic clock. Returns 1 if the windup began, 0 if it could not (no ball,
// or the thrower already stands on the target base — too close to throw). The gather animation follows
// the clock downstream.
static int begin_throw_windup(MatchSession* match, const FieldPositions* fieldPositions, BaseID base)
{
    if (match->pII.hasBallIndex == -1) {
        return 0;
    }
    if (base < BASE_HOME || base >= BASE_COUNT) return 0;

    int thrower = match->pII.hasBallIndex;
    Vector3D target = throw_target_point(fieldPositions, base);
    // which base the throw is going to keeps basemen from wandering out to catch it
    match->pRAI.throw_going_to_base = (int)base;
    ThrowWindup windup = {.target = base};
    windup.direction.x = target.x - match->playerInfo[thrower].tPI.location.x;
    windup.direction.z = target.z - match->playerInfo[thrower].tPI.location.z;
    windup.distance = (float)sqrt(windup.direction.x * windup.direction.x + windup.direction.z * windup.direction.z);
    // Can't throw to a base you are already standing on.
    if (windup.distance <= THROW_TO_BASE_DISTANCE) {
        match->pRAI.throw_going_to_base = -1;
        return 0;
    }

    // Stop the thrower if moving — the gather animation has no foot movement.
    if (match->playerInfo[thrower].cPI.moving == 1) {
        stop_movement(match->playerInfo, thrower);
    }
    // Gather animation (renderer-facing; downstream of the clock — the render arc maps the windup timer
    // across the gather frames, so animationStage here is just the initial pose, not the driver).
    match->playerInfo[thrower].cPI.model = PLAYER_ANIM_THROW_WINDUP;
    match->playerInfo[thrower].cPI.animationStage = 0;
    match->playerInfo[thrower].cPI.animationStageCount = THROW_LOAD_FRAMES;
    match->playerInfo[thrower].cPI.animationFrequency = THROW_ANIMATION_FREQUENCY;
    // Avoid twitching if a move key is still down while the thrower can't move.
    match->playerInfo[thrower].cPI.lastLastLocationUpdate = 1;
    // Face the target base.
    match->playerInfo[thrower].tPI.orientation.x = windup.direction.x;
    match->playerInfo[thrower].tPI.orientation.z = windup.direction.z;

    match->pendingActionState.catchingAction =
        (CatchingAction){.kind = CATCHING_ACTION_THROWING, .timer = 0, .as.throw = windup};
    return 1;
}

// Launch the ball toward the latched base with the DECLARED power — a trusted value (the AI's strategy, or
// the human's charge-widget sample), never a live meter or the engine clock. The action resolves here: the
// record is zeroed, which is the engine's "this one is done".
static void throw_release(MatchSession* match)
{
    ThrowWindup windup = match->pendingActionState.catchingAction.as.throw;
    match->pendingActionState.catchingAction = (CatchingAction){0};
    if (match->pII.hasBallIndex == -1) {
        return;
    }
    float power = windup.power;
    if (power < 0.0f) power = 0.0f;
    if (power > 1.0f) power = 1.0f;

    // release animation
    match->playerInfo[match->pII.hasBallIndex].cPI.model = PLAYER_ANIM_THROW_RELEASE;
    match->playerInfo[match->pII.hasBallIndex].cPI.animationStage = 0;
    match->playerInfo[match->pII.hasBallIndex].cPI.animationStageCount = 21;
    match->playerInfo[match->pII.hasBallIndex].cPI.animationFrequency = 2;
    // flag that the recoil animation is still going (no extra movement until it is over)
    match->playerInfo[match->pII.hasBallIndex].cTPI.throwRecoil = 1;

    // normalize the latched direction and give it a small upward component
    float dx = windup.direction.x / windup.distance;
    float dz = windup.direction.z / windup.distance;
    float dy = 0.06f;
    generic_sling_ball(
        &(match->ballInfo), dx * power * THROW_POWER_CONSTANT, dy + windup.distance * THROW_DISTANCE_CONSTANT,
        dz * power * THROW_POWER_CONSTANT
    );
    // Trigger fielder selection update after throw
    match->pRAI.refresh_catch_and_change = 1;
    match->pRAI.init_player_selection = 1;
    // prevent this player from catching the ball right after throwing
    match->pII.lastHadBallIndex = match->pII.hasBallIndex;
    match->pII.hasBallIndex = -1;
    // set running flag to 0 so that orientation will change
    match->playerInfo[match->pII.controlIndex].cPI.running = 0;
    // set control to -1 (the change key is the same as the throw key) — let generic_sling_ball handle
    // player changing.
    match->pII.controlIndex = -1;
}

// The throw actualizer — the single engine entry point, the throw's counterpart of the pitch's. A declared
// target begins the gather; a declared power, whenever it arrives, is held on it; and the ball leaves once
// the power is known and the windup for that power has elapsed. ONE rule for both producers: the AI
// declares both values at once, so its clock starts with the power known; a human declares the target
// first, so the gather runs WHILE it picks power on its own meter and the release usually follows the
// moment the power arrives. The client never commands the release instant; the engine owns it.
void update_throw_actualization(MatchSession* match, const FieldPositions* fieldPositions, const ThrowValues* declared)
{
    CatchingAction* action = &match->pendingActionState.catchingAction;

    if (declared->targetDeclared && action->kind == CATCHING_ACTION_NONE) {
        if (!begin_throw_windup(match, fieldPositions, declared->target)) return;
        if (declared->powerDeclared) {
            action->as.throw.powered = 1;
            action->as.throw.power = declared->power;
        }
        return; // never begin and release in the same frame — the windup is at least one tick
    }

    if (action->kind == CATCHING_ACTION_THROWING) {
        if (declared->powerDeclared) {
            action->as.throw.powered = 1;
            action->as.throw.power = declared->power;
        }
        action->timer++;
        if (action->as.throw.powered) {
            if (action->timer >= throw_windup_total_frames(action->as.throw.power)) {
                throw_release(match);
            }
        } else if (action->timer > THROW_WINDUP_MAX_FRAMES) {
            // No power yet: keep the gather arc resting fully wound while the human is still choosing.
            action->timer = THROW_WINDUP_MAX_FRAMES;
        }
    }
}

void drop_ball(MatchSession* match)
{
    // there is a possibility to drop ball if to the ground if you want. it could be convenient when
    // you want a baserunner to be able to get safe from a base for some strategical reason.
    //
    // This is the actualization only. Whether the drop was allowed at all — somebody is holding the
    // ball, and neither a throw nor a pitch has a claim on it — was decided once, at the INGEST gate,
    // against the world at the top of this tick; nothing between there and here can have taken the
    // ball away, since that would need the very throw the gate refuses to drop through. Re-asking the
    // question here would be a second home for one rule, and the two homes drift.
    float norm;
    float dx;
    float dz;

    // players' movement will be stopped when doing this, similar to throwing.
    if (match->playerInfo[match->pII.hasBallIndex].cPI.moving == 1) {
        stop_movement(match->playerInfo, match->pII.hasBallIndex);
    }
    // model is set to be the basic standing without ball model.
    match->playerInfo[match->pII.hasBallIndex].cPI.model = PLAYER_ANIM_STAND_NO_BALL;
    // and then just set a little upward-forward -directed value for ball so that we'll see the dropping
    dx = match->playerInfo[match->pII.hasBallIndex].tPI.orientation.x;
    dz = match->playerInfo[match->pII.hasBallIndex].tPI.orientation.z;
    norm = (float)sqrt(dx * dx + dz * dz);
    if (norm < EPSILON) norm = 1.0f;
    dx = dx / norm;
    dz = dz / norm;
    // and use generic_sling_ball again to get the ball to the world.
    generic_sling_ball(&(match->ballInfo), dx * DROP_BALL_CONSTANT, DROP_BALL_CONSTANT, dz * DROP_BALL_CONSTANT);
    // Trigger fielder selection update after drop
    match->pRAI.refresh_catch_and_change = 1;
    match->pRAI.init_player_selection = 1;
    // and set the lastHadBallIndex so that this player cannot catch it before it hits ground
    match->pII.lastHadBallIndex = match->pII.hasBallIndex;
    // and no player has the ball anymore.
    match->pII.hasBallIndex = -1;
}
