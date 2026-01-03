#include <math.h>
#include <stdlib.h>

#include "globals.h"
#include "batting_ai.h"
#include "batting_ai_strategy.h"
#include "action_implementation.h" // for flushKeys
#include "actions_messy/action_state.h"
#include "actions_messy/batting_system.h"
#include "game_manipulation.h"
#include "rng.h"
#include "base_logic.h"

#define CLICK_BREAK_CONSTANT 3

// batting team ai variables
static int aiBattingKeyDown;
static int aiActionKeyLock;

static int aiChangingKeyDown;

static int aiIncreaseKeyDown;
static int aiDecreaseKeyDown;
static int aiAngleDecided;
static float aiDecidedAngle;

static int aiBaseRunnerKeyDown[BASE_COUNT];
static int aiBaseRunnerDecisionMade[BASE_COUNT];
static int aiLastSafeOnBaseIndex[BASE_COUNT];
static int aiBaseRunnerLock[BASE_COUNT];
static int aiAmountOfClicks[BASE_COUNT];
static int aiClickBreak[BASE_COUNT];

static int aiBattingStyle;
static int aiRunningBatter;
static int aiRunningBaseRunners;

static int aiPlanCalculated;
static int aiFirstIndex;
static int aiFirstIndexSelected;
static int aiChange;
static int aiChangeHasHappened;

// Forward declarations if needed, but we use logic directly.
// The code uses doubleClickCounter which is static in action_implementation.c
// Wait, I noticed earlier that the moved code used doubleClickCounter[base].
// Let me check again.

// L829: if(doubleClickCounter[base] == -1) {
// That was in baseRun function.
// In aiLogic (batting part):
// L1092: if(shouldRun == 1 && isDoubleClickingOk == 1 && aiBaseRunnerLock[i] == AI_NO_LOCK && ...
// L1097: aiBaseRunnerLock[i] = AI_DOUBLE_CLICK_LOCK;

// It does NOT seem to use doubleClickCounter in the batting AI logic block.
// Let's verify searching doubleClickCounter in the file and looking at line numbers.

// But wait, there was logic about doubleClickCounter in actionImplementation (L134).
// And in baseRun (L806).
// aiLogic is just about DECIDING to press keys.
// The doubleClickCounter is about INTERPRETING the keys.
// So aiLogic shouldn't need doubleClickCounter.

void initBattingAI(void)
{
	int i;
	aiBattingKeyDown = 0;
	aiChangingKeyDown = 0;
	aiActionKeyLock = AI_NO_LOCK;
	aiBattingStyle = 0;
	aiRunningBatter = 0;
	aiRunningBaseRunners = 0;

	aiIncreaseKeyDown = 0;
	aiDecreaseKeyDown = 0;
	aiAngleDecided = 0;
	aiDecidedAngle = 0.0f;
	aiWrongPitch = 0;
	aiPlanCalculated = 0;
	aiFirstIndex = -1;
	aiFirstIndexSelected = 0;
	aiChange = 0;
	aiChangeHasHappened = 0;
	for(i = 0; i < BASE_COUNT; i++) {
		aiBaseRunnerKeyDown[i] = 0;
		aiLastSafeOnBaseIndex[i] = -1;
		aiBaseRunnerDecisionMade[i] = 0;
		aiAmountOfClicks[i] = 0;
		aiBaseRunnerLock[i] = AI_NO_LOCK;
		aiClickBreak[i] = 0;
	}
}

void updateBattingAI(StateInfo* stateInfo, unsigned int* rng_seed)
{
	int i;
	int isDoubleClickingOk = 0;
	// update some flags
	for(i = 0; i < BASE_COUNT; i++) {
		aiClickBreak[i]++;
		if(aiClickBreak[i] > 1000) aiClickBreak[i] = 0;
		if(aiBaseRunnerDecisionMade[i] == 1) {
			if(stateInfo->localGameInfo->pII.safeOnBaseIndex[i] == -1 ) {
				aiBaseRunnerDecisionMade[i] = 0;
			}
			if(aiLastSafeOnBaseIndex[i] != stateInfo->localGameInfo->pII.safeOnBaseIndex[i]) {
				aiBaseRunnerDecisionMade[i] = 0;
			}
		}
		aiLastSafeOnBaseIndex[i] = stateInfo->localGameInfo->pII.safeOnBaseIndex[i];
	}
	if(stateInfo->localGameInfo->pRAI.batterReady == 0 && aiPlanCalculated == 1) {
		aiPlanCalculated = 0;
	}
	// make free walk decision == accept
	if(stateInfo->localGameInfo->gameControl.waitingForFreeWalkDecision == 1) {
		if(aiBattingKeyDown == 0) {
			if(aiActionKeyLock == AI_NO_LOCK) {
				stateInfo->keyStates->imitateKeyPress[KEY_2] = 1;
				aiBattingKeyDown = 1;
				aiActionKeyLock = AI_WAITING_WALK_LOCK;
			}
		} else {
			stateInfo->keyStates->imitateKeyPress[KEY_2] = 0;
			aiActionKeyLock = AI_NO_LOCK;
			aiBattingKeyDown = 0;
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

		aiChange = should_change_batter(
		               fieldStatus,
		               stateInfo->localGameInfo->playerInfo[index].bTPI.power,
		               stateInfo->localGameInfo->playerInfo[index].bTPI.speed
		           );

		if(aiFirstIndexSelected == 0) {
			aiFirstIndex = index;
			aiFirstIndexSelected = 1;
		} else if(aiChangeHasHappened == 1) {
			if(aiFirstIndex == index) {
				aiChange = 0;
			}
		}

		// change player
		if(aiChange == 1 && aiChangingKeyDown == 0 && aiActionKeyLock == AI_NO_LOCK) {
			stateInfo->keyStates->imitateKeyPress[KEY_1] = 1;
			aiChangingKeyDown = 1;
			aiActionKeyLock = AI_CHANGE_LOCK;
		} else if(aiChangingKeyDown == 1 && aiActionKeyLock == AI_CHANGE_LOCK) {
			stateInfo->keyStates->imitateKeyPress[KEY_1] = 0;
			aiActionKeyLock = AI_NO_LOCK;
			aiChangingKeyDown = 0;
			aiChangeHasHappened = 1;

		}
		// select best batter.
		if(aiChange == 0 && aiBattingKeyDown == 0 && aiActionKeyLock == AI_NO_LOCK) {
			stateInfo->keyStates->imitateKeyPress[KEY_2] = 1;
			aiBattingKeyDown = 1;
			aiActionKeyLock = AI_WAITING_BATTER_LOCK;
		} else if(aiBattingKeyDown == 1 && aiActionKeyLock == AI_WAITING_BATTER_LOCK) {
			stateInfo->keyStates->imitateKeyPress[KEY_2] = 0;
			aiActionKeyLock = AI_NO_LOCK;
			aiBattingKeyDown = 0;
			aiFirstIndex = -1;
			aiFirstIndexSelected = 0;
			aiChangeHasHappened = 0;

		}

	} else if(stateInfo->localGameInfo->pRAI.batterReady == 1 && stateInfo->localGameInfo->pRAI.pitchInAir == 0 && stateInfo->localGameInfo->gameState.ballHome == 1) {
		// decision tree.. contents can be read within
		if(aiPlanCalculated == 0) {
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

			aiBattingStyle = strategy.style;
			aiRunningBaseRunners = strategy.runBaseRunners;
			aiRunningBatter = strategy.runBatter;

			aiPlanCalculated = 1;
		}
		// if we decide that batter should run, we click down once.
		if(aiRunningBatter == 1) {
			if(aiBaseRunnerDecisionMade[0] == 0 && aiBaseRunnerKeyDown[0] == 0 && aiBaseRunnerLock[0] == AI_NO_LOCK &&
			        aiClickBreak[0] > CLICK_BREAK_CONSTANT) {
				aiBaseRunnerKeyDown[0] = 1;
				aiBaseRunnerLock[0] = AI_CLICK_LOCK;
				stateInfo->keyStates->imitateKeyPress[KEY_DOWN] = 1;
			} else if(aiBaseRunnerKeyDown[0] == 1 && aiBaseRunnerLock[0] == AI_CLICK_LOCK) {
				aiBaseRunnerKeyDown[0] = 0;
				aiBaseRunnerDecisionMade[0] = 1;
				aiClickBreak[0] = 0;
				stateInfo->keyStates->imitateKeyPress[KEY_DOWN] = 0;
				aiBaseRunnerLock[0] = AI_NO_LOCK;
			}
		}
		// if decide that baserunners should run, we click their keys.
		if(aiRunningBaseRunners == 1) {
			int i;
			for(i = 1; i < BASE_COUNT; i++) {
				if(aiBaseRunnerDecisionMade[i] == 0 && stateInfo->localGameInfo->pII.safeOnBaseIndex[i] != -1 &&
				        stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.safeOnBaseIndex[i]].bTPI.state == PLAYER_STATE_SAFE_ON_BASE &&
				        aiBaseRunnerKeyDown[i] == 0 && aiBaseRunnerLock[i] == AI_NO_LOCK && aiClickBreak[i] > CLICK_BREAK_CONSTANT) {
					aiBaseRunnerKeyDown[i] = 1;
					aiBaseRunnerLock[i] = AI_CLICK_LOCK;
					if(i == 1) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 1;
					else if(i == 2) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 1;
					else if(i == 3) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 1;
				} else if(aiBaseRunnerKeyDown[i] == 1 && aiBaseRunnerLock[i] == AI_CLICK_LOCK) {
					aiBaseRunnerKeyDown[i] = 0;
					aiBaseRunnerLock[i] = AI_NO_LOCK;
					aiBaseRunnerDecisionMade[i] = 1;
					aiClickBreak[i] = 0;
					if(i == 1) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 0;
					else if(i == 2) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 0;
					else if(i == 3) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 0;
				}
			}
		}
	}
	// if ball is not home, we return players from first and second base to their bases
	else if(stateInfo->localGameInfo->pRAI.batterReady == 1 && stateInfo->localGameInfo->pRAI.pitchInAir == 0 && stateInfo->localGameInfo->gameState.ballHome == 0) {
		if(aiRunningBaseRunners == 1) {
			int i;
			for(i = 1; i < 3; i++) {
				if(stateInfo->localGameInfo->pII.safeOnBaseIndex[i] != -1 &&
				        stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.safeOnBaseIndex[i]].bTPI.state == PLAYER_STATE_LEADING &&
				        aiBaseRunnerKeyDown[i] == 0 && aiBaseRunnerLock[i] == AI_NO_LOCK && aiClickBreak[i] > CLICK_BREAK_CONSTANT) {
					aiBaseRunnerKeyDown[i] = 1;
					aiBaseRunnerLock[i] = AI_COME_BACK_LOCK;
					if(i == 1) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 1;
					else if(i == 2) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 1;
					else if(i == 3) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 1;
				} else if(aiBaseRunnerKeyDown[i] == 1 && aiBaseRunnerLock[i] == AI_COME_BACK_LOCK) {
					aiBaseRunnerKeyDown[i] = 0;
					aiBaseRunnerDecisionMade[i] = 0;
					aiBaseRunnerLock[i] = AI_NO_LOCK;
					aiClickBreak[i] = 0;
					if(i == 1) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 0;
					else if(i == 2) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 0;
					else if(i == 3) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 0;
				}
			}
		}
	}
	// and here we bat
	else if(stateInfo->localGameInfo->pRAI.pitchInAir == 1) {
		int i;
		// predict if pitch is going to be ball
		if(aiWrongPitch == 0 && is_wrong_pitch(
		            stateInfo->localGameInfo->ballInfo.velocity.x,
		            stateInfo->localGameInfo->ballInfo.velocity.y,
		            GRAVITY,
		            PLATE_WIDTH
		        )) {
			aiWrongPitch = 1;
		}
		if(aiWrongPitch == 1) {
			// batter isnt handled here
			// this code will make baserunners come back if wrong pitch is pitched
			for(i = 1; i < BASE_COUNT; i++) {
				int index = stateInfo->localGameInfo->pII.safeOnBaseIndex[i];
				if(index != -1 && stateInfo->localGameInfo->playerRuntime[index].goingForward == 1 && aiBaseRunnerKeyDown[i] == 0 &&
				        aiBaseRunnerLock[i] == AI_NO_LOCK && aiClickBreak[i] > CLICK_BREAK_CONSTANT) {
					aiBaseRunnerKeyDown[i] = 1;
					aiBaseRunnerLock[i] = AI_COME_BACK_WRONG_PITCH_LOCK;
					if(i == 1) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 1;
					else if(i == 2) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 1;
					else if(i == 3) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 1;
				} else if(aiBaseRunnerKeyDown[i] == 1 && aiBaseRunnerLock[i] == AI_COME_BACK_WRONG_PITCH_LOCK) {
					aiBaseRunnerKeyDown[i] = 0;
					aiBaseRunnerDecisionMade[i] = 0;
					aiBaseRunnerLock[i] = AI_NO_LOCK;
					aiClickBreak[i] = 0;
					if(i == 1) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 0;
					else if(i == 2) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 0;
					else if(i == 3) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 0;
				}
			}
		}
		// a bunt
		if(aiBattingStyle == 0) {
			if(aiAngleDecided == 0) {
				aiDecidedAngle = calculate_ai_batting_angle(0, -1, seeded_rand(rng_seed, RAND_MAX));
				aiAngleDecided = 1;
			}
			if(meterCounter > BAT_SWING_MAX - 23 && aiBattingKeyDown == 0 && aiActionKeyLock == AI_NO_LOCK && aiWrongPitch == 0) {
				stateInfo->keyStates->imitateKeyPress[KEY_2] = 1;
				aiBattingKeyDown = 1;
				aiActionKeyLock = AI_BATTING_LOCK;
			} else if(aiBattingKeyDown == 1 && aiActionKeyLock == AI_BATTING_LOCK) {
				if(meterCounter > BAT_LOAD_MAX - 9) {
					stateInfo->keyStates->imitateKeyPress[KEY_2] = 0;
					aiBattingKeyDown = 0;
					aiActionKeyLock = AI_NO_LOCK;
				}
			}
		}
		// a normal swing
		else if(aiBattingStyle == 1) {
			if(aiAngleDecided == 0) {
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
				aiDecidedAngle = calculate_ai_batting_angle(1, leadBase, seeded_rand(rng_seed, RAND_MAX));
				aiAngleDecided = 1;
			}
			if(meterCounter > BAT_SWING_MAX - 10 && aiBattingKeyDown == 0 && aiActionKeyLock == AI_NO_LOCK && aiWrongPitch == 0) {
				stateInfo->keyStates->imitateKeyPress[KEY_2] = 1;
				aiBattingKeyDown = 1;
				aiActionKeyLock = AI_BATTING_LOCK;
			} else if(aiBattingKeyDown == 1 && aiActionKeyLock == AI_BATTING_LOCK) {
				if(meterCounter > BAT_LOAD_MAX - 6) {
					stateInfo->keyStates->imitateKeyPress[KEY_2] = 0;
					aiBattingKeyDown = 0;
					aiActionKeyLock = AI_NO_LOCK;
				}
			}
		}
		// swing that tries to get oneself wounded
		else if(aiBattingStyle == 2) {
			if(aiAngleDecided == 0) {
				aiDecidedAngle = calculate_ai_batting_angle(2, -1, seeded_rand(rng_seed, RAND_MAX));
				aiAngleDecided = 1;
			}
			if(meterCounter > BAT_SWING_MAX - 11 && aiBattingKeyDown == 0 && aiActionKeyLock == AI_NO_LOCK && aiWrongPitch == 0) {
				stateInfo->keyStates->imitateKeyPress[KEY_2] = 1;
				aiBattingKeyDown = 1;
				aiActionKeyLock = AI_BATTING_LOCK;
			} else if(aiBattingKeyDown == 1 && aiActionKeyLock == AI_BATTING_LOCK) {
				if(meterCounter > BAT_LOAD_MAX - 8) {
					stateInfo->keyStates->imitateKeyPress[KEY_2] = 0;
					aiBattingKeyDown = 0;
					aiActionKeyLock = AI_NO_LOCK;
				}
			}
		}
		if(aiDecidedAngle >= 0 && batterAngle < aiDecidedAngle && aiIncreaseKeyDown == 0) {
			stateInfo->keyStates->imitateKeyPress[KEY_PLUS] = 1;
			aiIncreaseKeyDown = 1;
		} else if(batterAngle >= aiDecidedAngle && aiIncreaseKeyDown == 1) {
			stateInfo->keyStates->imitateKeyPress[KEY_PLUS] = 0;
			aiIncreaseKeyDown = 0;
		}

		if(aiDecidedAngle < 0 && batterAngle > aiDecidedAngle && aiDecreaseKeyDown == 0) {
			stateInfo->keyStates->imitateKeyPress[KEY_MINUS] = 1;
			aiDecreaseKeyDown = 1;
		} else if(batterAngle <= aiDecidedAngle && aiDecreaseKeyDown == 1) {
			stateInfo->keyStates->imitateKeyPress[KEY_MINUS] = 0;
			aiDecreaseKeyDown = 0;
		}

	}
	if(stateInfo->localGameInfo->pRAI.pitchInAir == 0 && aiAngleDecided == 1) {
		aiAngleDecided = 0;
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
		if(shouldRun == 1 && isDoubleClickingOk == 1 && aiBaseRunnerLock[i] == AI_NO_LOCK &&
		        aiBaseRunnerKeyDown[i] == 0 && index != -1 && stateInfo->localGameInfo->playerRuntime[index].goingForward != 1 &&
		        aiAmountOfClicks[i] == 0 && aiClickBreak[i] > CLICK_BREAK_CONSTANT) {
			aiBaseRunnerKeyDown[i] = 1;
			aiBaseRunnerLock[i] = AI_DOUBLE_CLICK_LOCK;
			if(i == 0) stateInfo->keyStates->imitateKeyPress[KEY_DOWN] = 1;
			else if(i == 1) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 1;
			else if(i == 2) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 1;
			else if(i == 3) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 1;

		} else if(aiBaseRunnerKeyDown[i] == 0 && aiBaseRunnerLock[i] == AI_DOUBLE_CLICK_LOCK && aiClickBreak[i] > CLICK_BREAK_CONSTANT) {
			aiBaseRunnerKeyDown[i] = 1;
			if(i == 0) stateInfo->keyStates->imitateKeyPress[KEY_DOWN] = 1;
			else if(i == 1) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 1;
			else if(i == 2) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 1;
			else if(i == 3) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 1;
		} else if(aiBaseRunnerKeyDown[i] == 1 && aiBaseRunnerLock[i] == AI_DOUBLE_CLICK_LOCK) {
			aiBaseRunnerKeyDown[i] = 0;
			if(aiAmountOfClicks[i] == 1) {
				aiBaseRunnerLock[i] = AI_NO_LOCK;
				aiAmountOfClicks[i] = 0;
			} else {
				aiAmountOfClicks[i]++;
			}
			aiClickBreak[i] = 0;

			if(i == 0) stateInfo->keyStates->imitateKeyPress[KEY_DOWN] = 0;
			else if(i == 1) stateInfo->keyStates->imitateKeyPress[KEY_LEFT] = 0;
			else if(i == 2) stateInfo->keyStates->imitateKeyPress[KEY_RIGHT] = 0;
			else if(i == 3) stateInfo->keyStates->imitateKeyPress[KEY_UP] = 0;
		}

	}
}
