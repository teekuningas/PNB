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
#include "rules_pure/player_utils.h"

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
	if (stateInfo->match->flowControl.waitingForBatterDecision == 0) {
		if (stateInfo->match->aiState.actionKeyLock == AI_WAITING_BATTER_LOCK ||
		        stateInfo->match->aiState.actionKeyLock == AI_CHANGE_LOCK) {
			stateInfo->match->aiState.actionKeyLock = AI_NO_LOCK;
			stateInfo->match->aiState.battingKeyDown = 0;
			stateInfo->match->aiState.changingKeyDown = 0;
		}
	}
	if (stateInfo->match->flowControl.waitingForFreeWalkDecision == 0) {
		if (stateInfo->match->aiState.actionKeyLock == AI_WAITING_WALK_LOCK) {
			stateInfo->match->aiState.actionKeyLock = AI_NO_LOCK;
			stateInfo->match->aiState.battingKeyDown = 0;
		}
	}

	// update some flags
	for(i = 0; i < BASE_COUNT; i++) {
		stateInfo->match->aiState.clickBreak[i]++;
		if(stateInfo->match->aiState.clickBreak[i] > 1000) stateInfo->match->aiState.clickBreak[i] = 0;
		if(stateInfo->match->aiState.baseRunnerDecisionMade[i] == 1) {
			if(get_base_controller(stateInfo->match, (BaseID)i) == -1 ) {
				stateInfo->match->aiState.baseRunnerDecisionMade[i] = 0;
			}
			if(stateInfo->match->aiState.lastSafeOnBaseIndex[i] != get_base_controller(stateInfo->match, (BaseID)i)) {
				stateInfo->match->aiState.baseRunnerDecisionMade[i] = 0;
			}
		}
		stateInfo->match->aiState.lastSafeOnBaseIndex[i] = get_base_controller(stateInfo->match, (BaseID)i);
	}
	if(stateInfo->match->pRAI.batterReady == 0 &&stateInfo->match->aiState.planCalculated == 1) {
		stateInfo->match->aiState.planCalculated = 0;
	}
	// make free walk decision == accept
	if(stateInfo->match->flowControl.waitingForFreeWalkDecision == 1) {
		if(stateInfo->match->aiState.battingKeyDown == 0) {
			if(stateInfo->match->aiState.actionKeyLock == AI_NO_LOCK) {
				stateInfo->match->aF.bTAF.takeFreeWalk = FREE_WALK_ACCEPT;
				stateInfo->match->aiState.battingKeyDown = 1;
				stateInfo->match->aiState.actionKeyLock = AI_WAITING_WALK_LOCK;
			}
		} else {
			stateInfo->match->aiState.actionKeyLock = AI_NO_LOCK;
			stateInfo->match->aiState.battingKeyDown = 0;
		}
	}
	// we decide batter only after ball is at home so that in normal situation ai will have more information
	// to make its strategy decisions
	if(stateInfo->match->flowControl.waitingForBatterDecision == 1 &&stateInfo->match->gameFlowState.ballHome == 1) {
		// we do this by brute force, we change player until we find a fit one or we are back to non joker.
		// plan is that if there is a man on first base and current batter would not have a great power,
		// we would try to find a joker that has power instead.
		// and if field is empty we would change a joker with speed instead.
		int firstBaseIndex = get_base_controller(stateInfo->match, (BaseID)1);
		int secondBaseIndex = get_base_controller(stateInfo->match, (BaseID)2);
		int thirdBaseIndex = get_base_controller(stateInfo->match, (BaseID)3);
		int fieldStatus;
		int index = stateInfo->match->pII.batterSelectionIndex;

		if(firstBaseIndex != -1) fieldStatus = 2;
		else if(secondBaseIndex != -1 || thirdBaseIndex != -1) fieldStatus = 1;
		else fieldStatus = 0;

		stateInfo->match->aiState.change = should_change_batter(
		                                       fieldStatus,
		                                       stateInfo->match->playerInfo[index].bTPI.power,
		                                       stateInfo->match->playerInfo[index].bTPI.speed
		                                   );

		if(stateInfo->match->aiState.firstIndexSelected == 0) {
			stateInfo->match->aiState.firstIndex = index;
			stateInfo->match->aiState.firstIndexSelected = 1;
		} else if(stateInfo->match->aiState.changeHasHappened == 1) {
			if(stateInfo->match->aiState.firstIndex == index) {
				stateInfo->match->aiState.change = 0;
			}
		}

		// change player
		if(stateInfo->match->aiState.change == 1 &&stateInfo->match->aiState.changingKeyDown == 0 &&stateInfo->match->aiState.actionKeyLock == AI_NO_LOCK) {
			stateInfo->match->aF.bTAF.chooseBatter = CHOOSE_BATTER_NEXT;
			stateInfo->match->aiState.changingKeyDown = 1;
			stateInfo->match->aiState.actionKeyLock = AI_CHANGE_LOCK;
		} else if(stateInfo->match->aiState.changingKeyDown == 1 &&stateInfo->match->aiState.actionKeyLock == AI_CHANGE_LOCK) {
			stateInfo->match->aiState.actionKeyLock = AI_NO_LOCK;
			stateInfo->match->aiState.changingKeyDown = 0;
			stateInfo->match->aiState.changeHasHappened = 1;

		}
		// select best batter.
		if(stateInfo->match->aiState.change == 0 &&stateInfo->match->aiState.battingKeyDown == 0 &&stateInfo->match->aiState.actionKeyLock == AI_NO_LOCK) {
			stateInfo->match->aF.bTAF.chooseBatter = CHOOSE_BATTER_SELECT;
			stateInfo->match->aiState.battingKeyDown = 1;
			stateInfo->match->aiState.actionKeyLock = AI_WAITING_BATTER_LOCK;
		} else if(stateInfo->match->aiState.battingKeyDown == 1 &&stateInfo->match->aiState.actionKeyLock == AI_WAITING_BATTER_LOCK) {
			stateInfo->match->aiState.actionKeyLock = AI_NO_LOCK;
			stateInfo->match->aiState.battingKeyDown = 0;
			stateInfo->match->aiState.firstIndex = -1;
			stateInfo->match->aiState.firstIndexSelected = 0;
			stateInfo->match->aiState.changeHasHappened = 0;

		}

	} else if(stateInfo->match->pRAI.batterReady == 1 &&stateInfo->match->pRAI.pitchState != PITCH_STAGE_AIRBORNE &&stateInfo->match->gameFlowState.ballHome == 1) {
		// decision tree.. contents can be read within
		if(stateInfo->match->aiState.planCalculated == 0) {
			int batterIndex = get_active_batter_index(stateInfo->match);
			int firstBaseIndex = get_base_controller(stateInfo->match, (BaseID)1);
			int secondBaseIndex = get_base_controller(stateInfo->match, (BaseID)2);
			int thirdBaseIndex = get_base_controller(stateInfo->match, (BaseID)3);
			int power = stateInfo->match->playerInfo[batterIndex].bTPI.power;
			int speed = stateInfo->match->playerInfo[batterIndex].bTPI.speed;
			int fieldStatus;

			if(firstBaseIndex != -1) fieldStatus = 2;
			else if(secondBaseIndex != -1 || thirdBaseIndex != -1) fieldStatus = 1;
			else fieldStatus = 0;

			BattingStrategy strategy = calculate_batting_strategy(
			                               &(stateInfo->match->halfInningState),
			                               fieldStatus,
			                               power,
			                               speed,
			                               stateInfo->match->scoreboard.period
			                           );

			stateInfo->match->aiState.battingStyle = strategy.style;
			stateInfo->match->aiState.runningBaseRunners = strategy.runBaseRunners;
			stateInfo->match->aiState.runningBatter = strategy.runBatter;

			stateInfo->match->aiState.planCalculated = 1;
		}
		// if we decide that batter should run, we click down once.
		if(stateInfo->match->aiState.runningBatter == 1) {
			if(stateInfo->match->aiState.baseRunnerDecisionMade[0] == 0 &&stateInfo->match->aiState.baseRunnerKeyDown[0] == 0 &&stateInfo->match->aiState.baseRunnerLock[0] == AI_NO_LOCK &&
			        stateInfo->match->aiState.clickBreak[0] > CLICK_BREAK_CONSTANT) {
				stateInfo->match->aiState.baseRunnerKeyDown[0] = 1;
				stateInfo->match->aiState.baseRunnerLock[0] = AI_CLICK_LOCK;
				stateInfo->match->aF.bTAF.baseRun[0] = ACTION_TRIGGER_START;
			} else if(stateInfo->match->aiState.baseRunnerKeyDown[0] == 1 &&stateInfo->match->aiState.baseRunnerLock[0] == AI_CLICK_LOCK) {
				stateInfo->match->aiState.baseRunnerKeyDown[0] = 0;
				stateInfo->match->aiState.baseRunnerDecisionMade[0] = 1;
				stateInfo->match->aiState.clickBreak[0] = 0;
				stateInfo->match->aiState.baseRunnerLock[0] = AI_NO_LOCK;
			}
		}
		// if decide that baserunners should run, we click their keys.
		if(stateInfo->match->aiState.runningBaseRunners == 1) {
			int i;
			for(i = 1; i < BASE_COUNT; i++) {
				// Prevent suicide runs: Don't run from 3rd base (to home) if ball is held at home
				if (i == BASE_THIRD &&stateInfo->match->gameFlowState.ballHome == 1) continue;

				if(stateInfo->match->aiState.baseRunnerDecisionMade[i] == 0 &&get_base_controller(stateInfo->match, (BaseID)i) != -1 &&
				        stateInfo->match->playerInfo[get_base_controller(stateInfo->match, (BaseID)i)].bTPI.state == PLAYER_STATE_ON_BASE &&
				        stateInfo->match->aiState.baseRunnerKeyDown[i] == 0 &&stateInfo->match->aiState.baseRunnerLock[i] == AI_NO_LOCK &&stateInfo->match->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
					stateInfo->match->aiState.baseRunnerKeyDown[i] = 1;
					stateInfo->match->aiState.baseRunnerLock[i] = AI_CLICK_LOCK;
					stateInfo->match->aF.bTAF.baseRun[i] = ACTION_TRIGGER_START;
				} else if(stateInfo->match->aiState.baseRunnerKeyDown[i] == 1 &&stateInfo->match->aiState.baseRunnerLock[i] == AI_CLICK_LOCK) {
					stateInfo->match->aiState.baseRunnerKeyDown[i] = 0;
					stateInfo->match->aiState.baseRunnerLock[i] = AI_NO_LOCK;
					stateInfo->match->aiState.baseRunnerDecisionMade[i] = 1;
					stateInfo->match->aiState.clickBreak[i] = 0;
				}
			}
		}
	}
	// if ball is not home, we return players from first and second base to their bases
	else if(stateInfo->match->pRAI.batterReady == 1 &&stateInfo->match->pRAI.pitchState != PITCH_STAGE_AIRBORNE &&stateInfo->match->gameFlowState.ballHome == 0) {
		if(stateInfo->match->aiState.runningBaseRunners == 1) {
			int i;
			for(i = 1; i < 3; i++) {
				if(get_base_controller(stateInfo->match, (BaseID)i) != -1 &&
				        stateInfo->match->playerInfo[get_base_controller(stateInfo->match, (BaseID)i)].bTPI.state == PLAYER_STATE_LEADING &&
				        stateInfo->match->aiState.baseRunnerKeyDown[i] == 0 &&stateInfo->match->aiState.baseRunnerLock[i] == AI_NO_LOCK &&stateInfo->match->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
					stateInfo->match->aiState.baseRunnerKeyDown[i] = 1;
					stateInfo->match->aiState.baseRunnerLock[i] = AI_COME_BACK_LOCK;
					stateInfo->match->aF.bTAF.baseRun[i] = ACTION_TRIGGER_START;
				} else if(stateInfo->match->aiState.baseRunnerKeyDown[i] == 1 &&stateInfo->match->aiState.baseRunnerLock[i] == AI_COME_BACK_LOCK) {
					stateInfo->match->aiState.baseRunnerKeyDown[i] = 0;
					stateInfo->match->aiState.baseRunnerDecisionMade[i] = 0;
					stateInfo->match->aiState.baseRunnerLock[i] = AI_NO_LOCK;
					stateInfo->match->aiState.clickBreak[i] = 0;
				}
			}
		}
	}
	// and here we bat
	else if(stateInfo->match->pRAI.pitchState == PITCH_STAGE_AIRBORNE) {
		int i;
		// predict if pitch is going to be ball
		if(stateInfo->match->aiState.aiWrongPitch == 0 &&is_wrong_pitch(
		            stateInfo->match->ballInfo.velocity.x,
		            stateInfo->match->ballInfo.velocity.y,
		            GRAVITY,
		            PLATE_WIDTH
		        )) {
			stateInfo->match->aiState.aiWrongPitch = 1;
		}
		if(stateInfo->match->aiState.aiWrongPitch == 1) {
			// batter isnt handled here
			// this code will make baserunners come back if wrong pitch is pitched
			for(i = 1; i < BASE_COUNT; i++) {
				int index = get_base_controller(stateInfo->match, (BaseID)i);
				if(index != -1 &&stateInfo->match->playerRuntime[index].goingForward == 1 &&stateInfo->match->aiState.baseRunnerKeyDown[i] == 0 &&
				        stateInfo->match->aiState.baseRunnerLock[i] == AI_NO_LOCK &&stateInfo->match->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
					stateInfo->match->aiState.baseRunnerKeyDown[i] = 1;
					stateInfo->match->aiState.baseRunnerLock[i] = AI_COME_BACK_WRONG_PITCH_LOCK;
					stateInfo->match->aF.bTAF.baseRun[i] = ACTION_TRIGGER_START;
				} else if(stateInfo->match->aiState.baseRunnerKeyDown[i] == 1 &&stateInfo->match->aiState.baseRunnerLock[i] == AI_COME_BACK_WRONG_PITCH_LOCK) {
					stateInfo->match->aiState.baseRunnerKeyDown[i] = 0;
					stateInfo->match->aiState.baseRunnerDecisionMade[i] = 0;
					stateInfo->match->aiState.baseRunnerLock[i] = AI_NO_LOCK;
					stateInfo->match->aiState.clickBreak[i] = 0;
				}
			}
		}
		// a bunt
		if(stateInfo->match->aiState.battingStyle == 0) {
			if(stateInfo->match->aiState.angleDecided == 0) {
				stateInfo->match->aiState.decidedAngle = calculate_ai_batting_angle(0, -1, seeded_rand(rng_seed, RAND_MAX));
				stateInfo->match->aiState.angleDecided = 1;
			}
			if(stateInfo->match->pendingActionState.meterCounter > BAT_SWING_MAX - 23 &&stateInfo->match->aiState.battingKeyDown == 0 &&stateInfo->match->aiState.actionKeyLock == AI_NO_LOCK &&stateInfo->match->aiState.aiWrongPitch == 0) {
				stateInfo->match->aF.bTAF.swing = BAT_ACTION_POWER_SET;
				stateInfo->match->aiState.battingKeyDown = 1;
				stateInfo->match->aiState.actionKeyLock = AI_BATTING_LOCK;
			} else if(stateInfo->match->aiState.battingKeyDown == 1 &&stateInfo->match->aiState.actionKeyLock == AI_BATTING_LOCK) {
				if(stateInfo->match->pendingActionState.meterCounter > BAT_LOAD_MAX - 9) {
					stateInfo->match->aF.bTAF.swing = BAT_ACTION_ANGLE_SET;
					stateInfo->match->aiState.battingKeyDown = 0;
					stateInfo->match->aiState.actionKeyLock = AI_NO_LOCK;
				}
			}
		}
		// a normal swing
		else if(stateInfo->match->aiState.battingStyle == 1) {
			if(stateInfo->match->aiState.angleDecided == 0) {
				int i;
				BaseID leadBase = BASE_NONE;

				for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
					if(stateInfo->match->playerInfo[i].bTPI.baseId != BASE_NONE &&stateInfo->match->playerInfo[i].bTPI.state != PLAYER_STATE_WOUNDED) {
						BaseID currentBaseId = stateInfo->match->playerInfo[i].bTPI.baseId;

						if(base_cmp(currentBaseId, leadBase) > 0) {
							leadBase = currentBaseId;
						}
					}
				}
				stateInfo->match->aiState.decidedAngle = calculate_ai_batting_angle(1, leadBase, seeded_rand(rng_seed, RAND_MAX));
				stateInfo->match->aiState.angleDecided = 1;
			}
			if(stateInfo->match->pendingActionState.meterCounter > BAT_SWING_MAX - 10 &&stateInfo->match->aiState.battingKeyDown == 0 &&stateInfo->match->aiState.actionKeyLock == AI_NO_LOCK &&stateInfo->match->aiState.aiWrongPitch == 0) {
				stateInfo->match->aF.bTAF.swing = BAT_ACTION_POWER_SET;
				stateInfo->match->aiState.battingKeyDown = 1;
				stateInfo->match->aiState.actionKeyLock = AI_BATTING_LOCK;
			} else if(stateInfo->match->aiState.battingKeyDown == 1 &&stateInfo->match->aiState.actionKeyLock == AI_BATTING_LOCK) {
				if(stateInfo->match->pendingActionState.meterCounter > BAT_LOAD_MAX - 6) {
					stateInfo->match->aF.bTAF.swing = BAT_ACTION_ANGLE_SET;
					stateInfo->match->aiState.battingKeyDown = 0;
					stateInfo->match->aiState.actionKeyLock = AI_NO_LOCK;
				}
			}
		}
		// swing that tries to get oneself wounded
		else if(stateInfo->match->aiState.battingStyle == 2) {
			if(stateInfo->match->aiState.angleDecided == 0) {
				stateInfo->match->aiState.decidedAngle = calculate_ai_batting_angle(2, BASE_NONE, seeded_rand(rng_seed, RAND_MAX));
				stateInfo->match->aiState.angleDecided = 1;
			}
			if(stateInfo->match->pendingActionState.meterCounter > BAT_SWING_MAX - 11 &&stateInfo->match->aiState.battingKeyDown == 0 &&stateInfo->match->aiState.actionKeyLock == AI_NO_LOCK &&stateInfo->match->aiState.aiWrongPitch == 0) {
				stateInfo->match->aF.bTAF.swing = BAT_ACTION_POWER_SET;
				stateInfo->match->aiState.battingKeyDown = 1;
				stateInfo->match->aiState.actionKeyLock = AI_BATTING_LOCK;
			} else if(stateInfo->match->aiState.battingKeyDown == 1 &&stateInfo->match->aiState.actionKeyLock == AI_BATTING_LOCK) {
				if(stateInfo->match->pendingActionState.meterCounter > BAT_LOAD_MAX - 8) {
					stateInfo->match->aF.bTAF.swing = BAT_ACTION_ANGLE_SET;
					stateInfo->match->aiState.battingKeyDown = 0;
					stateInfo->match->aiState.actionKeyLock = AI_NO_LOCK;
				}
			}
		}
		if(stateInfo->match->aiState.decidedAngle >= 0 &&stateInfo->match->pendingActionState.batterAngle < stateInfo->match->aiState.decidedAngle &&stateInfo->match->aiState.increaseKeyDown == 0) {
			stateInfo->match->aF.bTAF.increaseBatterAngle = ACTION_TRIGGER_START;
			stateInfo->match->aiState.increaseKeyDown = 1;
		} else if(stateInfo->match->pendingActionState.batterAngle >= stateInfo->match->aiState.decidedAngle &&stateInfo->match->aiState.increaseKeyDown == 1) {
			stateInfo->match->aF.bTAF.increaseBatterAngle = ACTION_TRIGGER_STOP;
			stateInfo->match->aiState.increaseKeyDown = 0;
		}

		if(stateInfo->match->aiState.decidedAngle < 0 &&stateInfo->match->pendingActionState.batterAngle > stateInfo->match->aiState.decidedAngle &&stateInfo->match->aiState.decreaseKeyDown == 0) {
			stateInfo->match->aF.bTAF.decreaseBatterAngle = ACTION_TRIGGER_START;
			stateInfo->match->aiState.decreaseKeyDown = 1;
		} else if(stateInfo->match->pendingActionState.batterAngle <= stateInfo->match->aiState.decidedAngle &&stateInfo->match->aiState.decreaseKeyDown == 1) {
			stateInfo->match->aF.bTAF.decreaseBatterAngle = ACTION_TRIGGER_STOP;
			stateInfo->match->aiState.decreaseKeyDown = 0;
		}

	}
	if(stateInfo->match->pRAI.pitchState != PITCH_STAGE_AIRBORNE &&stateInfo->match->aiState.angleDecided == 1) {
		stateInfo->match->aiState.angleDecided = 0;
	}
	// AI: Check if it's safe to advance runners
	// Ball was hit, not caught, no one has it, no throw in progress, and ball is physically outside field
	if(stateInfo->match->pRAI.batHit == 1 &&
	        stateInfo->match->betweenPitchState.catchHasBeenMade == 0 &&
	        stateInfo->match->pRAI.throwGoingToBase == -1 &&
	        stateInfo->match->pII.hasBallIndex == -1 &&
	        stateInfo->match->ballInfo.moving == 1 &&
	        checkIfBallIsOutOfBounds(&stateInfo->match->ballInfo, stateInfo->fieldPositions)) {
		isDoubleClickingOk = 1;
	}
	// we will run with everyone so we need to simulate double click here.
	for(i = 0; i < BASE_COUNT; i++) {
		int j;
		int index = get_base_controller(stateInfo->match, (BaseID)i);
		int shouldRun = 1;
		if(i == 0 &&stateInfo->match->pRAI.batterCanAdvance == 0) continue;
		// here we check that there is no one running this same base interval.
		for(j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
			BaseID bid = stateInfo->match->playerInfo[j].bTPI.baseId;
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
		if(shouldRun == 1 &&isDoubleClickingOk == 1 &&stateInfo->match->aiState.baseRunnerLock[i] == AI_NO_LOCK &&
		        stateInfo->match->aiState.baseRunnerKeyDown[i] == 0 &&index != -1 &&stateInfo->match->playerRuntime[index].goingForward != 1 &&
		        stateInfo->match->aiState.amountOfClicks[i] == 0 &&stateInfo->match->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
			stateInfo->match->aiState.baseRunnerKeyDown[i] = 1;
			stateInfo->match->aiState.baseRunnerLock[i] = AI_DOUBLE_CLICK_LOCK;
			stateInfo->match->aF.bTAF.baseRun[i] = ACTION_TRIGGER_START;

		} else if(stateInfo->match->aiState.baseRunnerKeyDown[i] == 0 &&stateInfo->match->aiState.baseRunnerLock[i] == AI_DOUBLE_CLICK_LOCK &&stateInfo->match->aiState.clickBreak[i] > CLICK_BREAK_CONSTANT) {
			stateInfo->match->aiState.baseRunnerKeyDown[i] = 1;
			stateInfo->match->aF.bTAF.baseRun[i] = ACTION_TRIGGER_START;
		} else if(stateInfo->match->aiState.baseRunnerKeyDown[i] == 1 &&stateInfo->match->aiState.baseRunnerLock[i] == AI_DOUBLE_CLICK_LOCK) {
			stateInfo->match->aiState.baseRunnerKeyDown[i] = 0;
			if(stateInfo->match->aiState.amountOfClicks[i] == 1) {
				stateInfo->match->aiState.baseRunnerLock[i] = AI_NO_LOCK;
				stateInfo->match->aiState.amountOfClicks[i] = 0;
			} else {
				stateInfo->match->aiState.amountOfClicks[i]++;
			}
			stateInfo->match->aiState.clickBreak[i] = 0;
		}

	}
}
