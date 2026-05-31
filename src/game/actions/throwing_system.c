#include "throwing_system.h"
#include "execute_actions.h"
#include "common_logic.h"
#include "vector_math.h"
#include <math.h>

#define THROW_TO_BASE_DISTANCE 1.0f
#define THROW_POWER_CONSTANT 0.65f
#define THROW_DISTANCE_CONSTANT 0.0012f

#define DROP_BALL_CONSTANT 0.02f

void init_throwing_system(StateInfo* stateInfo)
{
    stateInfo->match->pendingActionState.throwDistance = 0;
    stateInfo->match->pendingActionState.throwDirection.x = 0;
    stateInfo->match->pendingActionState.throwDirection.y = 0;
    stateInfo->match->pendingActionState.throwDirection.z = 0;
}

void prepare_throw(StateInfo* stateInfo, BaseID base)
{
    switch (base) {
    case BASE_HOME:
        stateInfo->match->pRAI.throwGoingToBase = 0;
        stateInfo->match->pendingActionState.throwDirection.x =
            stateInfo->fieldPositions->pitcher.x -
            stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.location.x;
        stateInfo->match->pendingActionState.throwDirection.z =
            stateInfo->fieldPositions->pitcher.z -
            stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.location.z;
        break;
    case BASE_FIRST:
        stateInfo->match->pRAI.throwGoingToBase = 1;
        stateInfo->match->pendingActionState.throwDirection.x =
            stateInfo->fieldPositions->firstBase.x -
            stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.location.x;
        stateInfo->match->pendingActionState.throwDirection.z =
            stateInfo->fieldPositions->firstBase.z -
            stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.location.z;
        break;
    case BASE_SECOND:
        stateInfo->match->pRAI.throwGoingToBase = 2;
        stateInfo->match->pendingActionState.throwDirection.x =
            stateInfo->fieldPositions->secondBase.x -
            stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.location.x;
        stateInfo->match->pendingActionState.throwDirection.z =
            stateInfo->fieldPositions->secondBase.z -
            stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.location.z;
        break;
    case BASE_THIRD:
        stateInfo->match->pRAI.throwGoingToBase = 3;
        stateInfo->match->pendingActionState.throwDirection.x =
            stateInfo->fieldPositions->thirdBase.x -
            stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.location.x;
        stateInfo->match->pendingActionState.throwDirection.z =
            stateInfo->fieldPositions->thirdBase.z -
            stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.location.z;
        break;
    default:
        break;
    }
}

void throw_release(StateInfo* stateInfo)
{
    if (stateInfo->match->pII.hasBallIndex != -1) {
        float power;
        // throw not going anymore, ball already flyin'
        stateInfo->match->pendingActionState.throwGoingOn = 0;
        // release animation
        stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.model = PLAYER_ANIM_THROW_RELEASE;
        stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.animationStage = 0;
        stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.animationStageCount = 21;
        stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.animationFrequency = 2;
        // set flag to indicate that animation is still going on ( so no extra movement
        // until its over ).
        stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cTPI.throwRecoil = 1;

        // take power naturally from meterCounter value
        power = 1.0f * stateInfo->match->pendingActionState.meterCounter /
                stateInfo->match->pendingActionState.meterCounterMax;
        // update these values a bit
        stateInfo->match->pendingActionState.throwDirection.x =
            stateInfo->match->pendingActionState.throwDirection.x / stateInfo->match->pendingActionState.throwDistance;
        stateInfo->match->pendingActionState.throwDirection.z =
            stateInfo->match->pendingActionState.throwDirection.z / stateInfo->match->pendingActionState.throwDistance;
        stateInfo->match->pendingActionState.throwDirection.y = 0.06f;
        // ... and then launch the ball
        generic_sling_ball(
            &(stateInfo->match->ballInfo),
            stateInfo->match->pendingActionState.throwDirection.x * power * THROW_POWER_CONSTANT,
            stateInfo->match->pendingActionState.throwDirection.y +
                stateInfo->match->pendingActionState.throwDistance * THROW_DISTANCE_CONSTANT,
            stateInfo->match->pendingActionState.throwDirection.z * power * THROW_POWER_CONSTANT
        );
        // Trigger fielder selection update after throw
        stateInfo->match->pRAI.refreshCatchAndChange = 1;
        stateInfo->match->pRAI.initPlayerSelection = 1;
        // set lastHadBallIndex, its used for example to prevent this player of catching
        // the ball right after throwing.
        stateInfo->match->pII.lastHadBallIndex = stateInfo->match->pII.hasBallIndex;
        // no player has ball anymore
        stateInfo->match->pII.hasBallIndex = -1;
        // set running flag to 0 so that orientation will change
        stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].cPI.running = 0;
        // set control to -1 and change_player to 0 as a precaution so that the player
        // wouldnt be changed right away after this, as the key
        // to do this is the same one. let the generic_sling_ball handle
        // player changing.
        stateInfo->match->pII.controlIndex = -1;
        stateInfo->match->aF.cTAF.change_player = 0;
    }
}

void throw_load(StateInfo* stateInfo, BaseID base)
{
    if (stateInfo->match->pII.hasBallIndex != -1) {
        // throw distance is the euclidean distance from the base to player throwing.
        stateInfo->match->pendingActionState.throwDistance = (float)sqrt(
            stateInfo->match->pendingActionState.throwDirection.x *
                stateInfo->match->pendingActionState.throwDirection.x +
            stateInfo->match->pendingActionState.throwDirection.z *
                stateInfo->match->pendingActionState.throwDirection.z
        );
        // if player is already on the base, cant throw.
        if (stateInfo->match->pendingActionState.throwDistance > THROW_TO_BASE_DISTANCE) {
            // stop player if he is moving, moving won't look good as the animation
            // doesn't have foot movement
            if (stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.moving == 1) {
                stop_movement(stateInfo->match->playerInfo, stateInfo->match->pII.hasBallIndex);
            }
            // set the animation
            stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.model = PLAYER_ANIM_THROW_WINDUP;
            stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.animationStage = 0;
            stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.animationStageCount = 11;
            stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.animationFrequency = 3;
            // initialize meters.
            stateInfo->match->pendingActionState.meterCounter = 0;
            stateInfo->match->pendingActionState.meterCounterMax =
                THROW_MAX; // arbitrary decision, seems about right though
            // set the flag that is used for example to determine can you move the player.
            stateInfo->match->pendingActionState.throwGoingOn = 1;
            // to avoid twitching when moving key is still pressed and player cant move as hes throwing
            stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.lastLastLocationUpdate = 1;
            // and orient player to look at the base too.
            stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.orientation.x =
                stateInfo->match->pendingActionState.throwDirection.x;
            stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.orientation.z =
                stateInfo->match->pendingActionState.throwDirection.z;
        } else {
            // if too close to base, terminate throwing.
            stateInfo->match->pendingActionState.throwGoingOn = 0;
            stateInfo->match->pRAI.throwGoingToBase = -1;
        }
    }
}

void fielder_move(StateInfo* stateInfo, int direction)
{
    // we can move if there is no throw going on and no pitch going on
    // .. and we have same player controlled
    if (stateInfo->match->pendingActionState.throwGoingOn == 0 &&
        stateInfo->match->pRAI.pitchState == PITCH_STAGE_NONE && stateInfo->match->pII.controlIndex != -1) {
        // stopping only possible when moving already going on
        // so thats the reason for this value 2
        stateInfo->match->aF.cTAF.move[direction] = ACTION_ACTIVE;

        stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].cTPI.movesToDirection[direction] = 1;
        // and we call this generic function that utilizes this movesToDirection to select
        // velocity and orientation for the player
        update_controlled_player_speed(stateInfo);
    } else {
        stateInfo->match->aF.cTAF.move[direction] = 0;
    }
}

void fielder_stop_move(StateInfo* stateInfo, int direction)
{
    // stopping cant be done either when pitching or throwing as update_controlled_player_speed can
    // have effects on player's model
    if (stateInfo->match->pendingActionState.throwGoingOn == 0 &&
        stateInfo->match->pRAI.pitchState == PITCH_STAGE_NONE && stateInfo->match->pII.controlIndex != -1) {
        stateInfo->match->aF.cTAF.move[direction] = ACTION_IDLE;
        stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].cTPI.movesToDirection[direction] = 0;
        update_controlled_player_speed(stateInfo);
    } else {
        stateInfo->match->aF.cTAF.move[direction] = 0;
    }
}

void drop_ball(StateInfo* stateInfo)
{
    // there is a possibility to drop ball if to the ground if you want. it could be convenient when
    // you want a baserunner to be able to get safe from a base for some strategical reason.
    if (stateInfo->match->pII.hasBallIndex != -1) {
        if (stateInfo->match->pendingActionState.throwGoingOn == 0 &&
            stateInfo->match->pRAI.pitchState == PITCH_STAGE_NONE) {
            float norm;
            float dx;
            float dz;

            // players' movement will be stopped when doing this, similar to throwing.
            if (stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.moving == 1) {
                stop_movement(stateInfo->match->playerInfo, stateInfo->match->pII.hasBallIndex);
            }
            // model is set to be the basic standing without ball model.
            stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].cPI.model = PLAYER_ANIM_STAND_NO_BALL;
            // and then just set a little upward-forward -directed value for ball so that we'll see the dropping
            dx = stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.orientation.x;
            dz = stateInfo->match->playerInfo[stateInfo->match->pII.hasBallIndex].tPI.orientation.z;
            norm = (float)sqrt(dx * dx + dz * dz);
            if (norm < EPSILON) norm = 1.0f;
            dx = dx / norm;
            dz = dz / norm;
            // and use generic_sling_ball again to get the ball to the world.
            generic_sling_ball(
                &(stateInfo->match->ballInfo), dx * DROP_BALL_CONSTANT, DROP_BALL_CONSTANT, dz * DROP_BALL_CONSTANT
            );
            // Trigger fielder selection update after drop
            stateInfo->match->pRAI.refreshCatchAndChange = 1;
            stateInfo->match->pRAI.initPlayerSelection = 1;
            // and set the lastHadBallIndex so that this player cannot catch it before it hits ground
            stateInfo->match->pII.lastHadBallIndex = stateInfo->match->pII.hasBallIndex;
            // and no player has the ball anymore.
            stateInfo->match->pII.hasBallIndex = -1;
        }
    }
    stateInfo->match->aF.cTAF.dropBall = ACTION_IDLE;
}

void update_controlled_player_speed(StateInfo* stateInfo)
{
    if (stateInfo->match->pII.controlIndex != -1) {
        // cant move when throw recoil going on.
        if (stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].cTPI.throwRecoil == 0) {
            float norm;
            // we select the direction by taking the difference of moves in x direction and moves in z direction
            // moves are 0 or 1, so as a net result we will get the direction where the player really should be going on
            int directionX = stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].cTPI.movesToDirection[1] -
                             stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].cTPI.movesToDirection[3];
            int directionZ =
                -stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].cTPI.movesToDirection[0] +
                stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].cTPI.movesToDirection[2];
            // always when player's velocity changes, ball's velocity must change too.
            stateInfo->match->ballInfo.needsMoveUpdate = 1;
            // if every component vanishes
            if (directionX * directionX + directionZ * directionZ == 0) {
                // set moving to zero
                stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].cPI.moving = 0;
                // if controlled player has also ball, set corresponding model
                // otherwise set model without ball
                if (stateInfo->match->pII.hasBallIndex == stateInfo->match->pII.controlIndex)
                    stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].cPI.model =
                        PLAYER_ANIM_STAND_WITH_BALL;
                else
                    stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].cPI.model =
                        PLAYER_ANIM_STAND_NO_BALL;
                // when stopping movement, need to update last location.
                stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].cPI.lastLastLocationUpdate = 1;
            } else {
                // if there is a non-zero component in x or z direction
                // moving is to be 1 and we are going to have walking or running animation
                stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].cPI.moving = 1;
                stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].cPI.animationFrequency = 3;
                stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].cPI.animationStage = 0;
                // set player's orientation so that player faces the direction he's moving to
                stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].tPI.orientation.x = (float)directionX;
                stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].tPI.orientation.z = (float)directionZ;

                // Find norm
                norm = (float)sqrt(directionX * directionX + directionZ * directionZ);
                if (norm < EPSILON) norm = 1.0f;

                // running
                stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].tPI.velocity.x =
                    (float)directionX * RUN_SPEED / norm;
                stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].tPI.velocity.z =
                    (float)directionZ * RUN_SPEED / norm;
                stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].cPI.animationStageCount = 20;
                // if has ball, then running with ball model, otherwise running without ball
                if (stateInfo->match->pII.hasBallIndex == stateInfo->match->pII.controlIndex) {
                    stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].cPI.model =
                        PLAYER_ANIM_RUN_WITH_BALL;

                } else {
                    stateInfo->match->playerInfo[stateInfo->match->pII.controlIndex].cPI.model =
                        PLAYER_ANIM_RUN_NO_BALL;
                }
            }
        }
    }
}
