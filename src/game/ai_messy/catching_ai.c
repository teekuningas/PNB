#include "ai_messy/catching_ai.h"
#include "action_implementation.h"
#include "actions_messy/throwing_system.h"
#include "common_logic.h"
#include "vector_math.h"

int aiMoveCounter = 0;
int aiThrowStage = 0;

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
