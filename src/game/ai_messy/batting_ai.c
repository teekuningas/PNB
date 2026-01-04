#include <math.h>
#include <stdlib.h>

#include "globals.h"
#include "batting_ai.h"
#include "batting_ai_strategy.h"
#include "action_implementation.h" // for flushKeys
#include "actions_messy/batting_system.h"
#include "game_manipulation.h"
#include "rng.h"
#include "base_logic.h"

#define CLICK_BREAK_CONSTANT 3

void initBattingAI(StateInfo* stateInfo)
{
	int i;
	stateInfo->localGameInfo->aiState.battingKeyDown = 0;
	stateInfo->localGameInfo->aiState.changingKeyDown = 0;
	stateInfo->localGameInfo->aiState.actionKeyLock = AI_NO_LOCK;
	stateInfo->localGameInfo->aiState.battingStyle = 0;
	stateInfo->localGameInfo->aiState.runningBatter = 0;
	stateInfo->localGameInfo->aiState.runningBaseRunners = 0;

	stateInfo->localGameInfo->aiState.increaseKeyDown = 0;
	stateInfo->localGameInfo->aiState.decreaseKeyDown = 0;
	stateInfo->localGameInfo->aiState.angleDecided = 0;
	stateInfo->localGameInfo->aiState.decidedAngle = 0.0f;
	stateInfo->localGameInfo->pendingActionState.aiWrongPitch = 0;
	stateInfo->localGameInfo->aiState.planCalculated = 0;
	stateInfo->localGameInfo->aiState.firstIndex = -1;
	stateInfo->localGameInfo->aiState.firstIndexSelected = 0;
	stateInfo->localGameInfo->aiState.change = 0;
	stateInfo->localGameInfo->aiState.changeHasHappened = 0;
	for(i = 0; i < BASE_COUNT; i++) {
		stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] = 0;
		stateInfo->localGameInfo->aiState.lastSafeOnBaseIndex[i] = -1;
		stateInfo->localGameInfo->aiState.baseRunnerDecisionMade[i] = 0;
		stateInfo->localGameInfo->aiState.amountOfClicks[i] = 0;
		stateInfo->localGameInfo->aiState.baseRunnerLock[i] = AI_NO_LOCK;
		stateInfo->localGameInfo->aiState.clickBreak[i] = 0;
	}
}

void updateBattingAI(StateInfo* stateInfo, unsigned int* rng_seed)
{
	int i;
	int isDoubleClickingOk = 0;
	// update some flags
	for(i = 0; i < BASE_COUNT; i++) {
		stateInfo->localGameInfo->aiState.clickBreak[i]++;
		if(stateInfo->localGameInfo->aiState.clickBreak[i] > 1000) stateInfo->localGameInfo->aiState.clickBreak[i] = 0;
		if(stateInfo->localGameInfo->aiState.baseRunnerDecisionMade[i] == 1) {
			if(stateInfo->localGameInfo->pII.safeOnBaseIndex[i] == -1 ) {
				stateInfo->localGameInfo->aiState.baseRunnerDecisionMade[i] = 0;
			}
			if(stateInfo->localGameInfo->aiState.lastSafeOnBaseIndex[i] != stateInfo->localGameInfo->pII.safeOnBaseIndex[i]) {
				stateInfo->localGameInfo->aiState.baseRunnerDecisionMade[i] = 0;
			}
		}
		stateInfo->localGameInfo->aiState.lastSafeOnBaseIndex[i] = stateInfo->localGameInfo->pII.safeOnBaseIndex[i];
	}
	if(stateInfo->localGameInfo->pRAI.batterReady == 0 && stateInfo->localGameInfo->aiState.planCalculated == 1) {
		stateInfo->localGameInfo->aiState.planCalculated = 0;
	}
	// make free walk decision == accept
	if(stateInfo->localGameInfo->gameControl.waitingForFreeWalkDecision == 1) {
		if(stateInfo->localGameInfo->aiState.battingKeyDown == 0) {
			if(stateInfo->localGameInfo->aiState.actionKeyLock == AI_NO_LOCK) {
				stateInfo->keyStates->imitateKeyPress[KEY_2] = 1;
				stateInfo->localGameInfo->aiState.battingKeyDown = 1;
				stateInfo->localGameInfo->aiState.actionKeyLock = AI_WAITING_WALK_LOCK;
			}
		} else {
			stateInfo->keyStates->imitateKeyPress[KEY_2] = 0;
			stateInfo->localGameInfo->aiState.actionKeyLock = AI_NO_LOCK;
			stateInfo->localGameInfo->aiState.battingKeyDown = 0;
		}
	}
	// we decide batter only after ball is at home so that in normal situation ai will have more information
	// to make its strategy decisions
	if(stateInfo->localGameInfo->gameControl.waitingForBatterDecision == 1 && stateInfo->localGameInfo->gameState.ballHome == 1) {
		// we do this by brute force, we change player until we find a fit one or we are back to non joker.
		// plan is that if there is a man on first base and current batter would not have a great power,
		// we would try to find a joker that has power instead.
		// and if field is empty we would change a joker with speed instead.
		int firstBaseIndex = stateInfo->localGameInfo->pII.safeOnBaseIndex[1];
		int secondBaseIndex = stateInfo->localGameInfo->pII.safeOnBaseIndex[2];
		int thirdBaseIndex = stateInfo->localGameInfo->pII.safeOnBaseIndex[3];
		int fieldStatus;
		int index = stateInfo->localGameInfo->pII.batterSelectionIndex;

		if(firstBaseIndex != -1) fieldStatus = 2;
		else if(secondBaseIndex != -1 || thirdBaseIndex != -1) fieldStatus = 1;
		else fieldStatus = 0;

		stateInfo->localGameInfo->aiState.change = should_change_batter(
		        fieldStatus,
		        stateInfo->localGameInfo->playerInfo[index].bTPI.power,
		        stateInfo->localGameInfo->playerInfo[index].bTPI.speed
		    );

		if(stateInfo->localGameInfo->aiState.firstIndexSelected == 0) {
			stateInfo->localGameInfo->aiState.firstIndex = index;
			stateInfo->localGameInfo->aiState.firstIndexSelected = 1;
		} else if(stateInfo->localGameInfo->aiState.changeHasHappened == 1) {
			if(stateInfo->localGameInfo->aiState.firstIndex == index) {
				stateInfo->localGameInfo->aiState.change = 0;
			}
		}

		// change player
		if(stateInfo->localGameInfo->aiState.change == 1 && stateInfo->localGameInfo->aiState.changingKeyDown == 0 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_NO_LOCK) {
			stateInfo->keyStates->imitateKeyPress[KEY_1] = 1;
			stateInfo->localGameInfo->aiState.changingKeyDown = 1;
			stateInfo->localGameInfo->aiState.actionKeyLock = AI_CHANGE_LOCK;
		} else if(stateInfo->localGameInfo->aiState.changingKeyDown == 1 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_CHANGE_LOCK) {
			stateInfo->keyStates->imitateKeyPress[KEY_1] = 0;
			stateInfo->localGameInfo->aiState.actionKeyLock = AI_NO_LOCK;
			stateInfo->localGameInfo->aiState.changingKeyDown = 0;
			stateInfo->localGameInfo->aiState.changeHasHappened = 1;

		}
		// select best batter.
		if(stateInfo->localGameInfo->aiState.change == 0 && stateInfo->localGameInfo->aiState.battingKeyDown == 0 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_NO_LOCK) {
			stateInfo->keyStates->imitateKeyPress[KEY_2] = 1;
			stateInfo->localGameInfo->aiState.battingKeyDown = 1;
			stateInfo->localGameInfo->aiState.actionKeyLock = AI_WAITING_BATTER_LOCK;
		} else if(stateInfo->localGameInfo->aiState.battingKeyDown == 1 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_WAITING_BATTER_LOCK) {
			stateInfo->keyStates->imitateKeyPress[KEY_2] = 0;
			stateInfo->localGameInfo->aiState.actionKeyLock = AI_NO_LOCK;
			stateInfo->localGameInfo->aiState.battingKeyDown = 0;
			stateInfo->localGameInfo->aiState.firstIndex = -1;
			stateInfo->localGameInfo->aiState.firstIndexSelected = 0;
			stateInfo->localGameInfo->aiState.changeHasHappened = 0;

		}

	} else if(stateInfo->localGameInfo->pRAI.batterReady == 1 && stateInfo->localGameInfo->pRAI.pitchState != PITCH_STAGE_AIRBORNE && stateInfo->localGameInfo->gameState.ballHome == 1) {
		// decision tree.. contents can be read within
		if(stateInfo->localGameInfo->aiState.planCalculated == 0) {
			int batterIndex = stateInfo->localGameInfo->pII.batterIndex;
			int firstBaseIndex = stateInfo->localGameInfo->pII.safeOnBaseIndex[1];
			int secondBaseIndex = stateInfo->localGameInfo->pII.safeOnBaseIndex[2];
			int thirdBaseIndex = stateInfo->localGameInfo->pII.safeOnBaseIndex[3];
			int power = stateInfo->localGameInfo->playerInfo[batterIndex].bTPI.power;
			int speed = stateInfo->localGameInfo->playerInfo[batterIndex].bTPI.speed;
			int fieldStatus;

			if(firstBaseIndex != -1) fieldStatus = 2;
			else if(secondBaseIndex != -1 || thirdBaseIndex != -1) fieldStatus = 1;
			else fieldStatus = 0;

			BattingStrategy strategy = calculate_batting_strategy(
			                               &(stateInfo->localGameInfo->gameState),
			                               fieldStatus,
			                               power,
			                               speed,
			                               stateInfo->globalGameInfo->period
			                           );

			stateInfo->localGameInfo->aiState.battingStyle = strategy.style;
			stateInfo->localGameInfo->aiState.runningBaseRunners = strategy.runBaseRunners;
			stateInfo->localGameInfo->aiState.runningBatter = strategy.runBatter;

			stateInfo->localGameInfo->aiState.planCalculated = 1;
		}
		// if we decide that batter should run, we click down once.
		if(stateInfo->localGameInfo->aiState.runningBatter == 1) {
			if(stateInfo->localGameInfo->aiState.baseRunnerDecisionMade[0] == 0 && stateInfo->localGameInfo->aiState.baseRunnerKeyDown[0] == 0 && stateInfo->localGameInfo->aiState.baseRunnerLock[0] == AI_NO_LOCK &&
			        stateInfo->localGameInfo->aiState.clickBreak[0] > CLICK_BREAK_CONSTANT) {
				stateInfo->localGameInfo->aiState.baseRunnerKeyDown[0] = 1;
				stateInfo->localGameInfo->aiState.baseRunnerLock[0] = AI_CLICK_LOCK;
				stateInfo->keyStates->imitateKeyPress[KEY_DOWN] = 1;
			} else if(stateInfo->localGameInfo->aiState.baseRunnerKeyDown[0] == 1 && stateInfo->localGameInfo->aiState.baseRunnerLock[0] == AI_CLICK_LOCK) {
				stateInfo->localGameInfo->aiState.baseRunnerKeyDown[0] = 0;
				stateInfo->localGameInfo->aiState.baseRunnerDecisionMade[0] = 1;
				stateInfo->localGameInfo->aiState.clickBreak[0] = 0;
				stateInfo->keyStates->imitateKeyPress[KEY_DOWN] = 0;
				stateInfo->localGameInfo->aiState.baseRunnerLock[0] = AI_NO_LOCK;
			}
		}
		// if decide that baserunners should run, we click their keys.
		if(stateInfo->localGameInfo->aiState.runningBaseRunners == 1) {
			int i;
			for(i = 1; i < BASE_COUNT; i++) {
				if(stateInfo->localGameInfo->aiState.baseRunnerDecisionMade[i] == 0 && stateInfo->localGameInfo->pII.safeOnBaseIndex[i] != -1 &&
				        stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.safeOnBaseIndex[i]].bTPI.state == PLAYER_STATE_SAFE_ON_BASE &&
				        stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] == 0 && stateInfo->localGameInfo->aiState.baseRunnerLock[i] == AI_NO_LOCK && stateInfo->localGameInfo->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
					stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] = 1;
					stateInfo->localGameInfo->aiState.baseRunnerLock[i] = AI_CLICK_LOCK;
					if(i == 1) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 1;
					else if(i == 2) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 1;
					else if(i == 3) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 1;
				} else if(stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] == 1 && stateInfo->localGameInfo->aiState.baseRunnerLock[i] == AI_CLICK_LOCK) {
					stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] = 0;
					stateInfo->localGameInfo->aiState.baseRunnerLock[i] = AI_NO_LOCK;
					stateInfo->localGameInfo->aiState.baseRunnerDecisionMade[i] = 1;
					stateInfo->localGameInfo->aiState.clickBreak[i] = 0;
					if(i == 1) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 0;
					else if(i == 2) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 0;
					else if(i == 3) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 0;
				}
			}
		}
	}
	// if ball is not home, we return players from first and second base to their bases
	else if(stateInfo->localGameInfo->pRAI.batterReady == 1 && stateInfo->localGameInfo->pRAI.pitchState != PITCH_STAGE_AIRBORNE && stateInfo->localGameInfo->gameState.ballHome == 0) {
		if(stateInfo->localGameInfo->aiState.runningBaseRunners == 1) {
			int i;
			for(i = 1; i < 3; i++) {
				if(stateInfo->localGameInfo->pII.safeOnBaseIndex[i] != -1 &&
				        stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.safeOnBaseIndex[i]].bTPI.state == PLAYER_STATE_LEADING &&
				        stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] == 0 && stateInfo->localGameInfo->aiState.baseRunnerLock[i] == AI_NO_LOCK && stateInfo->localGameInfo->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
					stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] = 1;
					stateInfo->localGameInfo->aiState.baseRunnerLock[i] = AI_COME_BACK_LOCK;
					if(i == 1) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 1;
					else if(i == 2) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 1;
					else if(i == 3) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 1;
				} else if(stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] == 1 && stateInfo->localGameInfo->aiState.baseRunnerLock[i] == AI_COME_BACK_LOCK) {
					stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] = 0;
					stateInfo->localGameInfo->aiState.baseRunnerDecisionMade[i] = 0;
					stateInfo->localGameInfo->aiState.baseRunnerLock[i] = AI_NO_LOCK;
					stateInfo->localGameInfo->aiState.clickBreak[i] = 0;
					if(i == 1) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 0;
					else if(i == 2) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 0;
					else if(i == 3) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 0;
				}
			}
		}
	}
	// and here we bat
	else if(stateInfo->localGameInfo->pRAI.pitchState == PITCH_STAGE_AIRBORNE) {
		int i;
		// predict if pitch is going to be ball
		if(stateInfo->localGameInfo->pendingActionState.aiWrongPitch == 0 && is_wrong_pitch(
		            stateInfo->localGameInfo->ballInfo.velocity.x,
		            stateInfo->localGameInfo->ballInfo.velocity.y,
		            GRAVITY,
		            PLATE_WIDTH
		        )) {
			stateInfo->localGameInfo->pendingActionState.aiWrongPitch = 1;
		}
		if(stateInfo->localGameInfo->pendingActionState.aiWrongPitch == 1) {
			// batter isnt handled here
			// this code will make baserunners come back if wrong pitch is pitched
			for(i = 1; i < BASE_COUNT; i++) {
				int index = stateInfo->localGameInfo->pII.safeOnBaseIndex[i];
				if(index != -1 && stateInfo->localGameInfo->playerRuntime[index].goingForward == 1 && stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] == 0 &&
				        stateInfo->localGameInfo->aiState.baseRunnerLock[i] == AI_NO_LOCK && stateInfo->localGameInfo->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
					stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] = 1;
					stateInfo->localGameInfo->aiState.baseRunnerLock[i] = AI_COME_BACK_WRONG_PITCH_LOCK;
					if(i == 1) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 1;
					else if(i == 2) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 1;
					else if(i == 3) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 1;
				} else if(stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] == 1 && stateInfo->localGameInfo->aiState.baseRunnerLock[i] == AI_COME_BACK_WRONG_PITCH_LOCK) {
					stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] = 0;
					stateInfo->localGameInfo->aiState.baseRunnerDecisionMade[i] = 0;
					stateInfo->localGameInfo->aiState.baseRunnerLock[i] = AI_NO_LOCK;
					stateInfo->localGameInfo->aiState.clickBreak[i] = 0;
					if(i == 1) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 0;
					else if(i == 2) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 0;
					else if(i == 3) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 0;
				}
			}
		}
		// a bunt
		if(stateInfo->localGameInfo->aiState.battingStyle == 0) {
			if(stateInfo->localGameInfo->aiState.angleDecided == 0) {
				stateInfo->localGameInfo->aiState.decidedAngle = calculate_ai_batting_angle(0, -1, seeded_rand(rng_seed, RAND_MAX));
				stateInfo->localGameInfo->aiState.angleDecided = 1;
			}
			if(stateInfo->localGameInfo->pendingActionState.meterCounter > BAT_SWING_MAX - 23 && stateInfo->localGameInfo->aiState.battingKeyDown == 0 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_NO_LOCK && stateInfo->localGameInfo->pendingActionState.aiWrongPitch == 0) {
				stateInfo->keyStates->imitateKeyPress[KEY_2] = 1;
				stateInfo->localGameInfo->aiState.battingKeyDown = 1;
				stateInfo->localGameInfo->aiState.actionKeyLock = AI_BATTING_LOCK;
			} else if(stateInfo->localGameInfo->aiState.battingKeyDown == 1 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_BATTING_LOCK) {
				if(stateInfo->localGameInfo->pendingActionState.meterCounter > BAT_LOAD_MAX - 9) {
					stateInfo->keyStates->imitateKeyPress[KEY_2] = 0;
					stateInfo->localGameInfo->aiState.battingKeyDown = 0;
					stateInfo->localGameInfo->aiState.actionKeyLock = AI_NO_LOCK;
				}
			}
		}
		// a normal swing
		else if(stateInfo->localGameInfo->aiState.battingStyle == 1) {
			if(stateInfo->localGameInfo->aiState.angleDecided == 0) {
				int i;
				int leadBase = -1;
				for(i = 0; i < BASE_COUNT; i++) {
					int index = stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[i];
					if(index != -1 && stateInfo->localGameInfo->playerInfo[index].bTPI.state != PLAYER_STATE_WOUNDED) {
						BaseID currentBaseId = stateInfo->localGameInfo->playerInfo[index].bTPI.baseId;
						int currentBaseInt = base_to_int_index(currentBaseId);

						if(currentBaseInt != -1 && currentBaseInt > leadBase) {
							leadBase = currentBaseInt;
						}
					}
				}
				stateInfo->localGameInfo->aiState.decidedAngle = calculate_ai_batting_angle(1, leadBase, seeded_rand(rng_seed, RAND_MAX));
				stateInfo->localGameInfo->aiState.angleDecided = 1;
			}
			if(stateInfo->localGameInfo->pendingActionState.meterCounter > BAT_SWING_MAX - 10 && stateInfo->localGameInfo->aiState.battingKeyDown == 0 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_NO_LOCK && stateInfo->localGameInfo->pendingActionState.aiWrongPitch == 0) {
				stateInfo->keyStates->imitateKeyPress[KEY_2] = 1;
				stateInfo->localGameInfo->aiState.battingKeyDown = 1;
				stateInfo->localGameInfo->aiState.actionKeyLock = AI_BATTING_LOCK;
			} else if(stateInfo->localGameInfo->aiState.battingKeyDown == 1 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_BATTING_LOCK) {
				if(stateInfo->localGameInfo->pendingActionState.meterCounter > BAT_LOAD_MAX - 6) {
					stateInfo->keyStates->imitateKeyPress[KEY_2] = 0;
					stateInfo->localGameInfo->aiState.battingKeyDown = 0;
					stateInfo->localGameInfo->aiState.actionKeyLock = AI_NO_LOCK;
				}
			}
		}
		// swing that tries to get oneself wounded
		else if(stateInfo->localGameInfo->aiState.battingStyle == 2) {
			if(stateInfo->localGameInfo->aiState.angleDecided == 0) {
				stateInfo->localGameInfo->aiState.decidedAngle = calculate_ai_batting_angle(2, -1, seeded_rand(rng_seed, RAND_MAX));
				stateInfo->localGameInfo->aiState.angleDecided = 1;
			}
			if(stateInfo->localGameInfo->pendingActionState.meterCounter > BAT_SWING_MAX - 11 && stateInfo->localGameInfo->aiState.battingKeyDown == 0 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_NO_LOCK && stateInfo->localGameInfo->pendingActionState.aiWrongPitch == 0) {
				stateInfo->keyStates->imitateKeyPress[KEY_2] = 1;
				stateInfo->localGameInfo->aiState.battingKeyDown = 1;
				stateInfo->localGameInfo->aiState.actionKeyLock = AI_BATTING_LOCK;
			} else if(stateInfo->localGameInfo->aiState.battingKeyDown == 1 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_BATTING_LOCK) {
				if(stateInfo->localGameInfo->pendingActionState.meterCounter > BAT_LOAD_MAX - 8) {
					stateInfo->keyStates->imitateKeyPress[KEY_2] = 0;
					stateInfo->localGameInfo->aiState.battingKeyDown = 0;
					stateInfo->localGameInfo->aiState.actionKeyLock = AI_NO_LOCK;
				}
			}
		}
		if(stateInfo->localGameInfo->aiState.decidedAngle >= 0 && stateInfo->localGameInfo->pendingActionState.batterAngle < stateInfo->localGameInfo->aiState.decidedAngle && stateInfo->localGameInfo->aiState.increaseKeyDown == 0) {
			stateInfo->keyStates->imitateKeyPress[KEY_PLUS] = 1;
			stateInfo->localGameInfo->aiState.increaseKeyDown = 1;
		} else if(stateInfo->localGameInfo->pendingActionState.batterAngle >= stateInfo->localGameInfo->aiState.decidedAngle && stateInfo->localGameInfo->aiState.increaseKeyDown == 1) {
			stateInfo->keyStates->imitateKeyPress[KEY_PLUS] = 0;
			stateInfo->localGameInfo->aiState.increaseKeyDown = 0;
		}

		if(stateInfo->localGameInfo->aiState.decidedAngle < 0 && stateInfo->localGameInfo->pendingActionState.batterAngle > stateInfo->localGameInfo->aiState.decidedAngle && stateInfo->localGameInfo->aiState.decreaseKeyDown == 0) {
			stateInfo->keyStates->imitateKeyPress[KEY_MINUS] = 1;
			stateInfo->localGameInfo->aiState.decreaseKeyDown = 1;
		} else if(stateInfo->localGameInfo->pendingActionState.batterAngle <= stateInfo->localGameInfo->aiState.decidedAngle && stateInfo->localGameInfo->aiState.decreaseKeyDown == 1) {
			stateInfo->keyStates->imitateKeyPress[KEY_MINUS] = 0;
			stateInfo->localGameInfo->aiState.decreaseKeyDown = 0;
		}

	}
	if(stateInfo->localGameInfo->pRAI.pitchState != PITCH_STAGE_AIRBORNE && stateInfo->localGameInfo->aiState.angleDecided == 1) {
		stateInfo->localGameInfo->aiState.angleDecided = 0;
	}
	//here we check if ball is going somewhere out of bounds so that players can try to run towards next bases.
	if(stateInfo->localGameInfo->ballInfo.hasHitGroundOutOfBounds == 1 && stateInfo->localGameInfo->pRAI.batHit == 1 &&
	        stateInfo->localGameInfo->pRAI.throwGoingToBase == -1 && stateInfo->localGameInfo->pII.hasBallIndex == -1 &&
	        stateInfo->localGameInfo->ballInfo.moving == 1) {
		isDoubleClickingOk = 1;
	}
	// we will run with everyone so we need to simulate double click here.
	for(i = 0; i < BASE_COUNT; i++) {
		int j;
		int index = stateInfo->localGameInfo->pII.safeOnBaseIndex[i];
		int shouldRun = 1;
		if(i == 0 && stateInfo->localGameInfo->pRAI.batterCanAdvance == 0) continue;
		// here we check that there is no one running this same base interval.
		for(j = 0; j < BASE_COUNT; j++) {
			int runnerIndex = stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[j];
			if(runnerIndex != -1) {
				BaseID baseId = stateInfo->localGameInfo->playerInfo[runnerIndex].bTPI.baseId;
				int baseInt = base_to_int_index(baseId);

				if(baseInt == i) {
					if(runnerIndex != index) {
						shouldRun = 0;
					}
				}
			}
		}
		// if everything ok, initiate running.
		if(shouldRun == 1 && isDoubleClickingOk == 1 && stateInfo->localGameInfo->aiState.baseRunnerLock[i] == AI_NO_LOCK &&
		        stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] == 0 && index != -1 && stateInfo->localGameInfo->playerRuntime[index].goingForward != 1 &&
		        stateInfo->localGameInfo->aiState.amountOfClicks[i] == 0 && stateInfo->localGameInfo->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
			stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] = 1;
			stateInfo->localGameInfo->aiState.baseRunnerLock[i] = AI_DOUBLE_CLICK_LOCK;
			if(i == 0) stateInfo->keyStates->imitateKeyPress[KEY_DOWN] = 1;
			else if(i == 1) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 1;
			else if(i == 2) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 1;
			else if(i == 3) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 1;

		} else if(stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] == 0 && stateInfo->localGameInfo->aiState.baseRunnerLock[i] == AI_DOUBLE_CLICK_LOCK && stateInfo->localGameInfo->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
			stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] = 1;
			if(i == 0) stateInfo->keyStates->imitateKeyPress[KEY_DOWN] = 1;
			else if(i == 1) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 1;
			else if(i == 2) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 1;
			else if(i == 3) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 1;
		} else if(stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] == 1 && stateInfo->localGameInfo->aiState.baseRunnerLock[i] == AI_DOUBLE_CLICK_LOCK) {
			stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] = 0;
			if(stateInfo->localGameInfo->aiState.amountOfClicks[i] == 1) {
				stateInfo->localGameInfo->aiState.baseRunnerLock[i] = AI_NO_LOCK;
				stateInfo->localGameInfo->aiState.amountOfClicks[i] = 0;
			} else {
				stateInfo->localGameInfo->aiState.amountOfClicks[i]++;
			}
			stateInfo->localGameInfo->aiState.clickBreak[i] = 0;

			if(i == 0) stateInfo->keyStates->imitateKeyPress[KEY_DOWN] = 0;
			else if(i == 1) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 0;
			else if(i == 2) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 0;
			else if(i == 3) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 0;
		}

	}
}
