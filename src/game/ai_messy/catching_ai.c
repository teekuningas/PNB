#include "ai_messy/catching_ai.h"
#include "action_implementation.h"
#include "actions_messy/throwing_system.h"
#include "actions_messy/pitching_system.h"
#include "common_logic.h"
#include "vector_math.h"
#include "catching_ai_strategy.h"
#include "rng.h"
#include "base_logic.h"

int aiMoveCounter = 0;
int aiThrowStage = 0;
int aiDropStage = 0;

void initCatchingAI(void)
{
	aiDropStage = 0;
	aiThrowStage = 0;
	aiMoveCounter = 0;
}

// we move towards the target position by simulating key presses.
void moveControlledPlayerToLocation(StateInfo* stateInfo, Vector3D* target)
{
	float px = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].tPI.location.x;
	float pz = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].tPI.location.z;
	float tx = target->x;
	float tz = target->z;
	float dx = tx - px;
	float dz = tz - pz;

	if(!isVectorSmallEnoughCircleXZ(dx, dz, 1.0f)) {
		if(aiMoveCounter >= 10) {
			flushKeys(stateInfo);
			MovementKeys keys = calculate_movement_keys(dx, dz);
			if (keys.up) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 1;
			if (keys.down) stateInfo->keyStates->imitateKeyPress[KEY_DOWN] = 1;
			if (keys.left) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 1;
			if (keys.right) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 1;
			aiMoveCounter = 0;
		}
	} else {
		flushKeys(stateInfo);
		aiMoveCounter = 0;
	}
	aiMoveCounter++;
}

void throwBallToBase(StateInfo* stateInfo, int base)
{
	if(aiThrowStage == 0) {
		if(aiActionEventLock == AI_NO_LOCK && aiLockUpdate == 0) {
			int hasBallIndex = stateInfo->localGameInfo->pII.hasBallIndex;
			int catcherIndex = stateInfo->localGameInfo->pII.catcherOnBaseIndex[base];
			int catcherNearHome = 0;
			if(catcherIndex != -1) {
				catcherNearHome = stateInfo->localGameInfo->playerInfo[catcherIndex].cTPI.isNearHomeLocation;
			}

			int replacerIndex = stateInfo->localGameInfo->pII.catcherReplacerOnBaseIndex[base];
			int replacerStage = 0;
			int replacerBase = -1;
			int replacerMoving = 0;
			if(replacerIndex != -1) {
				replacerStage = stateInfo->localGameInfo->playerInfo[replacerIndex].cTPI.replacingStage;
				replacerBase = stateInfo->localGameInfo->playerInfo[replacerIndex].cTPI.replacingBase;
				replacerMoving = stateInfo->localGameInfo->playerInfo[replacerIndex].cPI.moving;
			}

			int shouldThrow = should_ai_throw(hasBallIndex, catcherIndex, catcherNearHome,
			                                  replacerIndex, replacerStage, replacerBase, replacerMoving,
			                                  base);

			if(shouldThrow == 1) {
				aiThrowStage = 1;
				aiLockUpdate = 1;
				aiActionEventLock = AI_THROW_LOCK;
				flushKeys(stateInfo);
				if(base == 0) {
					stateInfo->keyStates->imitateKeyPress[KEY_DOWN] = 1;
				} else if(base == 1) {
					stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 1;
				} else if(base == 2) {
					stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 1;
				} else if(base == 3) {
					stateInfo->keyStates->imitateKeyPress[KEY_UP] = 1;
				}
				stateInfo->keyStates->imitateKeyPress[KEY_2] = 1;
			}
		}
	}
}

void updateCatchingAI(StateInfo* stateInfo, unsigned int* rng_seed)
{
	// Update AI pitching
	updateAIPitching(stateInfo, rng_seed);

	// finish dropping
	if(aiDropStage == 1) {
		flushKeys(stateInfo);
		aiActionEventLock = AI_NO_LOCK;
		aiDropStage = 0;
		aiLockUpdate = 1;
	}
	// finish throwing
	if(aiThrowStage == 1) {
		if(aiLockTimeoutCounter == -1) {
			aiLockTimeoutCounter = 0;
		}
		if(meterCounter > THROW_MAX*(3.0f/4)) {
			flushKeys(stateInfo);
			aiThrowStage = 0;
			aiActionEventLock = AI_NO_LOCK;
			aiLockUpdate = 1;
			aiLockTimeoutCounter = -1;
		} else {
			aiLockTimeoutCounter++;
			if(aiLockTimeoutCounter > TIMEOUT_CONSTANT) {
				aiThrowStage = 0;
				flushKeys(stateInfo);
				aiActionEventLock = AI_NO_LOCK;
				aiLockUpdate = 1;
				aiLockTimeoutCounter = -1;
			}
		}
	}
	// if noone has ball and someone is controlled, ai will try to move towards the target point calculated
	// in game_manipulation.
	if(stateInfo->localGameInfo->pII.hasBallIndex == -1 && stateInfo->localGameInfo->pII.controlIndex != -1) {
		if(aiActionEventLock == AI_NO_LOCK && aiLockUpdate == 0) {
			if(stateInfo->localGameInfo->pRAI.throwGoingToBase == -1 || stateInfo->localGameInfo->
			        ballInfo.hasHitGround == 1) {
				moveControlledPlayerToLocation(stateInfo, &(stateInfo->localGameInfo->cameraState.targetPoint));
			}
		}

	}
	// if someone has ball
	if(stateInfo->localGameInfo->pII.hasBallIndex != -1) {
		int index3 = stateInfo->localGameInfo->pII.safeOnBaseIndex[3];
		int index2 = stateInfo->localGameInfo->pII.safeOnBaseIndex[2];

		int r3OriginalBase = -1;
		int r3IsOnBase = 0;
		if (index3 != -1) {
			r3OriginalBase = stateInfo->localGameInfo->playerInfo[index3].bTPI.originalBase;
			r3IsOnBase = (stateInfo->localGameInfo->playerInfo[index3].bTPI.state == PLAYER_STATE_SAFE_ON_BASE);
		}

		int r2OriginalBase = -1;
		int r2IsOnBase = 0;
		if (index2 != -1) {
			r2OriginalBase = stateInfo->localGameInfo->playerInfo[index2].bTPI.originalBase;
			r2IsOnBase = (stateInfo->localGameInfo->playerInfo[index2].bTPI.state == PLAYER_STATE_SAFE_ON_BASE);
		}

		int catcherHomeIndex = stateInfo->localGameInfo->pII.catcherOnBaseIndex[0];
		int hasBallIndex = stateInfo->localGameInfo->pII.hasBallIndex;

		if(should_ai_drop_ball(stateInfo->localGameInfo->woundingState.woundingCatch,
		                       stateInfo->localGameInfo->gameControl.batterStartedRunning,
		                       r3OriginalBase, r3IsOnBase,
		                       r2OriginalBase, r2IsOnBase,
		                       catcherHomeIndex, hasBallIndex)) {
			if(aiActionEventLock == AI_NO_LOCK && aiLockUpdate == 0) {
				aiDropStage = 1;
				aiLockUpdate = 1;
				aiActionEventLock = AI_DROP_LOCK;
				flushKeys(stateInfo);
				stateInfo->keyStates->imitateKeyPress[KEY_2] = 1;
			}
		}
		// otherwise we throw or move towards a base where lead player is going. if lead player is going nowhere
		// we take ball to home base.
		else {
			int leadBase = -1;
			int throwBase = 0;

			CatchingRunnerInfo runners[BASE_COUNT];
			int runnerCount = 0;
			int i;
			for(i = 0; i < BASE_COUNT; i++) {
				int index = stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[i];
				if(index != -1) {
					PlayerUnitState s = stateInfo->localGameInfo->playerInfo[index].bTPI.state;
					runners[runnerCount].isOnBase = (s == PLAYER_STATE_SAFE_ON_BASE || s == PLAYER_STATE_AT_BAT);
					runners[runnerCount].takingFreeWalk = (s == PLAYER_STATE_ADVANCING_FREELY);
					runners[runnerCount].base = base_to_int_index(stateInfo->localGameInfo->playerInfo[index].bTPI.baseId);
					if (stateInfo->localGameInfo->playerInfo[index].bTPI.baseId == BASE_HOME_SCORED) {
						runners[runnerCount].base = 4;
					}
					runners[runnerCount].leading = (s == PLAYER_STATE_LEADING);
					runnerCount++;
				}
			}

			int randomVal = seeded_rand(rng_seed, 500);
			leadBase = determine_lead_base(runners, runnerCount, randomVal);

			if(leadBase > -1 && leadBase < 3) throwBase = leadBase + 1;
			else throwBase = 0;

			if(aiActionEventLock == AI_NO_LOCK && aiLockUpdate == 0) {
				Vector3D target;
				target.x = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->
				           pII.catcherOnBaseIndex[throwBase]].tPI.homeLocation.x;
				target.z = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->
				           pII.catcherOnBaseIndex[throwBase]].tPI.homeLocation.z;
				moveControlledPlayerToLocation(stateInfo, &target);
			}
			throwBallToBase(stateInfo, throwBase);
		}
	}
	if(aiLockUpdate == 1) {
		aiLockUpdate = 0;
	}
}
