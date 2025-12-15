#include "ai_messy/catching_ai.h"
#include "action_implementation.h"
#include "actions_messy/throwing_system.h"
#include "actions_messy/pitching_system.h"
#include "common_logic.h"
#include "vector_math.h"

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
	if(!isVectorSmallEnoughCircleXZ((px-tx), (pz-tz), 1.0f)) {
		if(aiMoveCounter >= 10) {
			float angle = (float)atan2(-(tz-pz), (tx-px));
			flushKeys(stateInfo);
			if(angle > 7*PI/8 || angle <= -7*PI/8) {
				stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 1;
			} else if(angle <= 7*PI/8 && angle > 5*PI/8) {
				stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 1;
				stateInfo->keyStates->imitateKeyPress[KEY_UP] = 1;
			} else if(angle <= 5*PI/8 && angle > 3*PI/8) {
				stateInfo->keyStates->imitateKeyPress[KEY_UP] = 1;
			} else if(angle <= 3*PI/8 && angle > PI/8) {
				stateInfo->keyStates->imitateKeyPress[KEY_UP] = 1;
				stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 1;
			} else if(angle <= PI/8 && angle > -PI/8) {
				stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 1;
			} else if(angle <= -PI/8 && angle > -3*PI/8) {
				stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 1;
				stateInfo->keyStates->imitateKeyPress[KEY_DOWN] = 1;
			} else if(angle <= -3*PI/8 && angle > -5*PI/8) {
				stateInfo->keyStates->imitateKeyPress[KEY_DOWN] = 1;
			} else if(angle <= -5*PI/8 && angle > -7*PI/8) {
				stateInfo->keyStates->imitateKeyPress[KEY_DOWN] = 1;
				stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 1;
			}
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
			int shouldThrow = 0;
			// we can throw if there is a normal catcher or a replacer on that base.
			if(stateInfo->localGameInfo->pII.hasBallIndex != stateInfo->localGameInfo->pII.catcherOnBaseIndex[base]) {
				if(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.catcherOnBaseIndex[base]].cTPI.
				        isNearHomeLocation == 1) {
					shouldThrow = 1;
				}
			}
			if(stateInfo->localGameInfo->pII.hasBallIndex != stateInfo->localGameInfo->pII.catcherReplacerOnBaseIndex[base]) {
				if(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.catcherReplacerOnBaseIndex[base]].cTPI.replacingStage == 1 &&
				        stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.catcherReplacerOnBaseIndex[base]].cTPI.replacingBase == base &&
				        stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.catcherReplacerOnBaseIndex[base]].cPI.moving == 0) {
					shouldThrow = 1;
				}
			}
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

void updateCatchingAI(StateInfo* stateInfo)
{
	// Update AI pitching
	updateAIPitching(stateInfo);

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
				moveControlledPlayerToLocation(stateInfo, &(stateInfo->localGameInfo->gAI.targetPoint));
			}
		}

	}
	// if someone has ball
	if(stateInfo->localGameInfo->pII.hasBallIndex != -1) {
		int index3 = stateInfo->localGameInfo->pII.safeOnBaseIndex[3];
		int index2 = stateInfo->localGameInfo->pII.safeOnBaseIndex[2];
		// if we have this cool event of having player on second and third base and batter running and situation
		// being that ball has just been catched, we drop the ball to let first player come to the base.
		if(stateInfo->localGameInfo->gAI.woundingCatch == 1 && stateInfo->localGameInfo->gAI.batterStartedRunning == 1 &&
		        index3 != -1 && stateInfo->localGameInfo->playerInfo[index3].bTPI.originalBase == 3 &&
		        stateInfo->localGameInfo->playerInfo[index3].bTPI.isOnBase == 1 &&
		        index2 != -1 && stateInfo->localGameInfo->playerInfo[index2].bTPI.originalBase == 2 &&
		        stateInfo->localGameInfo->playerInfo[index2].bTPI.isOnBase == 1 &&
		        stateInfo->localGameInfo->pII.catcherOnBaseIndex[0] != stateInfo->localGameInfo->pII.hasBallIndex) {
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
			int i;
			for(i = 0; i < BASE_COUNT; i++) {
				int index = stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[i];
				if(index != -1) {
					if(stateInfo->localGameInfo->playerInfo[index].bTPI.isOnBase == 0 &&
					        stateInfo->localGameInfo->playerInfo[index].bTPI.takingFreeWalk == 0) {
						if(stateInfo->localGameInfo->playerInfo[index].bTPI.base > leadBase) {
							if(stateInfo->localGameInfo->playerInfo[index].bTPI.leading == 0) {
								leadBase = stateInfo->localGameInfo->playerInfo[index].bTPI.base;
							} else {
								int random = rand()%500;
								if(random == 0) {
									leadBase = stateInfo->localGameInfo->playerInfo[index].bTPI.base - 1;
								}
							}
						}
					}
				}
			}
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
