/*
	So this file specifically handles user-initiated events. This works in conjuction
	with action_invocations.c where all the input reading is done. This has also some
	dirty floating point code, but it should be closely related to something that happens
	after user presses some keys. Like batting or throwing or running.
*/

#include "globals.h"
#include "action_implementation.h"
#include "common_logic.h"
#include "actions_messy/pitching_system.h"
#include "actions_messy/batting_system.h"
#include "actions_messy/throwing_system.h"
#include "ai_messy/catching_ai.h"
#include "ai_messy/batting_ai.h"
#include "base_logic.h"
#include "base_control.h"

#define ANIMATION_FREQUENCY 3

#define CLICK_BREAK_CONSTANT 3

static void changeBatter(StateInfo* stateInfo);
static void takeFreeWalkDecision(StateInfo* stateInfo);
static void baseRun(StateInfo* stateInfo, BaseID base);;
static void updateMeters(StateInfo* stateInfo);
static void aiLogic(StateInfo* stateInfo, unsigned int* rng_seed);

void initActionImplementation(StateInfo* stateInfo)
{
	// just initialize everyone of these static variables to zero
	int i;

	stateInfo->localGameInfo->pendingActionState.meterCounter = 0;
	stateInfo->localGameInfo->pendingActionState.meterCounterMax = 0;
	for(i = 0; i < BASE_COUNT; i++) {
		stateInfo->localGameInfo->pendingActionState.doubleClickCounter[i] = -1;
	}

	resetPitchingSystem(stateInfo);
	initBattingSystem(stateInfo);
	initThrowingSystem(stateInfo);
	stateInfo->localGameInfo->pendingActionState.runBatFlag = 0;

	//ai uses a few flags..

	initCatchingAI(&(stateInfo->localGameInfo->aiState));
	stateInfo->localGameInfo->pendingActionState.aiActionEventLock = -1;
	stateInfo->localGameInfo->pendingActionState.aiLockUpdate = 0;

	initBattingAI(&(stateInfo->localGameInfo->aiState));
}

void actionImplementation(StateInfo* stateInfo, unsigned int* rng_seed)
{
	int i;

	// double click counter
	for(i = 0; i < BASE_COUNT; i++) {
		if(stateInfo->localGameInfo->pendingActionState.doubleClickCounter[i] >= 0) {
			stateInfo->localGameInfo->pendingActionState.doubleClickCounter[i]++;
			if(stateInfo->localGameInfo->pendingActionState.doubleClickCounter[i] >= 20) {
				stateInfo->localGameInfo->pendingActionState.doubleClickCounter[i] = -1;
			}
		}
	}

	/*
	 * CATCHING TEAM
	 */

	for(i = 0; i < BASE_COUNT; i++) {
		// for every direction we check if throw key has been pressed
		if(stateInfo->localGameInfo->aF.cTAF.throwToBase[i] == ACTION_TRIGGER_START) {
			int throwNotReleasingYet = 1;
			int j;
			for(j = 0; j < BASE_COUNT; j++) {
				if(stateInfo->localGameInfo->aF.cTAF.throwToBase[i] >= ACTION_TRIGGER_STOP) {
					throwNotReleasingYet = 0;
				}
			}
			// can throw only if someone has the ball and no throw is already going on
			if(throwNotReleasingYet == 1 && stateInfo->localGameInfo->pII.hasBallIndex != -1) {
				for(j = 0; j < BASE_COUNT; j++) {
					if(j != i) stateInfo->localGameInfo->aF.cTAF.throwToBase[j] = ACTION_IDLE;
				}
				// stop pitching if throwing
				if(stateInfo->localGameInfo->pRAI.pitchState != PITCH_STAGE_NONE) {
					stateInfo->localGameInfo->aF.cTAF.pitch = PITCH_ACTION_IDLE;
					stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 0;
					stateInfo->localGameInfo->pRAI.pitchState = PITCH_STAGE_NONE;
					// when pitching the ball is moved to the center of the plate so now when we are terminating the pitch
					// to throw, we must move the ball back to the player
					stateInfo->localGameInfo->ballInfo.location.x = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.x;
					stateInfo->localGameInfo->ballInfo.location.z = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.z;
				}
				// throwGoingToBase variables are used to have better control
				// over basemen who are wanting go out of base catching the ball.
				// throws can be directed only towards bases.
				prepareThrow(stateInfo, i);
				// start by loading
				genericThrowLoad(stateInfo, i);
			} else {
				// if no luck, then set throwToBase to one so that can try again
				stateInfo->localGameInfo->aF.cTAF.throwToBase[i] = ACTION_IDLE;
				stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 0;
			}
		}
		// if already on release phase, then continue with that and
		// set the throwToBase to zero so that one can start trying to throw again
		// immediately
		else if(stateInfo->localGameInfo->aF.cTAF.throwToBase[i] == ACTION_TRIGGER_STOP) {
			stateInfo->localGameInfo->aF.cTAF.throwToBase[i] = ACTION_IDLE;
			stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 0;
			genericThrowRelease(stateInfo);
		}



	}
	// if move keys have been pressed, depending on if its down or release
	// call corresponding function for every direction
	for(i = 0; i < DIRECTION_COUNT; i++) {
		if(stateInfo->localGameInfo->aF.cTAF.move[i] == ACTION_TRIGGER_START) {
			genericMove(stateInfo, i);
		} else if(stateInfo->localGameInfo->aF.cTAF.move[i] == ACTION_TRIGGER_STOP) {
			genericStopMove(stateInfo, i);
		}
	}

	//if change player key has been pressed
	if(stateInfo->localGameInfo->aF.cTAF.changePlayer == ACTION_TRIGGER_START) {
		// no one must have the ball
		if(stateInfo->localGameInfo->pII.hasBallIndex == -1) {
			// we go to next element in changePlayerArray.
			stateInfo->localGameInfo->pII.changePlayerArrayIndex =
			    (stateInfo->localGameInfo->pII.changePlayerArrayIndex + 1) % CHANGE_PLAYER_COUNT;
			// and try to ensure that there is difference. we dont want to end up in a endless loop
			// though so we do it only once.
			if(stateInfo->localGameInfo->pII.controlIndex ==
			        stateInfo->localGameInfo->pII.fielderRankedIndices[stateInfo->localGameInfo->pII.changePlayerArrayIndex]) {
				stateInfo->localGameInfo->pII.changePlayerArrayIndex =
				    (stateInfo->localGameInfo->pII.changePlayerArrayIndex + 1) % CHANGE_PLAYER_COUNT;
			}
			// and then set the flag, so that other parts of code can handle
			// the job
			changePlayer(stateInfo->localGameInfo);
		}
		stateInfo->localGameInfo->aF.cTAF.changePlayer = ACTION_IDLE;
		stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 0;
	}
	// if drop ball key has been pressed, try dropping
	if(stateInfo->localGameInfo->aF.cTAF.dropBall == ACTION_TRIGGER_START) {
		dropBall(stateInfo);
	}
	// pitching
	if(stateInfo->localGameInfo->aF.cTAF.pitch == PITCH_ACTION_START) {
		startPitch(stateInfo);
	} else if(stateInfo->localGameInfo->aF.cTAF.pitch == PITCH_ACTION_POWER_SET) {
		continuePitch(stateInfo);
	} else if(stateInfo->localGameInfo->aF.cTAF.pitch == PITCH_ACTION_ANGLE_SET) {
		releasePitch(stateInfo);
	}
	/*
	 * BATTING TEAM
	 */
	// when there's no batter, user is prompted to select the next batter
	if(stateInfo->localGameInfo->aF.bTAF.chooseBatter == CHOOSE_BATTER_NEXT) {
		changeBatter(stateInfo);
	} else if(stateInfo->localGameInfo->aF.bTAF.chooseBatter == CHOOSE_BATTER_SELECT) {
		selectBatter(stateInfo);
	}
	// free walk decisions, takeFreeWalk can be 0, 1 or 2. if its 2
	// takeFreeWalkDecision() is called but will basically just set takeFreeWalk to 0.
	if(stateInfo->localGameInfo->aF.bTAF.takeFreeWalk > FREE_WALK_IDLE) {
		takeFreeWalkDecision(stateInfo);
	}
	// batter angles
	if(stateInfo->localGameInfo->aF.bTAF.increaseBatterAngle == ACTION_TRIGGER_START) {
		startIncreaseBatterAngle(stateInfo);
	} else if(stateInfo->localGameInfo->aF.bTAF.increaseBatterAngle == ACTION_TRIGGER_STOP) {
		stopIncreaseBatterAngle(stateInfo);
	}
	if(stateInfo->localGameInfo->aF.bTAF.decreaseBatterAngle == ACTION_TRIGGER_START) {
		startDecreaseBatterAngle(stateInfo);
	} else if(stateInfo->localGameInfo->aF.bTAF.decreaseBatterAngle == ACTION_TRIGGER_STOP) {
		stopDecreaseBatterAngle(stateInfo);
	}
	// batting
	if(stateInfo->localGameInfo->aF.bTAF.swing == BAT_ACTION_POWER_SET) {
		selectPower(stateInfo);
	} else if(stateInfo->localGameInfo->aF.bTAF.swing == BAT_ACTION_ANGLE_SET) {
		selectAngle(stateInfo);
	}
	// baserunners must be able to run!
	for(i = 0; i < BASE_COUNT; i++) {
		baseRun(stateInfo, i);
	}
	// this is used to handle a lot of stuff happening between and after the decisions.
	updateBatting(stateInfo);

	/*
	* COMMON
	*/
	// meters need to be updated for the logic and for the screen.
	updateMeters(stateInfo);
	aiLogic(stateInfo, rng_seed);
}

static void takeFreeWalkDecision(StateInfo* stateInfo)
{
	if(stateInfo->localGameInfo->aF.bTAF.takeFreeWalk == FREE_WALK_ACCEPT) {
		int index = stateInfo->localGameInfo->gameControl.freeWalkIndex;
		BaseID base = stateInfo->localGameInfo->gameControl.freeWalkBase;
		if(index != -1) {
			// there can be a little gap between the decision and when the possibility to decide came
			// so player might have run already to the following base, and free walk actually
			// gave him the right to go to just that base.
			// so if he still has the same base as before we can go on
			if(stateInfo->globalGameInfo->period >= 4) {
				// REFEREE MIGRATION: Logic moved to referee.c
				// We just signal the event here.
				stateInfo->localGameInfo->gameEvents.freeWalkAccepted = 1;

			} else {
				BaseID currentBaseId = stateInfo->localGameInfo->playerInfo[index].bTPI.baseId;

				if(currentBaseId == base) {
					// we start running to the next base
					runToNextBase(stateInfo->localGameInfo, stateInfo->fieldPositions, index, base);

					// set takingFreeWalk flag so that this player cant get wounded or tagged
					// when he's running
					stateInfo->localGameInfo->playerInfo[index].bTPI.state = PLAYER_STATE_ADVANCING_FREELY;
					// if he's safe on previous base, set the baseControlIndex for that base to -1
					// if he was batter, set the batterIndex to -1 so that we can have a new batter.
					if(stateInfo->localGameInfo->pII.batterIndex == index) {
						stateInfo->localGameInfo->pII.batterIndex = -1;
					}
				}
				// REFEREE MIGRATION: Logic moved to referee.c
				// We just signal the event here.
				stateInfo->localGameInfo->gameEvents.freeWalkAccepted = 1;
			}
		}
	}
	// no more decision to make.
	stateInfo->localGameInfo->gameControl.waitingForFreeWalkDecision = 0;
	stateInfo->localGameInfo->aF.bTAF.takeFreeWalk = FREE_WALK_IDLE;
}
// so when there is no batter and few other conditions hold
// we can select the batter from one player from the normal ordering of players and three joker players
static void changeBatter(StateInfo* stateInfo)
{
	int done = 0;
	int counter = 0;
	// index in a teams[] array
	int battingTeamIndex = (stateInfo->globalGameInfo->
	                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
	int index;

	stateInfo->localGameInfo->aF.bTAF.chooseBatter = 0;
	// batterSelect variable will point to the current player in selection
	// and now as we are changing the selection, we add one to it.
	stateInfo->localGameInfo->pendingActionState.batterSelect++;
	// here we have a loop that basically just searches through the possible players and selects
	// the next one. batterSelect == 0 indicates that it is a normal player, batterSelect != 0 indicates
	// it is a joker player.
	// there must be at least one player as this function cannot get called without
	// waitingForBatterDecision-flag, and that can flag cant be true if
	// there is not at least one player.
	while(done == 0) {
		if(stateInfo->localGameInfo->pendingActionState.batterSelect == 0) {
			if(stateInfo->localGameInfo->playerCounters.nonJokerPlayersLeft != 0) done = 1;
			else stateInfo->localGameInfo->pendingActionState.batterSelect = 1;
		} else if(stateInfo->localGameInfo->pendingActionState.batterSelect == 4) {
			if(stateInfo->localGameInfo->playerCounters.nonJokerPlayersLeft != 0) {
				stateInfo->localGameInfo->pendingActionState.batterSelect = 0;
				done = 1;
			} else stateInfo->localGameInfo->pendingActionState.batterSelect = 1;

		} else {
			if(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->
			                                        pII.jokerIndices[stateInfo->localGameInfo->pendingActionState.batterSelect - 1]].bTPI.joker == JOKER_USED) stateInfo->localGameInfo->pendingActionState.batterSelect++;
			else done = 1;

		}
		if(counter == 4) done = 1;
		counter++;
	}
	// now we have the batterSelect value and we just need to find a corresponding index for that
	// player.
	if(stateInfo->localGameInfo->pendingActionState.batterSelect == 0) {
		index = stateInfo->globalGameInfo->teams[battingTeamIndex].batterOrder[stateInfo->globalGameInfo->teams[battingTeamIndex].batterOrderIndex];
	} else {
		index = stateInfo->localGameInfo->pII.jokerIndices[stateInfo->localGameInfo->pendingActionState.batterSelect - 1];
	}
	// and set it here.
	stateInfo->localGameInfo->pII.batterSelectionIndex = index;
}

void genericSlingBall(BallInfo* ballInfo, float x, float y, float z)
{
// Make ball visible and moving
	ballInfo->visible = 1;
	ballInfo->moving = 1;

// Set the velocity
	setVectorXYZ(&(ballInfo->velocity), x, y, z);
}


// so baserunning.
// idea is just to update willStartRunning in every button press. and in special double click case we just run.
static void baseRun(StateInfo* stateInfo, BaseID base)
{
	// so baserunning.
	// idea is just to update willStartRunning in every button press. and in special double click case we just run.
	if(get_base_controller(stateInfo->localGameInfo, base) != -1) {
		if(stateInfo->localGameInfo->aF.bTAF.baseRun[base] == ACTION_TRIGGER_START) {
			int index = get_base_controller(stateInfo->localGameInfo, base);
			if(stateInfo->localGameInfo->playerInfo[index].bTPI.state == PLAYER_STATE_ON_BASE ||
			        stateInfo->localGameInfo->playerInfo[index].bTPI.state == PLAYER_STATE_AT_BAT) {
				if(stateInfo->localGameInfo->pRAI.willStartRunning[base] == 0) {
					if(index != -1 && stateInfo->localGameInfo->playerInfo[index].cPI.moving == 0) {
						stateInfo->localGameInfo->pRAI.willStartRunning[base] = 1;
						if(base == BASE_FIRST || base == BASE_SECOND) {
							lead(stateInfo->localGameInfo->playerInfo, stateInfo->localGameInfo->playerRuntime, stateInfo->fieldPositions, index);
						}
					}
				} else {
					stateInfo->localGameInfo->pRAI.willStartRunning[base] = 0;
				}
			} else {
				stateInfo->localGameInfo->pRAI.willStartRunning[base] = 0;
				if(index != -1) {
					if(stateInfo->localGameInfo->playerInfo[index].bTPI.state != PLAYER_STATE_ON_BASE &&
					        stateInfo->localGameInfo->playerInfo[index].bTPI.state != PLAYER_STATE_AT_BAT) {
						runToPreviousBase(stateInfo->localGameInfo, stateInfo->fieldPositions, index, base);
					}
				}
			}
			if(stateInfo->localGameInfo->pendingActionState.doubleClickCounter[base] == -1) {
				stateInfo->localGameInfo->pendingActionState.doubleClickCounter[base] = 0;
			} else {
				if(stateInfo->localGameInfo->pendingActionState.doubleClickCounter[base] >= 0) {
					if(index != -1) {
						runToNextBase(stateInfo->localGameInfo, stateInfo->fieldPositions, index, base);
					}

				}
				stateInfo->localGameInfo->pendingActionState.doubleClickCounter[base] = -1;
			}
		}

	}
	stateInfo->localGameInfo->aF.bTAF.baseRun[base] = ACTION_IDLE;
}

static void updateMeters(StateInfo* stateInfo)
{
	updatePitchingMeter(stateInfo);

	if(stateInfo->localGameInfo->pendingActionState.throwGoingOn == 1) {
		if(stateInfo->localGameInfo->pendingActionState.meterCounter < stateInfo->localGameInfo->pendingActionState.meterCounterMax) {
			stateInfo->localGameInfo->pendingActionState.meterCounter += 1;
		}
		stateInfo->localGameInfo->pRAI.meterValue = 1.0f*stateInfo->localGameInfo->pendingActionState.meterCounter / stateInfo->localGameInfo->pendingActionState.meterCounterMax;
	} else {
		updateBattingMeter(stateInfo);
	}
}

static void aiLogic(StateInfo* stateInfo, unsigned int* rng_seed)
{
	int battingTeamIndex = (stateInfo->globalGameInfo->
	                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
	TeamControlMode battingControl = stateInfo->globalGameInfo->teams[battingTeamIndex].control;
	TeamControlMode catchingControl = stateInfo->globalGameInfo->teams[(battingTeamIndex+1)%2].control;

	// first ai for catching team

	if(catchingControl == CONTROL_AI) {
		updateCatchingAI(stateInfo, rng_seed);
	}
	// then ai for batting team
	if(battingControl == CONTROL_AI) {
		updateBattingAI(stateInfo, rng_seed);
	}

}