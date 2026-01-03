#include "throwing_system.h"
#include "action_implementation.h"
#include "actions_messy/action_state.h"
#include "common_logic.h"
#include "vector_math.h"
#include <math.h>

#define THROW_TO_BASE_DISTANCE 1.0f
#define THROW_POWER_CONSTANT 0.65f
#define THROW_DISTANCE_CONSTANT 0.0012f

#define DROP_BALL_CONSTANT 0.02f

static float throwDistance;
static Vector3D throwDirection;

void initThrowingSystem(void)
{
	throwDistance = 0;
	throwDirection.x = 0;
	throwDirection.y = 0;
	throwDirection.z = 0;
}

void prepareThrow(StateInfo* stateInfo, int base)
{
	switch(base) {
	case 0:
		stateInfo->localGameInfo->pRAI.throwGoingToBase = 0;
		throwDirection.x = stateInfo->fieldPositions->pitcher.x - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.x;
		throwDirection.z = stateInfo->fieldPositions->pitcher.z - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.z;
		break;
	case 1:
		stateInfo->localGameInfo->pRAI.throwGoingToBase = 1;
		throwDirection.x = stateInfo->fieldPositions->firstBase.x - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.x;
		throwDirection.z = stateInfo->fieldPositions->firstBase.z - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.z;
		break;
	case 2:
		stateInfo->localGameInfo->pRAI.throwGoingToBase = 2;
		throwDirection.x = stateInfo->fieldPositions->secondBase.x - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.x;
		throwDirection.z = stateInfo->fieldPositions->secondBase.z - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.z;
		break;
	case 3:
		stateInfo->localGameInfo->pRAI.throwGoingToBase = 3;
		throwDirection.x = stateInfo->fieldPositions->thirdBase.x - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.x;
		throwDirection.z = stateInfo->fieldPositions->thirdBase.z - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.z;
		break;
	}
}

void genericThrowRelease(StateInfo* stateInfo)
{
	if(stateInfo->localGameInfo->pII.hasBallIndex != -1) {
		float power;
		// throw not going anymore, ball already flyin'
		throwGoingOn = 0;
		// release animation
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.model = 9;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStage = 0;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStageCount = 21;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationFrequency = 2;
		// set flag to indicate that animation is still going on ( so no extra movement
		// until its over ).
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cTPI.throwRecoil = 1;

		// take power naturally from meterCounter value
		power = 1.0f*meterCounter / meterCounterMax;
		// update these values a bit
		throwDirection.x = throwDirection.x / throwDistance;
		throwDirection.z = throwDirection.z / throwDistance;
		throwDirection.y = 0.06f;
		// ... and then edit them a bit more and send them to genericSlingBall.
		genericSlingBall(stateInfo->localGameInfo, throwDirection.x*power*THROW_POWER_CONSTANT, throwDirection.y + throwDistance*THROW_DISTANCE_CONSTANT, throwDirection.z*power*THROW_POWER_CONSTANT);
		// set lastHadBallIndex, its used for example to prevent this player of catching
		// the ball right after throwing.
		stateInfo->localGameInfo->pII.lastHadBallIndex = stateInfo->localGameInfo->pII.hasBallIndex;
		// no player has ball anymore
		stateInfo->localGameInfo->pII.hasBallIndex = -1;
		// set running flag to 0 so that orientation will change
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.running = 0;
		// set control to -1 and changePlayer to 0 as a precaution so that the player
		// wouldnt be changed right away after this, as the key
		// to do this is the same one. let the genericSlingBall handle
		// player changing.
		stateInfo->localGameInfo->pII.controlIndex = -1;
		stateInfo->localGameInfo->aF.cTAF.changePlayer = 0;
	}
}

void genericThrowLoad(StateInfo* stateInfo, int base)
{
	if(stateInfo->localGameInfo->pII.hasBallIndex != -1) {
		// throw distance is the euclidean distance from the base to player throwing.
		throwDistance = (float)sqrt(throwDirection.x*throwDirection.x + throwDirection.z*throwDirection.z);
		// if player is already on the base, cant throw.
		if(throwDistance > THROW_TO_BASE_DISTANCE) {
			// stop player if he is moving, moving won't look good as the animation
			// doesn't have foot movement
			if(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.moving == 1) {
				stopMovement(stateInfo->localGameInfo, stateInfo->localGameInfo->pII.hasBallIndex);
			}
			// set the animation
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.model = 8;
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStage = 0;
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStageCount = 11;
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationFrequency = 3;
			// initialize meters.
			meterCounter = 0;
			meterCounterMax = THROW_MAX; // arbitrary decision, seems about right though
			// continue to next phase
			stateInfo->localGameInfo->aF.cTAF.throwToBase[base] = 2;
			// set the flag that is used for example to determine can you move the player.
			throwGoingOn = 1;
			// to avoid twitching when moving key is still pressed and player cant move as hes throwing
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.lastLastLocationUpdate = 1;
			// and orient player to look at the base too.
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.orientation.x = throwDirection.x;
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.orientation.z = throwDirection.z;
		} else {
			// if too close to base, terminate throwing.
			stateInfo->localGameInfo->aF.cTAF.throwToBase[base] = 0;
			stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 0;
			throwGoingOn = 0;
			stateInfo->localGameInfo->pRAI.throwGoingToBase = -1;
		}
	}
}

void genericMove(StateInfo* stateInfo, int direction)
{
	// we can move if there is no throw going on and no pitch going on
	// .. and we have same player controlled
	if(throwGoingOn == 0 && stateInfo->localGameInfo->pRAI.pitchGoingOn == 0 &&
	        stateInfo->localGameInfo->pII.controlIndex != -1) {
		// stopping only possible when moving already going on
		// so thats the reason for this value 2
		stateInfo->localGameInfo->aF.cTAF.move[direction] = 2;

		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cTPI.movesToDirection[direction] = 1;
		// and we call this generic function that utilizes this movesToDirection to select
		// velocity and orientation for the player
		updateControlledPlayerSpeed(stateInfo);
	} else {
		stateInfo->localGameInfo->aF.cTAF.move[direction] = 0;
	}

}

void genericStopMove(StateInfo* stateInfo, int direction)
{
	// stopping cant be done either when pitching or throwing as updateControlledPlayerSpeed can
	// have effects on player's model
	if(throwGoingOn == 0 && stateInfo->localGameInfo->pRAI.pitchGoingOn == 0 &&
	        stateInfo->localGameInfo->pII.controlIndex != -1) {
		stateInfo->localGameInfo->aF.cTAF.move[direction] = 0;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cTPI.movesToDirection[direction] = 0;
		updateControlledPlayerSpeed(stateInfo);
	} else {
		stateInfo->localGameInfo->aF.cTAF.move[direction] = 0;
	}
}

void dropBall(StateInfo* stateInfo)
{
	// there is a possibility to drop ball if to the ground if you want. it could be convenient when
	// you want a baserunner to be able to get safe from a base for some strategical reason.
	if(stateInfo->localGameInfo->pII.hasBallIndex != -1) {
		if(throwGoingOn == 0 && stateInfo->localGameInfo->pRAI.pitchGoingOn == 0) {
			float norm;
			float dx;
			float dz;

			// players' movement will be stopped when doing this, similar to throwing.
			if(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.moving == 1) {
				stopMovement(stateInfo->localGameInfo, stateInfo->localGameInfo->pII.hasBallIndex);
			}
			// model is set to be the basic standing without ball model.
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.model = 0;
			// and then just set a little upward-forward -directed value for ball so that we'll see the dropping
			dx = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.orientation.x;
			dz = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.orientation.z;
			norm = (float)sqrt(dx*dx+dz*dz);
			if( norm < EPSILON ) norm = 1.0f;
			dx = dx / norm;
			dz = dz / norm;
			// and use genericSlingBall again to get the ball to the world.
			genericSlingBall(stateInfo->localGameInfo, dx*DROP_BALL_CONSTANT, DROP_BALL_CONSTANT, dz*DROP_BALL_CONSTANT);
			// and set the lastHadBallIndex so that this player cannot catch it before it hits ground
			stateInfo->localGameInfo->pII.lastHadBallIndex = stateInfo->localGameInfo->pII.hasBallIndex;
			// and no player has the ball anymore.
			stateInfo->localGameInfo->pII.hasBallIndex = -1;
		}
	}
	stateInfo->localGameInfo->aF.cTAF.dropBall = 0;
	stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 0;
}

void updateControlledPlayerSpeed(StateInfo* stateInfo)
{
	if(stateInfo->localGameInfo->pII.controlIndex != -1) {
		// cant move when throw recoil going on.
		if(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cTPI.throwRecoil == 0) {
			float norm;
			// we select the direction by taking the difference of moves in x direction and moves in z direction
			// moves are 0 or 1, so as a net result we will get the direction where the player really should be going on
			int directionX = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cTPI.movesToDirection[1] -
			                 stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cTPI.movesToDirection[3];
			int directionZ = - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cTPI.movesToDirection[0] +
			                 stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cTPI.movesToDirection[2];
			// always when player's velocity changes, ball's velocity must change too.
			stateInfo->localGameInfo->ballInfo.needsMoveUpdate = 1;
			// if every component vanishes
			if(directionX*directionX + directionZ*directionZ == 0) {
				// set moving to zero
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.moving = 0;
				// if controlled player has also ball, set corresponding model
				// otherwise set model without ball
				if(stateInfo->localGameInfo->pII.hasBallIndex == stateInfo->localGameInfo->pII.controlIndex)
					stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.model = 1;
				else
					stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.model = 0;
				// when stopping movement, need to update last location.
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.lastLastLocationUpdate = 1;
			} else {
				// if there is a non-zero component in x or z direction
				// moving is to be 1 and we are going to have walking or running animation
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.moving = 1;
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.animationFrequency = 3;
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.animationStage = 0;
				// set player's orientation so that player faces the direction he's moving to
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].tPI.orientation.x = (float)directionX;
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].tPI.orientation.z = (float)directionZ;

				// Find norm
				norm = (float)sqrt(directionX*directionX + directionZ*directionZ);
				if (norm < EPSILON) norm = 1.0f;

				// running
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].tPI.velocity.x = (float)directionX*RUN_SPEED/norm;
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].tPI.velocity.z = (float)directionZ*RUN_SPEED/norm;
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.animationStageCount = 20;
				// if has ball, then running with ball model, otherwise running without ball
				if(stateInfo->localGameInfo->pII.hasBallIndex == stateInfo->localGameInfo->pII.controlIndex) {
					stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.model = 5;

				} else {
					stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.model = 4;
				}
			}
		}
	}
}
