#include "throwing_system.h"
#include "execute_actions.h"
#include "common_logic.h"
#include "vector_math.h"
#include <math.h>

#define THROW_TO_BASE_DISTANCE 1.0f
#define THROW_POWER_CONSTANT 0.65f
#define THROW_DISTANCE_CONSTANT 0.0012f

#define DROP_BALL_CONSTANT 0.02f

float throw_charge_to_power(int charge)
{
    if (charge < 0) charge = 0;
    if (charge > THROW_CHARGE_MAX) charge = THROW_CHARGE_MAX;
    float t = (float)charge / (float)THROW_CHARGE_MAX;
    return THROW_POWER_MIN + (1.0f - THROW_POWER_MIN) * t;
}

static void
orient_player_toward_base(MatchSession* match, const FieldPositions* fieldPositions, int playerIndex, BaseID base)
{
    Vector3D target;
    switch (base) {
    case BASE_HOME:
        target = fieldPositions->pitcher;
        break;
    case BASE_FIRST:
        target = fieldPositions->firstBase;
        break;
    case BASE_SECOND:
        target = fieldPositions->secondBase;
        break;
    case BASE_THIRD:
        target = fieldPositions->thirdBase;
        break;
    default:
        return;
    }
    // Same un-normalized direction vector throw_load writes; the renderer only reads its angle.
    match->playerInfo[playerIndex].tPI.orientation.x = target.x - match->playerInfo[playerIndex].tPI.location.x;
    match->playerInfo[playerIndex].tPI.orientation.z = target.z - match->playerInfo[playerIndex].tPI.location.z;
}

void init_throwing_system(MatchSession* match)
{
    match->pendingActionState.throw_distance = 0;
    match->pendingActionState.throw_direction.x = 0;
    match->pendingActionState.throw_direction.y = 0;
    match->pendingActionState.throw_direction.z = 0;
}

void prepare_throw(MatchSession* match, const FieldPositions* fieldPositions, BaseID base)
{
    switch (base) {
    case BASE_HOME:
        match->pRAI.throw_going_to_base = 0;
        match->pendingActionState.throw_direction.x =
            fieldPositions->pitcher.x - match->playerInfo[match->pII.hasBallIndex].tPI.location.x;
        match->pendingActionState.throw_direction.z =
            fieldPositions->pitcher.z - match->playerInfo[match->pII.hasBallIndex].tPI.location.z;
        break;
    case BASE_FIRST:
        match->pRAI.throw_going_to_base = 1;
        match->pendingActionState.throw_direction.x =
            fieldPositions->firstBase.x - match->playerInfo[match->pII.hasBallIndex].tPI.location.x;
        match->pendingActionState.throw_direction.z =
            fieldPositions->firstBase.z - match->playerInfo[match->pII.hasBallIndex].tPI.location.z;
        break;
    case BASE_SECOND:
        match->pRAI.throw_going_to_base = 2;
        match->pendingActionState.throw_direction.x =
            fieldPositions->secondBase.x - match->playerInfo[match->pII.hasBallIndex].tPI.location.x;
        match->pendingActionState.throw_direction.z =
            fieldPositions->secondBase.z - match->playerInfo[match->pII.hasBallIndex].tPI.location.z;
        break;
    case BASE_THIRD:
        match->pRAI.throw_going_to_base = 3;
        match->pendingActionState.throw_direction.x =
            fieldPositions->thirdBase.x - match->playerInfo[match->pII.hasBallIndex].tPI.location.x;
        match->pendingActionState.throw_direction.z =
            fieldPositions->thirdBase.z - match->playerInfo[match->pII.hasBallIndex].tPI.location.z;
        break;
    default:
        break;
    }
}

void throw_release(MatchSession* match)
{
    if (match->pII.hasBallIndex != -1) {
        float power;
        // throw not going anymore, ball already flyin'
        match->pendingActionState.throw_going_on = 0;
        // release animation
        match->playerInfo[match->pII.hasBallIndex].cPI.model = PLAYER_ANIM_THROW_RELEASE;
        match->playerInfo[match->pII.hasBallIndex].cPI.animationStage = 0;
        match->playerInfo[match->pII.hasBallIndex].cPI.animationStageCount = 21;
        match->playerInfo[match->pII.hasBallIndex].cPI.animationFrequency = 2;
        // set flag to indicate that animation is still going on ( so no extra movement
        // until its over ).
        match->playerInfo[match->pII.hasBallIndex].cTPI.throwRecoil = 1;

        // power is the DECLARED value carried by the intent (copied into throw_power at throw_load) —
        // never a live meter read. The meter/clock only times the windup; it does not set the outcome.
        power = match->pendingActionState.throw_power;
        // update these values a bit
        match->pendingActionState.throw_direction.x =
            match->pendingActionState.throw_direction.x / match->pendingActionState.throw_distance;
        match->pendingActionState.throw_direction.z =
            match->pendingActionState.throw_direction.z / match->pendingActionState.throw_distance;
        match->pendingActionState.throw_direction.y = 0.06f;
        // ... and then launch the ball
        generic_sling_ball(
            &(match->ballInfo), match->pendingActionState.throw_direction.x * power * THROW_POWER_CONSTANT,
            match->pendingActionState.throw_direction.y +
                match->pendingActionState.throw_distance * THROW_DISTANCE_CONSTANT,
            match->pendingActionState.throw_direction.z * power * THROW_POWER_CONSTANT
        );
        // Trigger fielder selection update after throw
        match->pRAI.refresh_catch_and_change = 1;
        match->pRAI.init_player_selection = 1;
        // set lastHadBallIndex, its used for example to prevent this player of catching
        // the ball right after throwing.
        match->pII.lastHadBallIndex = match->pII.hasBallIndex;
        // no player has ball anymore
        match->pII.hasBallIndex = -1;
        // set running flag to 0 so that orientation will change
        match->playerInfo[match->pII.controlIndex].cPI.running = 0;
        // set control to -1 and change_player to 0 as a precaution so that the player
        // wouldnt be changed right away after this, as the key
        // to do this is the same one. let the generic_sling_ball handle
        // player changing.
        match->pII.controlIndex = -1;
        match->aF.cTAF.change_player = 0;
    }
}

void throw_load(MatchSession* match, BaseID base, float power)
{
    if (match->pII.hasBallIndex != -1) {
        // throw distance is the euclidean distance from the base to player throwing.
        match->pendingActionState.throw_distance = (float)sqrt(
            match->pendingActionState.throw_direction.x * match->pendingActionState.throw_direction.x +
            match->pendingActionState.throw_direction.z * match->pendingActionState.throw_direction.z
        );
        // if player is already on the base, cant throw.
        if (match->pendingActionState.throw_distance > THROW_TO_BASE_DISTANCE) {
            // stop player if he is moving, moving won't look good as the animation
            // doesn't have foot movement
            if (match->playerInfo[match->pII.hasBallIndex].cPI.moving == 1) {
                stop_movement(match->playerInfo, match->pII.hasBallIndex);
            }
            // set the animation
            match->playerInfo[match->pII.hasBallIndex].cPI.model = PLAYER_ANIM_THROW_WINDUP;
            match->playerInfo[match->pII.hasBallIndex].cPI.animationStage = 0;
            match->playerInfo[match->pII.hasBallIndex].cPI.animationStageCount = 11;
            match->playerInfo[match->pII.hasBallIndex].cPI.animationFrequency = 3;
            // Store the declared power, clamped to [0,1]. The release velocity reads this — not a meter.
            if (power < 0.0f) power = 0.0f;
            if (power > 1.0f) power = 1.0f;
            match->pendingActionState.throw_power = power;
            // The windup is an engine-owned clock: meter_counter ramps 0 → meter_counter_max, and the ball
            // leaves when it completes (execute_actions). The windup LENGTH is FIXED (THROW_WINDUP) — the
            // same short wait for every throw, independent of power; power only sets the release velocity
            // (above). No game logic reads meter_counter for the outcome — only to time the windup.
            match->pendingActionState.meter_counter = 0;
            match->pendingActionState.meter_counter_max = THROW_WINDUP;
            // set the flag that is used for example to determine can you move the player.
            match->pendingActionState.throw_going_on = 1;
            // to avoid twitching when moving key is still pressed and player cant move as hes throwing
            match->playerInfo[match->pII.hasBallIndex].cPI.lastLastLocationUpdate = 1;
            // and orient player to look at the base too.
            match->playerInfo[match->pII.hasBallIndex].tPI.orientation.x = match->pendingActionState.throw_direction.x;
            match->playerInfo[match->pII.hasBallIndex].tPI.orientation.z = match->pendingActionState.throw_direction.z;
        } else {
            // if too close to base, terminate throwing.
            match->pendingActionState.throw_going_on = 0;
            match->pRAI.throw_going_to_base = -1;
        }
    }
}

void fielder_move(MatchSession* match, int direction)
{
    // we can move if there is no throw going on and no pitch going on
    // .. and we have same player controlled
    if (match->pendingActionState.throw_going_on == 0 && match->pRAI.pitch_state == PITCH_STAGE_NONE &&
        match->pII.controlIndex != -1) {
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
    if (match->pendingActionState.throw_going_on == 0 && match->pRAI.pitch_state == PITCH_STAGE_NONE &&
        match->pII.controlIndex != -1) {
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
    if (match->pII.hasBallIndex != -1) {
        if (match->pendingActionState.throw_going_on == 0 && match->pRAI.pitch_state == PITCH_STAGE_NONE) {
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
            generic_sling_ball(
                &(match->ballInfo), dx * DROP_BALL_CONSTANT, DROP_BALL_CONSTANT, dz * DROP_BALL_CONSTANT
            );
            // Trigger fielder selection update after drop
            match->pRAI.refresh_catch_and_change = 1;
            match->pRAI.init_player_selection = 1;
            // and set the lastHadBallIndex so that this player cannot catch it before it hits ground
            match->pII.lastHadBallIndex = match->pII.hasBallIndex;
            // and no player has the ball anymore.
            match->pII.hasBallIndex = -1;
        }
    }
    match->aF.cTAF.drop_ball = ACTION_IDLE;
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

// Client-visual only. While a human is charging a throw, face the ball-holder toward the latched target
// base so they can see where the ball will go. Called from execute_actions right after the per-direction
// move handling, so it is the single orientation writer for a charging thrower: movement is suppressed
// during a charge (checkMove stops the fielder while the action key is held), so the two never fight, and
// running this last makes the facing authoritative if a stale move event set an orientation that frame.
// It never touches the throw outcome — that direction is computed fresh at declaration (prepare_throw) —
// and is gated on the human charge gesture, which the AI never sets.
void update_thrower_facing(
    MatchSession* match, const ClientInputState* clientInput, const FieldPositions* fieldPositions
)
{
    const ThrowCharge* tc = &clientInput->throw_charge;
    if (!tc->engaged || tc->base == BASE_NONE || match->pII.hasBallIndex == -1) {
        return;
    }
    orient_player_toward_base(match, fieldPositions, match->pII.hasBallIndex, tc->base);
}
