#include "throwing_system.h"
#include "execute_actions.h"
#include "common_logic.h"
#include "vector_math.h"
#include <math.h>

#define THROW_POWER_CONSTANT 0.65f
#define THROW_DISTANCE_CONSTANT 0.0012f

#define DROP_BALL_CONSTANT 0.02f

#define THROW_ANIMATION_FREQUENCY 3

// Windup sizing: how long the physical windup for a declared power lasts. Once an intent is COMMITTED the
// engine releases when its clock reaches this — for BOTH producers. A human's clock starts earlier
// (at INITIATED, running while it picks power on the client charge widget), so by its COMMITTED frame the
// windup has usually elapsed → immediate release; the client never reads this function or the clock.
int throw_windup_total_frames(float power)
{
    if (power < THROW_POWER_MIN) power = THROW_POWER_MIN;
    if (power > 1.0f) power = 1.0f;
    float t = (power - THROW_POWER_MIN) / (1.0f - THROW_POWER_MIN);
    return THROW_WINDUP_MIN_FRAMES + (int)(t * (THROW_WINDUP_MAX_FRAMES - THROW_WINDUP_MIN_FRAMES));
}

void init_throwing_system(MatchSession* match)
{
    match->pendingActionState.throw_distance = 0;
    match->pendingActionState.throw_direction.x = 0;
    match->pendingActionState.throw_direction.y = 0;
    match->pendingActionState.throw_direction.z = 0;
    match->pendingActionState.throwActualization.timer = 0;
}

static void clear_throw_declaration(ThrowDeclaration* decl)
{
    decl->phase = THROW_DECL_IDLE;
    decl->target = BASE_NONE;
    decl->power = 0.0f;
}

// Compute the (un-normalized) throw direction from the ball-holder toward a target base, and record which
// base the throw is going to (throw_going_to_base keeps basemen from wandering out to catch it).
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

static void set_throw_direction(MatchSession* match, const FieldPositions* fieldPositions, BaseID base)
{
    if (base < BASE_HOME || base >= BASE_COUNT) return;

    Vector3D target = throw_target_point(fieldPositions, base);
    match->pRAI.throw_going_to_base = (int)base;
    match->pendingActionState.throw_direction.x = target.x - match->playerInfo[match->pII.hasBallIndex].tPI.location.x;
    match->pendingActionState.throw_direction.z = target.z - match->playerInfo[match->pII.hasBallIndex].tPI.location.z;
}

// Begin the engine-owned windup for a throw toward `base`: cancel any pitch, latch the direction/distance,
// stop and orient the thrower, and start the deterministic clock. Returns 1 if the windup began, 0 if it
// could not (no ball, or the thrower already stands on the target base — too close to throw). This is the
// throw analog of the pitch's begin_windup; the gather animation follows the clock downstream.
static int begin_throw_windup(MatchSession* match, const FieldPositions* fieldPositions, BaseID base)
{
    if (match->pII.hasBallIndex == -1) {
        return 0;
    }

    // A pitch may be cancelled for a throw (never the reverse): drop the pitch declaration + its windup
    // clock, and move the ball back from the plate centre to the pitcher's hand.
    if (match->pRAI.pitch_state != PITCH_STAGE_NONE) {
        match->pendingActionState.pitchDeclaration.phase = PITCH_DECL_IDLE;
        match->pendingActionState.pitchDeclaration.power = 0.0f;
        match->pendingActionState.pitchDeclaration.direction = 0.0f;
        match->pRAI.pitch_state = PITCH_STAGE_NONE;
        match->pendingActionState.pitchActualization.timer = 0;
        match->ballInfo.location.x = match->playerInfo[match->pII.hasBallIndex].tPI.location.x;
        match->ballInfo.location.z = match->playerInfo[match->pII.hasBallIndex].tPI.location.z;
    }

    set_throw_direction(match, fieldPositions, base);
    match->pendingActionState.throw_distance = (float)sqrt(
        match->pendingActionState.throw_direction.x * match->pendingActionState.throw_direction.x +
        match->pendingActionState.throw_direction.z * match->pendingActionState.throw_direction.z
    );
    // Can't throw to a base you are already standing on.
    if (match->pendingActionState.throw_distance <= THROW_TO_BASE_DISTANCE) {
        match->pRAI.throw_going_to_base = -1;
        return 0;
    }

    int thrower = match->pII.hasBallIndex;
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
    match->playerInfo[thrower].tPI.orientation.x = match->pendingActionState.throw_direction.x;
    match->playerInfo[thrower].tPI.orientation.z = match->pendingActionState.throw_direction.z;

    match->pendingActionState.throwActualization.timer = 0;
    match->pendingActionState.current_catching_action = CATCHING_ACTION_THROWING;
    return 1;
}

// Launch the ball toward the latched base with the DECLARED power — a trusted client value (the AI's
// strategy, or the human's charge-widget sample carried on the COMMITTED declaration) — never a live meter
// or the engine clock. Preserves every legacy release side effect (recoil, fielder-selection
// refresh, control handoff to generic_sling_ball).
static void throw_release(MatchSession* match, float power)
{
    if (match->pII.hasBallIndex == -1) {
        return;
    }
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
    match->pendingActionState.throw_direction.x =
        match->pendingActionState.throw_direction.x / match->pendingActionState.throw_distance;
    match->pendingActionState.throw_direction.z =
        match->pendingActionState.throw_direction.z / match->pendingActionState.throw_distance;
    match->pendingActionState.throw_direction.y = 0.06f;
    generic_sling_ball(
        &(match->ballInfo), match->pendingActionState.throw_direction.x * power * THROW_POWER_CONSTANT,
        match->pendingActionState.throw_direction.y +
            match->pendingActionState.throw_distance * THROW_DISTANCE_CONSTANT,
        match->pendingActionState.throw_direction.z * power * THROW_POWER_CONSTANT
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

// Release and clear all throw actualization state back to idle (consumer-clears the declaration).
static void resolve_throw(MatchSession* match, ThrowDeclaration* decl, float power)
{
    throw_release(match, power);
    match->pendingActionState.current_catching_action = CATCHING_ACTION_NONE;
    match->pendingActionState.throwActualization.timer = 0;
    clear_throw_declaration(decl);
}

// The throw actualizer — the single engine entry point (the throw analog of update_pitch_actualization).
// Reads the phased ThrowDeclaration, runs the deterministic windup clock, and — once the intent is COMMITTED
// (power known) — releases when the clock reaches throw_windup_total_frames(power). ONE rule for both
// producers: the AI declares COMMITTED at once (clock starts then); a human declares INITIATED first (clock
// starts, gather animates while it picks power on the meter) then COMMITTED (power in), releasing at once if
// the windup already elapsed. The client never commands the release instant; the engine owns it.
void update_throw_actualization(MatchSession* match, const FieldPositions* fieldPositions)
{
    ThrowDeclaration* decl = &match->pendingActionState.throwDeclaration;
    PendingActionState* pas = &match->pendingActionState;

    // Begin a windup the first frame a throw is declared while no catching action is in progress — if it
    // can legally begin; otherwise drop the declaration.
    if (decl->phase != THROW_DECL_IDLE && pas->current_catching_action == CATCHING_ACTION_NONE) {
        if (!begin_throw_windup(match, fieldPositions, decl->target)) {
            clear_throw_declaration(decl);
        }
        return; // never begin and release in the same frame — the windup is at least one tick
    }

    if (pas->current_catching_action == CATCHING_ACTION_THROWING) {
        pas->throwActualization.timer++;

        if (decl->phase == THROW_DECL_COMMITTED) {
            // The full intent is assembled (power known — the AI at once, or the human's second frame). The
            // ENGINE owns the release instant: fire once the physical windup for that power has elapsed. The
            // clock has run since the throw began (INITIATED for a human, so concurrently with the human's
            // power pick — no double-wait), so it is usually already past the target → immediate; if power
            // arrived early it waits out the windup. One rule for both producers; no client "fire now" edge.
            if (pas->throwActualization.timer >= throw_windup_total_frames(decl->power)) {
                resolve_throw(match, decl, decl->power);
            }
        } else {
            // INITIATED (human: direction declared, power pending): keep winding to drive the render gather
            // arc; cap at the full-power windup so it rests at full while the human is still picking power
            // (holding past full does not overspill). Cannot release yet — the power is not known.
            if (pas->throwActualization.timer > THROW_WINDUP_MAX_FRAMES) {
                pas->throwActualization.timer = THROW_WINDUP_MAX_FRAMES;
            }
        }
    }
}

void fielder_move(MatchSession* match, int direction)
{
    // we can move if there is no throw going on and no pitch going on
    // .. and we have same player controlled
    if (match->pendingActionState.current_catching_action != CATCHING_ACTION_THROWING &&
        match->pRAI.pitch_state == PITCH_STAGE_NONE && match->pII.controlIndex != -1) {
        // stopping only possible when moving already going on
        // so thats the reason for this value 2
        match->aF.cTAF.move[direction] = ACTION_ACTIVE;

        match->playerInfo[match->pII.controlIndex].cTPI.movesToDirection[direction] = 1;
        // and we call this generic function that utilizes this movesToDirection to select
        // velocity and orientation for the player
        update_controlled_player_speed(match);
    } else {
        match->aF.cTAF.move[direction] = 0;
    }
}

void fielder_stop_move(MatchSession* match, int direction)
{
    // stopping cant be done either when pitching or throwing as update_controlled_player_speed can
    // have effects on player's model
    if (match->pendingActionState.current_catching_action != CATCHING_ACTION_THROWING &&
        match->pRAI.pitch_state == PITCH_STAGE_NONE && match->pII.controlIndex != -1) {
        match->aF.cTAF.move[direction] = ACTION_IDLE;
        match->playerInfo[match->pII.controlIndex].cTPI.movesToDirection[direction] = 0;
        update_controlled_player_speed(match);
    } else {
        match->aF.cTAF.move[direction] = 0;
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

void update_controlled_player_speed(MatchSession* match)
{
    if (match->pII.controlIndex != -1) {
        // cant move when throw recoil going on.
        if (match->playerInfo[match->pII.controlIndex].cTPI.throwRecoil == 0) {
            float norm;
            // we select the direction by taking the difference of moves in x direction and moves in z direction
            // moves are 0 or 1, so as a net result we will get the direction where the player really should be going on
            int directionX = match->playerInfo[match->pII.controlIndex].cTPI.movesToDirection[1] -
                             match->playerInfo[match->pII.controlIndex].cTPI.movesToDirection[3];
            int directionZ = -match->playerInfo[match->pII.controlIndex].cTPI.movesToDirection[0] +
                             match->playerInfo[match->pII.controlIndex].cTPI.movesToDirection[2];
            // always when player's velocity changes, ball's velocity must change too.
            match->ballInfo.needsMoveUpdate = 1;
            // if every component vanishes
            if (directionX * directionX + directionZ * directionZ == 0) {
                // set moving to zero
                match->playerInfo[match->pII.controlIndex].cPI.moving = 0;
                // if controlled player has also ball, set corresponding model
                // otherwise set model without ball
                if (match->pII.hasBallIndex == match->pII.controlIndex)
                    match->playerInfo[match->pII.controlIndex].cPI.model = PLAYER_ANIM_STAND_WITH_BALL;
                else
                    match->playerInfo[match->pII.controlIndex].cPI.model = PLAYER_ANIM_STAND_NO_BALL;
                // when stopping movement, need to update last location.
                match->playerInfo[match->pII.controlIndex].cPI.lastLastLocationUpdate = 1;
            } else {
                // if there is a non-zero component in x or z direction
                // moving is to be 1 and we are going to have walking or running animation
                match->playerInfo[match->pII.controlIndex].cPI.moving = 1;
                match->playerInfo[match->pII.controlIndex].cPI.animationFrequency = 3;
                match->playerInfo[match->pII.controlIndex].cPI.animationStage = 0;
                // set player's orientation so that player faces the direction he's moving to
                match->playerInfo[match->pII.controlIndex].tPI.orientation.x = (float)directionX;
                match->playerInfo[match->pII.controlIndex].tPI.orientation.z = (float)directionZ;

                // Find norm
                norm = (float)sqrt(directionX * directionX + directionZ * directionZ);
                if (norm < EPSILON) norm = 1.0f;

                // running
                match->playerInfo[match->pII.controlIndex].tPI.velocity.x = (float)directionX * RUN_SPEED / norm;
                match->playerInfo[match->pII.controlIndex].tPI.velocity.z = (float)directionZ * RUN_SPEED / norm;
                match->playerInfo[match->pII.controlIndex].cPI.animationStageCount = 20;
                // if has ball, then running with ball model, otherwise running without ball
                if (match->pII.hasBallIndex == match->pII.controlIndex) {
                    match->playerInfo[match->pII.controlIndex].cPI.model = PLAYER_ANIM_RUN_WITH_BALL;

                } else {
                    match->playerInfo[match->pII.controlIndex].cPI.model = PLAYER_ANIM_RUN_NO_BALL;
                }
            }
        }
    }
}
