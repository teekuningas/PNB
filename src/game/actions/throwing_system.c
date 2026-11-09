#include "throwing_system.h"
#include "field.h"
#include "intents.h"
#include "world.h"
#include "ball_flight.h"
#include "player_motion.h"
#include "fielder_behaviours.h"
#include <math.h>

#define THROW_POWER_CONSTANT 0.65f
#define THROW_DISTANCE_CONSTANT 0.0012f

#define DROP_BALL_CONSTANT 0.02f

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

// The line from the ball-holder to `base`, unnormalized: what a throw there would travel along.
static Vector3D line_to_base(const MatchSession* match, const FieldPositions* fieldPositions, BaseID base)
{
    Vector3D target = throw_target_point(fieldPositions, base);
    const Vector3D* from = &match->playerInfo[match->ballInfo.holder].body.location;
    return (Vector3D){.x = target.x - from->x, .y = 0.0f, .z = target.z - from->z};
}

static float horizontal_length(Vector3D v)
{
    return (float)sqrt(v.x * v.x + v.z * v.z);
}

// May a throw to `base` begin in the world as it stands? Asked by the INGEST gate of a declared target:
// nothing else is being done with the ball, somebody is holding it, and they are not already standing on
// the base they would throw to.
Permission throw_may_begin(const MatchSession* match, const FieldPositions* fieldPositions, BaseID base)
{
    if (match->catching_action.kind != CATCHING_ACTION_NONE) {
        return (Permission){0, RULE_CATCHING_ACTION_IN_PROGRESS};
    }
    if (match->ballInfo.holder == -1) {
        return (Permission){0, RULE_THROW_NEEDS_BALL_IN_HAND};
    }
    if (horizontal_length(line_to_base(match, fieldPositions, base)) <= THROW_TO_BASE_DISTANCE) {
        return (Permission){0, RULE_THROW_TARGET_TOO_CLOSE};
    }
    return (Permission){1, RULE_NONE};
}

// Begin the engine-owned windup for a throw toward `base`: latch the line to the target, stop and orient
// the thrower, and start the deterministic clock. The gate has already established that it may begin; the
// gather animation follows the clock downstream.
static void begin_throw_windup(MatchSession* match, const FieldPositions* fieldPositions, BaseID base)
{
    int thrower = match->ballInfo.holder;
    ThrowWindup windup = {.target = base, .direction = line_to_base(match, fieldPositions, base)};
    windup.distance = horizontal_length(windup.direction);

    // Stop the thrower if moving — the gather animation has no foot movement.
    if (match->playerInfo[thrower].motion.moving == 1) {
        stop_movement(match->playerInfo, thrower);
    }
    record_player_activity(&match->playerInfo[thrower].events, PLAYER_ACTIVITY_THROW_BEGUN, true);
    // Avoid twitching if a move key is still down while the thrower can't move.
    match->playerInfo[thrower].motion.sync_previous_location = 1;
    // Face the target base.
    match->playerInfo[thrower].body.orientation.x = windup.direction.x;
    match->playerInfo[thrower].body.orientation.z = windup.direction.z;

    match->catching_action = (CatchingAction){.kind = CATCHING_ACTION_THROWING, .timer = 0, .as.throw = windup};
}

// Launch the ball toward the latched base with the DECLARED power — a trusted value (the AI's strategy, or
// the human's charge-widget sample), never a live meter or the engine clock. The action resolves here: the
// record is zeroed, which is the engine's "this one is done".
static void throw_release(MatchSession* match)
{
    ThrowWindup windup = match->catching_action.as.throw;
    match->catching_action = (CatchingAction){0};
    if (match->ballInfo.holder == -1) {
        return;
    }
    float power = windup.power;
    if (power < 0.0f) power = 0.0f;
    if (power > 1.0f) power = 1.0f;

    record_player_activity(&match->playerInfo[match->ballInfo.holder].events, PLAYER_ACTIVITY_THROW_RELEASED, true);
    match->playerInfo[match->ballInfo.holder].cTPI.throw_recovery_ticks = THROW_RECOVERY_TICKS;

    // normalize the latched direction and give it a small upward component
    float dx = windup.direction.x / windup.distance;
    float dz = windup.direction.z / windup.distance;
    float dy = 0.06f;
    generic_sling_ball(
        &(match->ballInfo), dx * power * THROW_POWER_CONSTANT, dy + windup.distance * THROW_DISTANCE_CONSTANT,
        dz * power * THROW_POWER_CONSTANT
    );
    // The flight is a throw to that base, which keeps the baseman there from wandering out to meet it.
    // Set at the release and not at the gather: it is a fact about the ball in the air, and every reader
    // asks it with nobody holding the ball — while the thrower gathers, the target is the windup's own.
    match->ballInfo.thrown_to = windup.target;
    // Trigger fielder selection update after throw
    request_chase_replan(match, CHASE_REPLAN_AND_HAND_OVER);
    // prevent this player from catching the ball right after throwing
    match->ballInfo.released_by = match->ballInfo.holder;
    match->ballInfo.holder = -1;
    // set running flag to 0 so that orientation will change
    match->playerInfo[match->catchingState.controlled_fielder].motion.running = 0;
    // set control to -1 (the change key is the same as the throw key) — let generic_sling_ball handle
    // player changing.
    match->catchingState.controlled_fielder = -1;
}

void advance_throw_recovery(CatchingTeamPlayerInfo* fielding)
{
    if (fielding->throw_recovery_ticks > 0) fielding->throw_recovery_ticks--;
}

// The throw actualizer — the single engine entry point, the throw's counterpart of the pitch's. A declared
// target begins the gather (the gate has already established that it may); a declared power, whenever it arrives, is
// held on it; and the ball leaves once the power is known and the windup for that power has elapsed. ONE rule for both
// producers: the AI declares both values at once, so its clock starts with the power known; a human declares the target
// first, so the gather runs WHILE it picks power on its own meter and the release usually follows the
// moment the power arrives. The client never commands the release instant; the engine owns it.
void update_throw_actualization(MatchSession* match, const FieldPositions* fieldPositions, const ThrowValues* declared)
{
    CatchingAction* action = &match->catching_action;

    // The gate has settled what these may do: a target only when nothing else is in progress, a power
    // only while a throw is gathering or begins this tick.
    if (declared->targetDeclared) {
        begin_throw_windup(match, fieldPositions, declared->target);
    }
    if (declared->powerDeclared) {
        action->as.throw.powered = 1;
        action->as.throw.power = declared->power;
    }
    if (declared->targetDeclared) return; // never begin and release in the same frame — the windup is at least one tick

    if (action->kind == CATCHING_ACTION_THROWING) {
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
    if (match->playerInfo[match->ballInfo.holder].motion.moving == 1) {
        stop_movement(match->playerInfo, match->ballInfo.holder);
    }
    record_player_activity(&match->playerInfo[match->ballInfo.holder].events, PLAYER_ACTIVITY_BALL_DROPPED, false);
    // and then just set a little upward-forward -directed value for ball so that we'll see the dropping
    dx = match->playerInfo[match->ballInfo.holder].body.orientation.x;
    dz = match->playerInfo[match->ballInfo.holder].body.orientation.z;
    norm = (float)sqrt(dx * dx + dz * dz);
    if (norm < EPSILON) norm = 1.0f;
    dx = dx / norm;
    dz = dz / norm;
    // and use generic_sling_ball again to get the ball to the world.
    generic_sling_ball(&(match->ballInfo), dx * DROP_BALL_CONSTANT, DROP_BALL_CONSTANT, dz * DROP_BALL_CONSTANT);
    // Trigger fielder selection update after drop
    request_chase_replan(match, CHASE_REPLAN_AND_HAND_OVER);
    // and set released_by so that this player cannot catch it before it hits ground
    match->ballInfo.released_by = match->ballInfo.holder;
    // and no player has the ball anymore.
    match->ballInfo.holder = -1;
}
