/*
	So this file specifically handles user-initiated events. This works in conjuction
	with action_invocations.c where all the input reading is done. This has also some
	dirty floating point code, but it should be closely related to something that happens
	after user presses some keys. Like batting or throwing or running.
*/

#include "globals.h"
#include "action_implementation.h"
#include "common_logic.h"
#include "actions_messy/action_state.h"
#include "actions_messy/pitching_system.h"
#include "actions_messy/batting_system.h"
#include "actions_messy/throwing_system.h"
#include "ai_messy/catching_ai.h"
#include "ai_messy/batting_ai.h"

#define ANIMATION_FREQUENCY 3

#define CLICK_BREAK_CONSTANT 3

static int doubleClickCounter[BASE_COUNT];

static void changeBatter(StateInfo* stateInfo);
static void takeFreeWalkDecision(StateInfo* stateInfo);
static void baseRun(StateInfo* stateInfo, int base);
static void updateMeters(StateInfo* stateInfo);
static void aiLogic(StateInfo* stateInfo, unsigned int* rng_seed);

void initActionImplementation(StateInfo* stateInfo)
{
	// just initialize everyone of these static variables to zero
	int i;

	meterCounter = 0;
	meterCounterMax = 0;
	for(i = 0; i < BASE_COUNT; i++) {
		doubleClickCounter[i] = -1;
	}

	resetPitchingSystem();
	initBattingSystem();
	initThrowingSystem();
	runBatFlag = 0;

	//ai uses a few flags..

	initCatchingAI();
	aiActionEventLock = -1;
	aiLockUpdate = 0;

	initBattingAI();

	flushKeys(stateInfo);
}

void actionImplementation(StateInfo* stateInfo, unsigned int* rng_seed)
{
	int i;
	// init?
	if(stateInfo->localGameInfo->gAI.initLocals > 0) {
		initActionImplementation(stateInfo);
		stateInfo->localGameInfo->gAI.initLocals++;
		if(stateInfo->localGameInfo->gAI.initLocals == INIT_LOCALS_COUNT) {
			stateInfo->localGameInfo->gAI.initLocals = 0;
		}
	}

	// double click counter
	for(i = 0; i < BASE_COUNT; i++) {
		if(doubleClickCounter[i] >= 0) {
			doubleClickCounter[i]++;
			if(doubleClickCounter[i] >= 20) {
				doubleClickCounter[i] = -1;
			}
		}
	}

	/*
	 * CATCHING TEAM
	 */

	for(i = 0; i < BASE_COUNT; i++) {
		// for every direction we check if throw key has been pressed
		if(stateInfo->localGameInfo->aF.cTAF.throwToBase[i] == 1) {
			int throwNotReleasingYet = 1;
			int j;
			for(j = 0; j < BASE_COUNT; j++) {
				if(stateInfo->localGameInfo->aF.cTAF.throwToBase[i] >= 3) {
					throwNotReleasingYet = 0;
				}
			}
			// can throw only if someone has the ball and no throw is already going on
			if(throwNotReleasingYet == 1 && stateInfo->localGameInfo->pII.hasBallIndex != -1) {
				for(j = 0; j < BASE_COUNT; j++) {
					if(j != i) stateInfo->localGameInfo->aF.cTAF.throwToBase[j] = 0;
				}
				// stop pitching if throwing
				if(stateInfo->localGameInfo->pRAI.pitchGoingOn == 1) {
					stateInfo->localGameInfo->aF.cTAF.pitch = 0;
					stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 0;
					stateInfo->localGameInfo->pRAI.pitchGoingOn = 0;
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
				stateInfo->localGameInfo->aF.cTAF.throwToBase[i] = 0;
				stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 0;
			}
		}
		// if already on release phase, then continue with that and
		// set the throwToBase to zero so that one can start trying to throw again
		// immediately
		else if(stateInfo->localGameInfo->aF.cTAF.throwToBase[i] == 3) {
			stateInfo->localGameInfo->aF.cTAF.throwToBase[i] = 0;
			stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 0;
			genericThrowRelease(stateInfo);
		}



	}
	// if move keys have been pressed, depending on if its down or release
	// call corresponding function for every direction
	for(i = 0; i < DIRECTION_COUNT; i++) {
		if(stateInfo->localGameInfo->aF.cTAF.move[i] == 1) {
			genericMove(stateInfo, i);
		} else if(stateInfo->localGameInfo->aF.cTAF.move[i] == 3) {
			genericStopMove(stateInfo, i);
		}
	}

	//if change player key has been pressed
	if(stateInfo->localGameInfo->aF.cTAF.changePlayer == 1) {
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
			changePlayer(stateInfo);
		}
		stateInfo->localGameInfo->aF.cTAF.changePlayer = 0;
		stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 0;
	}
	// if drop ball key has been pressed, try dropping
	if(stateInfo->localGameInfo->aF.cTAF.dropBall == 1) {
		dropBall(stateInfo);
	}
	// pitching
	if(stateInfo->localGameInfo->aF.cTAF.pitch == 1) {
		startPitch(stateInfo);
	} else if(stateInfo->localGameInfo->aF.cTAF.pitch == 3) {
		continuePitch(stateInfo);
	} else if(stateInfo->localGameInfo->aF.cTAF.pitch == 5) {
		releasePitch(stateInfo);
	}
	/*
	 * BATTING TEAM
	 */
	// when there's no batter, user is prompted to select the next batter
	if(stateInfo->localGameInfo->aF.bTAF.chooseBatter == 1) {
		changeBatter(stateInfo);
	} else if(stateInfo->localGameInfo->aF.bTAF.chooseBatter == 2) {
		selectBatter(stateInfo);
	}
	// free walk decisions, takeFreeWalk can be 0, 1 or 2. if its 2
	// takeFreeWalkDecision() is called but will basically just set takeFreeWalk to 0.
	if(stateInfo->localGameInfo->aF.bTAF.takeFreeWalk > 0) {
		takeFreeWalkDecision(stateInfo);
	}
	// batter angles
	if(stateInfo->localGameInfo->aF.bTAF.increaseBatterAngle == 1) {
		startIncreaseBatterAngle(stateInfo);
	} else if(stateInfo->localGameInfo->aF.bTAF.increaseBatterAngle == 3) {
		stopIncreaseBatterAngle(stateInfo);
	}
	if(stateInfo->localGameInfo->aF.bTAF.decreaseBatterAngle == 1) {
		startDecreaseBatterAngle(stateInfo);
	} else if(stateInfo->localGameInfo->aF.bTAF.decreaseBatterAngle == 3) {
		stopDecreaseBatterAngle(stateInfo);
	}
	// batting
	if(stateInfo->localGameInfo->aF.bTAF.swing == 2) {
		selectPower(stateInfo);
	} else if(stateInfo->localGameInfo->aF.bTAF.swing == 4) {
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
	// so if user selected to take a free walk, this will happen. otherwise we just set freewalk-actionflag
	// to 0 and dont have any further actions
	if(stateInfo->localGameInfo->aF.bTAF.takeFreeWalk == 1) {
		// index and base have been selected before. they are the lead runner 's base and index when
		// to decision opportunity came available
		int index = stateInfo->localGameInfo->gAI.freeWalkIndex;
		int base = stateInfo->localGameInfo->gAI.freeWalkBase;
		if(index != -1) {
			// there can be a little gap between the decision and when the possibility to decide came
			// so player might have run already to the following base, and free walk actually
			// gave him the right to go to just that base.
			// so if he still has the same base as before we can go on
			if(stateInfo->globalGameInfo->period >= 4) {
				// for a guy who is at the third base, originalBase will be 4
				int battingTeamIndex = (stateInfo->globalGameInfo->
				                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
				int catchingTeamIndex = (battingTeamIndex+1)%2;
				stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase = 4;

				// add a run
				stateInfo->globalGameInfo->teams[battingTeamIndex].runs += 1;
				stateInfo->localGameInfo->gAI.runsInTheInning += 1;

				if(stateInfo->localGameInfo->gAI.balls >= 3) {
					stateInfo->globalGameInfo->teams[battingTeamIndex].runs += 1;
					stateInfo->localGameInfo->gAI.runsInTheInning += 1;
					stateInfo->localGameInfo->gAI.gameInfoEvent = 9;
					stateInfo->localGameInfo->gAI.event = EVENT_TWO_RUNS_SCORED;
				} else {
					// set info to screen
					stateInfo->localGameInfo->gAI.gameInfoEvent = 3;
					stateInfo->localGameInfo->gAI.event = EVENT_RUN_SCORED;
				}

				if((stateInfo->globalGameInfo->inning + 1)%2 == 0) {
					if(stateInfo->globalGameInfo->teams[battingTeamIndex].runs >
					        stateInfo->globalGameInfo->teams[catchingTeamIndex].runs) {
						stateInfo->localGameInfo->gAI.endPeriod = 1;
					}
				}
				stateInfo->localGameInfo->gAI.forceNextPair = 1;
			} else {

				if(stateInfo->localGameInfo->playerInfo[index].bTPI.base == base) {
					// we start running to the next base
					runToNextBase(stateInfo, index, base);

					// set takingFreeWalk flag so that this player cant get wounded or tagged
					// when he's running
					stateInfo->localGameInfo->playerInfo[index].bTPI.state = PLAYER_STATE_ADVANCING_FREELY;
					stateInfo->localGameInfo->playerInfo[index].bTPI.takingFreeWalk = 1;
					// if he's safe on previous base, set the safeOnBaseIndex for that base to -1
					if(stateInfo->localGameInfo->pII.safeOnBaseIndex[stateInfo->localGameInfo->playerInfo[index].bTPI.base] == index) {
						stateInfo->localGameInfo->pII.safeOnBaseIndex[stateInfo->localGameInfo->playerInfo[index].bTPI.base] = -1;
					}
					// if he was batter, set the batterIndex to -1 so that we can have a new batter.
					if(stateInfo->localGameInfo->pII.batterIndex == index) {
						stateInfo->localGameInfo->pII.batterIndex = -1;
					}
				}
				// we also set here the originalBase for freewalkers to be the following base, so that
				// in out of bounds situations these players will be at correct bases in post foul play world
				if(base != 3) {
					stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase =
					    base + 1;
				} else {
					// for a guy who is at the third base, originalBase will be 4
					int battingTeamIndex = (stateInfo->globalGameInfo->
					                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
					int catchingTeamIndex = (battingTeamIndex+1)%2;
					stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase = 4;

					// add a run
					stateInfo->globalGameInfo->teams[battingTeamIndex].runs += 1;
					stateInfo->localGameInfo->gAI.runsInTheInning += 1;
					if(stateInfo->localGameInfo->gAI.runsInTheInning%2 == 0) {
						stateInfo->localGameInfo->gAI.nonJokerPlayersLeft = PLAYERS_IN_TEAM;
						stateInfo->localGameInfo->gAI.noMorePlayers = 0;
					}
					// set info to screen
					stateInfo->localGameInfo->gAI.gameInfoEvent = 3;
					stateInfo->localGameInfo->gAI.event = EVENT_RUN_SCORED;

					if((stateInfo->globalGameInfo->inning + 1)%stateInfo->globalGameInfo->halfInningsInPeriod == 0 ||
					        stateInfo->globalGameInfo->inning + 1 == stateInfo->globalGameInfo->halfInningsInPeriod*2 + 2) {
						if(stateInfo->globalGameInfo->teams[battingTeamIndex].runs >
						        stateInfo->globalGameInfo->teams[catchingTeamIndex].runs) {
							stateInfo->localGameInfo->gAI.endPeriod = 1;
						}
						if(stateInfo->globalGameInfo->inning + 1 == stateInfo->globalGameInfo->halfInningsInPeriod*2 &&
						        stateInfo->globalGameInfo->teams[battingTeamIndex].period0Runs >
						        stateInfo->globalGameInfo->teams[catchingTeamIndex].period0Runs &&
						        stateInfo->globalGameInfo->teams[catchingTeamIndex].runs ==
						        stateInfo->globalGameInfo->teams[battingTeamIndex].runs ) {
							stateInfo->localGameInfo->gAI.endPeriod = 1;
						}
					}
				}
			}
		}
	}
	// no more decision to make.
	stateInfo->localGameInfo->gAI.waitingForFreeWalkDecision = 0;
	stateInfo->localGameInfo->aF.bTAF.takeFreeWalk = 0;
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
	batterSelect++;
	// here we have a loop that basically just searches through the possible players and selects
	// the next one. batterSelect == 0 indicates that it is a normal player, batterSelect != 0 indicates
	// it is a joker player.
	// there must be at least one player as this function cannot get called without
	// waitingForBatterDecision-flag, and that can flag cant be true if
	// there is not at least one player.
	while(done == 0) {
		if(batterSelect == 0) {
			if(stateInfo->localGameInfo->gAI.nonJokerPlayersLeft != 0) done = 1;
			else batterSelect = 1;
		} else if(batterSelect == 4) {
			if(stateInfo->localGameInfo->gAI.nonJokerPlayersLeft != 0) {
				batterSelect = 0;
				done = 1;
			} else batterSelect = 1;

		} else {
			if(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->
			                                        pII.jokerIndices[batterSelect - 1]].bTPI.joker == 2) batterSelect++;
			else done = 1;

		}
		if(counter == 4) done = 1;
		counter++;
	}
	// now we have the batterSelect value and we just need to find a corresponding index for that
	// player.
	if(batterSelect == 0) {
		index = stateInfo->globalGameInfo->teams[battingTeamIndex].batterOrder[stateInfo->globalGameInfo->teams[battingTeamIndex].batterOrderIndex];
	} else {
		index = stateInfo->localGameInfo->pII.jokerIndices[batterSelect - 1];
	}
	// and set it here.
	stateInfo->localGameInfo->pII.batterSelectionIndex = index;
}

void genericSlingBall(StateInfo* stateInfo, float x, float y, float z)
{
	// this is called for example when throwing and batting
	// in these cases we want the change player arrays to update and to have new selected player from
	// those arrays
	stateInfo->localGameInfo->pRAI.refreshCatchAndChange = 1;
	stateInfo->localGameInfo->pRAI.initPlayerSelection = 1;
	// make ball visible and updatable
	stateInfo->localGameInfo->ballInfo.visible = 1;
	stateInfo->localGameInfo->ballInfo.moving = 1;

	// and set the new velocity
	setVectorXYZ(&(stateInfo->localGameInfo->ballInfo.velocity), x, y, z);

}


static void baseRun(StateInfo* stateInfo, int base)
{
	// so baserunning.
	// idea is just to update willStartRunning in every button press. and in special double click case we just run.
	if(stateInfo->localGameInfo->pII.safeOnBaseIndex[base] != -1) {
		if(stateInfo->localGameInfo->aF.bTAF.baseRun[base] == 1) {
			int index = stateInfo->localGameInfo->pII.safeOnBaseIndex[base];
			if(stateInfo->localGameInfo->playerInfo[index].bTPI.state == PLAYER_STATE_SAFE_ON_BASE ||
			        stateInfo->localGameInfo->playerInfo[index].bTPI.state == PLAYER_STATE_AT_BAT) {
				if(stateInfo->localGameInfo->pRAI.willStartRunning[base] == 0) {
					if(index != -1 && stateInfo->localGameInfo->playerInfo[index].cPI.moving == 0) {
						stateInfo->localGameInfo->pRAI.willStartRunning[base] = 1;
						if(base == 1 || base == 2) {
							lead(stateInfo, index);
						}
					}
				} else {
					stateInfo->localGameInfo->pRAI.willStartRunning[base] = 0;
				}
			} else {
				stateInfo->localGameInfo->pRAI.willStartRunning[base] = 0;
				if(index != -1) {
					if(stateInfo->localGameInfo->playerInfo[index].bTPI.state != PLAYER_STATE_SAFE_ON_BASE &&
					        stateInfo->localGameInfo->playerInfo[index].bTPI.state != PLAYER_STATE_AT_BAT) {
						runToPreviousBase(stateInfo, index, base);
					}
				}
			}
			if(doubleClickCounter[base] == -1) {
				doubleClickCounter[base] = 0;
			} else {
				if(doubleClickCounter[base] >= 0) {
					if(index != -1) {
						runToNextBase(stateInfo, index, base);
					}

				}
				doubleClickCounter[base] = -1;
			}
		}

	}
	stateInfo->localGameInfo->aF.bTAF.baseRun[base] = 0;
}

static void updateMeters(StateInfo* stateInfo)
{
	updatePitchingMeter(stateInfo);

	if(throwGoingOn == 1) {
		if(meterCounter < meterCounterMax) {
			meterCounter += 1;
		}
		stateInfo->localGameInfo->pRAI.meterValue = 1.0f*meterCounter / meterCounterMax;
	} else {
		updateBattingMeter(stateInfo);
	}
}

static void aiLogic(StateInfo* stateInfo, unsigned int* rng_seed)
{
	int battingTeamIndex = (stateInfo->globalGameInfo->
	                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
	int battingControl = stateInfo->globalGameInfo->teams[battingTeamIndex].control;
	int catchingControl = stateInfo->globalGameInfo->teams[(battingTeamIndex+1)%2].control;
	// first ai for catching team

	if(catchingControl == 2) {
		updateCatchingAI(stateInfo, rng_seed);
	}
	// then ai for batting team
	if(battingControl == 2) {
		updateBattingAI(stateInfo, rng_seed);
	}

}

void flushKeys(StateInfo* stateInfo)
{
	int i;
	for(i = 0; i < KEY_COUNT; i++) {
		stateInfo->keyStates->imitateKeyPress[i] = 0;
	}
}