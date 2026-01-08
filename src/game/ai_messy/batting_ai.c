#include <math.h>
#include <stdlib.h>

#include "globals.h"
#include "batting_ai.h"
#include "batting_ai_strategy.h"
#include "action_implementation.h" // for flushKeys
#include "actions_messy/batting_system.h"
#include "game_manipulation.h"
#include "rng.h"
#include "actions_messy/batting_system.h"
#include "batting_ai_strategy.h"
#include "base_logic.h"
#include "base_control.h"

// Macros moved from action_implementation.c

#define CLICK_BREAK_CONSTANT 3

void initBattingAI(AIState* aiState)
{
	int i;
	aiState->battingKeyDown = 0;
	aiState->changingKeyDown = 0;
	aiState->actionKeyLock = AI_NO_LOCK;
	aiState->battingStyle = 0;
	aiState->runningBatter = 0;
	aiState->runningBaseRunners = 0;

	aiState->increaseKeyDown = 0;
	aiState->decreaseKeyDown = 0;
	aiState->angleDecided = 0;
	aiState->decidedAngle = 0.0f;
	aiState->aiWrongPitch = 0;
	aiState->planCalculated = 0;
	aiState->firstIndex = -1;
	aiState->firstIndexSelected = 0;
	aiState->change = 0;
	aiState->changeHasHappened = 0;
	for(i = 0; i < BASE_COUNT; i++) {
		aiState->baseRunnerKeyDown[i] = 0;
		aiState->lastSafeOnBaseIndex[i] = -1;
		aiState->baseRunnerDecisionMade[i] = 0;
		aiState->amountOfClicks[i] = 0;
		aiState->baseRunnerLock[i] = AI_NO_LOCK;
		aiState->clickBreak[i] = 0;
	}
}

void updateBattingAI(StateInfo* stateInfo, unsigned int* rng_seed)
{
	int i;
	int isDoubleClickingOk = 0;

	// Cleanup dangling locks if state changed externally
	if (stateInfo->localGameInfo->gameControl.waitingForBatterDecision == 0) {
		if (stateInfo->localGameInfo->aiState.actionKeyLock == AI_WAITING_BATTER_LOCK ||
		        stateInfo->localGameInfo->aiState.actionKeyLock == AI_CHANGE_LOCK) {
			stateInfo->localGameInfo->aiState.actionKeyLock = AI_NO_LOCK;
			stateInfo->localGameInfo->aiState.battingKeyDown = 0;
			stateInfo->localGameInfo->aiState.changingKeyDown = 0;
		}
	}
	if (stateInfo->localGameInfo->gameControl.waitingForFreeWalkDecision == 0) {
		if (stateInfo->localGameInfo->aiState.actionKeyLock == AI_WAITING_WALK_LOCK) {
			stateInfo->localGameInfo->aiState.actionKeyLock = AI_NO_LOCK;
			stateInfo->localGameInfo->aiState.battingKeyDown = 0;
		}
	}

	// update some flags
	for(i = 0; i < BASE_COUNT; i++) {
		stateInfo->localGameInfo->aiState.clickBreak[i]++;
		if(stateInfo->localGameInfo->aiState.clickBreak[i] > 1000) stateInfo->localGameInfo->aiState.clickBreak[i] = 0;
		if(stateInfo->localGameInfo->aiState.baseRunnerDecisionMade[i] == 1) {
			if(get_base_controller(stateInfo->localGameInfo, (BaseID)i) == -1 ) {
				stateInfo->localGameInfo->aiState.baseRunnerDecisionMade[i] = 0;
			}
			if(stateInfo->localGameInfo->aiState.lastSafeOnBaseIndex[i] != get_base_controller(stateInfo->localGameInfo, (BaseID)i)) {
				stateInfo->localGameInfo->aiState.baseRunnerDecisionMade[i] = 0;
			}
		}
		stateInfo->localGameInfo->aiState.lastSafeOnBaseIndex[i] = get_base_controller(stateInfo->localGameInfo, (BaseID)i);
	}
	if(stateInfo->localGameInfo->pRAI.batterReady == 0 && stateInfo->localGameInfo->aiState.planCalculated == 1) {
		stateInfo->localGameInfo->aiState.planCalculated = 0;
	}
	// make free walk decision == accept
	if(stateInfo->localGameInfo->gameControl.waitingForFreeWalkDecision == 1) {
		if(stateInfo->localGameInfo->aiState.battingKeyDown == 0) {
			if(stateInfo->localGameInfo->aiState.actionKeyLock == AI_NO_LOCK) {
				stateInfo->localGameInfo->aF.bTAF.takeFreeWalk = FREE_WALK_ACCEPT;
				stateInfo->localGameInfo->aiState.battingKeyDown = 1;
				stateInfo->localGameInfo->aiState.actionKeyLock = AI_WAITING_WALK_LOCK;
			}
		} else {
			stateInfo->localGameInfo->aiState.actionKeyLock = AI_NO_LOCK;
			stateInfo->localGameInfo->aiState.battingKeyDown = 0;
		}
	}
	// we decide batter only after ball is at home so that in normal situation ai will have more information
	// to make its strategy decisions
	if(stateInfo->localGameInfo->gameControl.waitingForBatterDecision == 1 && stateInfo->localGameInfo->gameFlowState.ballHome == 1) {
		// we do this by brute force, we change player until we find a fit one or we are back to non joker.
		// plan is that if there is a man on first base and current batter would not have a great power,
		// we would try to find a joker that has power instead.
		// and if field is empty we would change a joker with speed instead.
		int firstBaseIndex = get_base_controller(stateInfo->localGameInfo, (BaseID)1);
		int secondBaseIndex = get_base_controller(stateInfo->localGameInfo, (BaseID)2);
		int thirdBaseIndex = get_base_controller(stateInfo->localGameInfo, (BaseID)3);
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
			stateInfo->localGameInfo->aF.bTAF.chooseBatter = CHOOSE_BATTER_NEXT;
			stateInfo->localGameInfo->aiState.changingKeyDown = 1;
			stateInfo->localGameInfo->aiState.actionKeyLock = AI_CHANGE_LOCK;
		} else if(stateInfo->localGameInfo->aiState.changingKeyDown == 1 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_CHANGE_LOCK) {
			stateInfo->localGameInfo->aiState.actionKeyLock = AI_NO_LOCK;
			stateInfo->localGameInfo->aiState.changingKeyDown = 0;
			stateInfo->localGameInfo->aiState.changeHasHappened = 1;

		}
		// select best batter.
		if(stateInfo->localGameInfo->aiState.change == 0 && stateInfo->localGameInfo->aiState.battingKeyDown == 0 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_NO_LOCK) {
			stateInfo->localGameInfo->aF.bTAF.chooseBatter = CHOOSE_BATTER_SELECT;
			stateInfo->localGameInfo->aiState.battingKeyDown = 1;
			stateInfo->localGameInfo->aiState.actionKeyLock = AI_WAITING_BATTER_LOCK;
		} else if(stateInfo->localGameInfo->aiState.battingKeyDown == 1 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_WAITING_BATTER_LOCK) {
			stateInfo->localGameInfo->aiState.actionKeyLock = AI_NO_LOCK;
			stateInfo->localGameInfo->aiState.battingKeyDown = 0;
			stateInfo->localGameInfo->aiState.firstIndex = -1;
			stateInfo->localGameInfo->aiState.firstIndexSelected = 0;
			stateInfo->localGameInfo->aiState.changeHasHappened = 0;

		}

	} else if(stateInfo->localGameInfo->pRAI.batterReady == 1 && stateInfo->localGameInfo->pRAI.pitchState != PITCH_STAGE_AIRBORNE && stateInfo->localGameInfo->gameFlowState.ballHome == 1) {
		// decision tree.. contents can be read within
		if(stateInfo->localGameInfo->aiState.planCalculated == 0) {
			int batterIndex = stateInfo->localGameInfo->pII.batterIndex;
			int firstBaseIndex = get_base_controller(stateInfo->localGameInfo, (BaseID)1);
			int secondBaseIndex = get_base_controller(stateInfo->localGameInfo, (BaseID)2);
			int thirdBaseIndex = get_base_controller(stateInfo->localGameInfo, (BaseID)3);
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
				stateInfo->localGameInfo->aF.bTAF.baseRun[0] = ACTION_TRIGGER_START;
			} else if(stateInfo->localGameInfo->aiState.baseRunnerKeyDown[0] == 1 && stateInfo->localGameInfo->aiState.baseRunnerLock[0] == AI_CLICK_LOCK) {
				stateInfo->localGameInfo->aiState.baseRunnerKeyDown[0] = 0;
				stateInfo->localGameInfo->aiState.baseRunnerDecisionMade[0] = 1;
				stateInfo->localGameInfo->aiState.clickBreak[0] = 0;
				stateInfo->localGameInfo->aiState.baseRunnerLock[0] = AI_NO_LOCK;
			}
		}
		// if decide that baserunners should run, we click their keys.
		if(stateInfo->localGameInfo->aiState.runningBaseRunners == 1) {
			int i;
			for(i = 1; i < BASE_COUNT; i++) {
				// Prevent suicide runs: Don't run from 3rd base (to home) if ball is held at home
				if (i == BASE_THIRD && stateInfo->localGameInfo->gameFlowState.ballHome == 1) continue;

				if(stateInfo->localGameInfo->aiState.baseRunnerDecisionMade[i] == 0 && get_base_controller(stateInfo->localGameInfo, (BaseID)i) != -1 &&
				        stateInfo->localGameInfo->playerInfo[get_base_controller(stateInfo->localGameInfo, (BaseID)i)].bTPI.state == PLAYER_STATE_ON_BASE &&
				        stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] == 0 && stateInfo->localGameInfo->aiState.baseRunnerLock[i] == AI_NO_LOCK && stateInfo->localGameInfo->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
					stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] = 1;
					stateInfo->localGameInfo->aiState.baseRunnerLock[i] = AI_CLICK_LOCK;
					stateInfo->localGameInfo->aF.bTAF.baseRun[i] = ACTION_TRIGGER_START;
				} else if(stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] == 1 && stateInfo->localGameInfo->aiState.baseRunnerLock[i] == AI_CLICK_LOCK) {
					stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] = 0;
					stateInfo->localGameInfo->aiState.baseRunnerLock[i] = AI_NO_LOCK;
					stateInfo->localGameInfo->aiState.baseRunnerDecisionMade[i] = 1;
					stateInfo->localGameInfo->aiState.clickBreak[i] = 0;
				}
			}
		}
	}
	// if ball is not home, we return players from first and second base to their bases
	else if(stateInfo->localGameInfo->pRAI.batterReady == 1 && stateInfo->localGameInfo->pRAI.pitchState != PITCH_STAGE_AIRBORNE && stateInfo->localGameInfo->gameFlowState.ballHome == 0) {
		if(stateInfo->localGameInfo->aiState.runningBaseRunners == 1) {
			int i;
			for(i = 1; i < 3; i++) {
				if(get_base_controller(stateInfo->localGameInfo, (BaseID)i) != -1 &&
				        stateInfo->localGameInfo->playerInfo[get_base_controller(stateInfo->localGameInfo, (BaseID)i)].bTPI.state == PLAYER_STATE_LEADING &&
				        stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] == 0 && stateInfo->localGameInfo->aiState.baseRunnerLock[i] == AI_NO_LOCK && stateInfo->localGameInfo->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
					stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] = 1;
					stateInfo->localGameInfo->aiState.baseRunnerLock[i] = AI_COME_BACK_LOCK;
					stateInfo->localGameInfo->aF.bTAF.baseRun[i] = ACTION_TRIGGER_START;
				} else if(stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] == 1 && stateInfo->localGameInfo->aiState.baseRunnerLock[i] == AI_COME_BACK_LOCK) {
					stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] = 0;
					stateInfo->localGameInfo->aiState.baseRunnerDecisionMade[i] = 0;
					stateInfo->localGameInfo->aiState.baseRunnerLock[i] = AI_NO_LOCK;
					stateInfo->localGameInfo->aiState.clickBreak[i] = 0;
				}
			}
		}
	}
	// and here we bat
	else if(stateInfo->localGameInfo->pRAI.pitchState == PITCH_STAGE_AIRBORNE) {
		int i;
		// predict if pitch is going to be ball
		if(stateInfo->localGameInfo->aiState.aiWrongPitch == 0 && is_wrong_pitch(
		            stateInfo->localGameInfo->ballInfo.velocity.x,
		            stateInfo->localGameInfo->ballInfo.velocity.y,
		            GRAVITY,
		            PLATE_WIDTH
		        )) {
			stateInfo->localGameInfo->aiState.aiWrongPitch = 1;
		}
		if(stateInfo->localGameInfo->aiState.aiWrongPitch == 1) {
			// batter isnt handled here
			// this code will make baserunners come back if wrong pitch is pitched
			for(i = 1; i < BASE_COUNT; i++) {
				int index = get_base_controller(stateInfo->localGameInfo, (BaseID)i);
				if(index != -1 && stateInfo->localGameInfo->playerRuntime[index].goingForward == 1 && stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] == 0 &&
				        stateInfo->localGameInfo->aiState.baseRunnerLock[i] == AI_NO_LOCK && stateInfo->localGameInfo->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
					stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] = 1;
					stateInfo->localGameInfo->aiState.baseRunnerLock[i] = AI_COME_BACK_WRONG_PITCH_LOCK;
					stateInfo->localGameInfo->aF.bTAF.baseRun[i] = ACTION_TRIGGER_START;
				} else if(stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] == 1 && stateInfo->localGameInfo->aiState.baseRunnerLock[i] == AI_COME_BACK_WRONG_PITCH_LOCK) {
					stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] = 0;
					stateInfo->localGameInfo->aiState.baseRunnerDecisionMade[i] = 0;
					stateInfo->localGameInfo->aiState.baseRunnerLock[i] = AI_NO_LOCK;
					stateInfo->localGameInfo->aiState.clickBreak[i] = 0;
				}
			}
		}
		// a bunt
		if(stateInfo->localGameInfo->aiState.battingStyle == 0) {
			if(stateInfo->localGameInfo->aiState.angleDecided == 0) {
				stateInfo->localGameInfo->aiState.decidedAngle = calculate_ai_batting_angle(0, -1, seeded_rand(rng_seed, RAND_MAX));
				stateInfo->localGameInfo->aiState.angleDecided = 1;
			}
			if(stateInfo->localGameInfo->pendingActionState.meterCounter > BAT_SWING_MAX - 23 && stateInfo->localGameInfo->aiState.battingKeyDown == 0 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_NO_LOCK && stateInfo->localGameInfo->aiState.aiWrongPitch == 0) {
				stateInfo->localGameInfo->aF.bTAF.swing = BAT_ACTION_POWER_SET;
				stateInfo->localGameInfo->aiState.battingKeyDown = 1;
				stateInfo->localGameInfo->aiState.actionKeyLock = AI_BATTING_LOCK;
			} else if(stateInfo->localGameInfo->aiState.battingKeyDown == 1 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_BATTING_LOCK) {
				if(stateInfo->localGameInfo->pendingActionState.meterCounter > BAT_LOAD_MAX - 9) {
					stateInfo->localGameInfo->aF.bTAF.swing = BAT_ACTION_ANGLE_SET;
					stateInfo->localGameInfo->aiState.battingKeyDown = 0;
					stateInfo->localGameInfo->aiState.actionKeyLock = AI_NO_LOCK;
				}
			}
		}
		// a normal swing
		else if(stateInfo->localGameInfo->aiState.battingStyle == 1) {
			if(stateInfo->localGameInfo->aiState.angleDecided == 0) {
				int i;
				BaseID leadBase = BASE_NONE;

				for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
					if(stateInfo->localGameInfo->playerInfo[i].bTPI.baseId != BASE_NONE && stateInfo->localGameInfo->playerInfo[i].bTPI.state != PLAYER_STATE_WOUNDED) {
						BaseID currentBaseId = stateInfo->localGameInfo->playerInfo[i].bTPI.baseId;

						if(base_cmp(currentBaseId, leadBase) > 0) {
							leadBase = currentBaseId;
						}
					}
				}
				stateInfo->localGameInfo->aiState.decidedAngle = calculate_ai_batting_angle(1, leadBase, seeded_rand(rng_seed, RAND_MAX));
				stateInfo->localGameInfo->aiState.angleDecided = 1;
			}
			if(stateInfo->localGameInfo->pendingActionState.meterCounter > BAT_SWING_MAX - 10 && stateInfo->localGameInfo->aiState.battingKeyDown == 0 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_NO_LOCK && stateInfo->localGameInfo->aiState.aiWrongPitch == 0) {
				stateInfo->localGameInfo->aF.bTAF.swing = BAT_ACTION_POWER_SET;
				stateInfo->localGameInfo->aiState.battingKeyDown = 1;
				stateInfo->localGameInfo->aiState.actionKeyLock = AI_BATTING_LOCK;
			} else if(stateInfo->localGameInfo->aiState.battingKeyDown == 1 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_BATTING_LOCK) {
				if(stateInfo->localGameInfo->pendingActionState.meterCounter > BAT_LOAD_MAX - 6) {
					stateInfo->localGameInfo->aF.bTAF.swing = BAT_ACTION_ANGLE_SET;
					stateInfo->localGameInfo->aiState.battingKeyDown = 0;
					stateInfo->localGameInfo->aiState.actionKeyLock = AI_NO_LOCK;
				}
			}
		}
		// swing that tries to get oneself wounded
		else if(stateInfo->localGameInfo->aiState.battingStyle == 2) {
			if(stateInfo->localGameInfo->aiState.angleDecided == 0) {
				stateInfo->localGameInfo->aiState.decidedAngle = calculate_ai_batting_angle(2, BASE_NONE, seeded_rand(rng_seed, RAND_MAX));
				stateInfo->localGameInfo->aiState.angleDecided = 1;
			}
			if(stateInfo->localGameInfo->pendingActionState.meterCounter > BAT_SWING_MAX - 11 && stateInfo->localGameInfo->aiState.battingKeyDown == 0 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_NO_LOCK && stateInfo->localGameInfo->aiState.aiWrongPitch == 0) {
				stateInfo->localGameInfo->aF.bTAF.swing = BAT_ACTION_POWER_SET;
				stateInfo->localGameInfo->aiState.battingKeyDown = 1;
				stateInfo->localGameInfo->aiState.actionKeyLock = AI_BATTING_LOCK;
			} else if(stateInfo->localGameInfo->aiState.battingKeyDown == 1 && stateInfo->localGameInfo->aiState.actionKeyLock == AI_BATTING_LOCK) {
				if(stateInfo->localGameInfo->pendingActionState.meterCounter > BAT_LOAD_MAX - 8) {
					stateInfo->localGameInfo->aF.bTAF.swing = BAT_ACTION_ANGLE_SET;
					stateInfo->localGameInfo->aiState.battingKeyDown = 0;
					stateInfo->localGameInfo->aiState.actionKeyLock = AI_NO_LOCK;
				}
			}
		}
		if(stateInfo->localGameInfo->aiState.decidedAngle >= 0 && stateInfo->localGameInfo->pendingActionState.batterAngle < stateInfo->localGameInfo->aiState.decidedAngle && stateInfo->localGameInfo->aiState.increaseKeyDown == 0) {
			stateInfo->localGameInfo->aF.bTAF.increaseBatterAngle = ACTION_TRIGGER_START;
			stateInfo->localGameInfo->aiState.increaseKeyDown = 1;
		} else if(stateInfo->localGameInfo->pendingActionState.batterAngle >= stateInfo->localGameInfo->aiState.decidedAngle && stateInfo->localGameInfo->aiState.increaseKeyDown == 1) {
			stateInfo->localGameInfo->aF.bTAF.increaseBatterAngle = ACTION_TRIGGER_STOP;
			stateInfo->localGameInfo->aiState.increaseKeyDown = 0;
		}

		if(stateInfo->localGameInfo->aiState.decidedAngle < 0 && stateInfo->localGameInfo->pendingActionState.batterAngle > stateInfo->localGameInfo->aiState.decidedAngle && stateInfo->localGameInfo->aiState.decreaseKeyDown == 0) {
			stateInfo->localGameInfo->aF.bTAF.decreaseBatterAngle = ACTION_TRIGGER_START;
			stateInfo->localGameInfo->aiState.decreaseKeyDown = 1;
		} else if(stateInfo->localGameInfo->pendingActionState.batterAngle <= stateInfo->localGameInfo->aiState.decidedAngle && stateInfo->localGameInfo->aiState.decreaseKeyDown == 1) {
			stateInfo->localGameInfo->aF.bTAF.decreaseBatterAngle = ACTION_TRIGGER_STOP;
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
		int index = get_base_controller(stateInfo->localGameInfo, (BaseID)i);
		int shouldRun = 1;
		if(i == 0 && stateInfo->localGameInfo->pRAI.batterCanAdvance == 0) continue;
		// here we check that there is no one running this same base interval.
		for(j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
			BaseID bid = stateInfo->localGameInfo->playerInfo[j].bTPI.baseId;
			if(bid != BASE_NONE) {
				int baseInt = base_to_int_index(bid);

				if(baseInt == i) {
					if(j != index) {
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
			stateInfo->localGameInfo->aF.bTAF.baseRun[i] = ACTION_TRIGGER_START;

		} else if(stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] == 0 && stateInfo->localGameInfo->aiState.baseRunnerLock[i] == AI_DOUBLE_CLICK_LOCK && stateInfo->localGameInfo->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
			stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] = 1;
			stateInfo->localGameInfo->aF.bTAF.baseRun[i] = ACTION_TRIGGER_START;
		} else if(stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] == 1 && stateInfo->localGameInfo->aiState.baseRunnerLock[i] == AI_DOUBLE_CLICK_LOCK) {
			stateInfo->localGameInfo->aiState.baseRunnerKeyDown[i] = 0;
			if(stateInfo->localGameInfo->aiState.amountOfClicks[i] == 1) {
				stateInfo->localGameInfo->aiState.baseRunnerLock[i] = AI_NO_LOCK;
				stateInfo->localGameInfo->aiState.amountOfClicks[i] = 0;
			} else {
				stateInfo->localGameInfo->aiState.amountOfClicks[i]++;
			}
			stateInfo->localGameInfo->aiState.clickBreak[i] = 0;
		}

	}
}
