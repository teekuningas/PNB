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

// here some constants used in the code

#define ANIMATION_FREQUENCY 3

#define TIMEOUT_CONSTANT 200

#define CLICK_BREAK_CONSTANT 3

// some static variables that are used only in action_implementation.c
// how they work is explained where they are needed, so if interested should check
// the code.

static int doubleClickCounter[BASE_COUNT];

// to ensure that no throws going different directions at the same time and that throwing player's orientation changes correctly
// static int throwGoingOn; // moved to action_state
// static int runBatFlag; // moved to action_state

// ai
static int aiDropStage;
static int aiThrowStage;
static int aiMoveCounter;

// batting team ai
static int aiBattingKeyDown;
static int aiActionKeyLock;

static int aiChangingKeyDown;

static int aiIncreaseKeyDown;
static int aiDecreaseKeyDown;
static int aiAngleDecided;
static float aiDecidedAngle;

// static int aiWrongPitch; // moved to action_state

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

static void changeBatter(StateInfo* stateInfo);
static void takeFreeWalkDecision(StateInfo* stateInfo);
static void baseRun(StateInfo* stateInfo, int base);
static void updateMeters(StateInfo* stateInfo);
static void aiLogic(StateInfo* stateInfo);
static void moveControlledPlayerToLocation(StateInfo* stateInfo, Vector3D* target);
static void throwBallToBase(StateInfo* stateInfo, int base);

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

	aiDropStage = 0;
	aiThrowStage = 0;
	aiActionEventLock = -1;
	aiLockUpdate = 0;
	aiMoveCounter = 0;
	aiLockTimeoutCounter = -1;

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
	flushKeys(stateInfo);
}

void actionImplementation(StateInfo* stateInfo)
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
	aiLogic(stateInfo);
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
				} else {
					// set info to screen
					stateInfo->localGameInfo->gAI.gameInfoEvent = 3;
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
			if(stateInfo->localGameInfo->playerInfo[index].
			        bTPI.isOnBase == 1) {
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
					if(stateInfo->localGameInfo->playerInfo[index].bTPI.isOnBase == 0) {
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
	}
	else {
		updateBattingMeter(stateInfo);
	}
}

static void aiLogic(StateInfo* stateInfo)
{
	int battingTeamIndex = (stateInfo->globalGameInfo->
	                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
	int battingControl = stateInfo->globalGameInfo->teams[battingTeamIndex].control;
	int catchingControl = stateInfo->globalGameInfo->teams[(battingTeamIndex+1)%2].control;
	int i;
	// first ai for catching team
	if(catchingControl == 2) {
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
	// then ai for batting team
	if(battingControl == 2) {
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
		if(stateInfo->localGameInfo->gAI.waitingForFreeWalkDecision == 1) {
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
		if(stateInfo->localGameInfo->gAI.waitingForBatterDecision == 1 && stateInfo->localGameInfo->gAI.ballHome == 1) {
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


			if(fieldStatus == 0) {
				if(stateInfo->localGameInfo->playerInfo[index].bTPI.speed > 2) {
					aiChange = 0;
				} else {
					aiChange = 1;
				}
			} else if(fieldStatus == 2) {
				if(stateInfo->localGameInfo->playerInfo[index].bTPI.power > 2) {
					aiChange = 0;
				} else {
					aiChange = 1;
				}
			} else {
				aiChange = 0;
			}
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

		} else if(stateInfo->localGameInfo->pRAI.batterReady == 1 && stateInfo->localGameInfo->pRAI.pitchInAir == 0 && stateInfo->localGameInfo->gAI.ballHome == 1) {
			// decision tree.. contents can be read within
			if(aiPlanCalculated == 0) {
				int batterIndex = stateInfo->localGameInfo->pII.batterIndex;
				int firstBaseIndex = stateInfo->localGameInfo->pII.safeOnBaseIndex[1];
				int secondBaseIndex = stateInfo->localGameInfo->pII.safeOnBaseIndex[2];
				int thirdBaseIndex = stateInfo->localGameInfo->pII.safeOnBaseIndex[3];
				int power = stateInfo->localGameInfo->playerInfo[batterIndex].bTPI.power;
				int speed = stateInfo->localGameInfo->playerInfo[batterIndex].bTPI.speed;
				int fieldStatus;
				int hasPower;
				int isFast;
				if(power > 2) hasPower = 1;
				else hasPower = 0;
				if(speed > 2) isFast = 1;
				else isFast = 0;
				if(firstBaseIndex != -1) fieldStatus = 2;
				else if(secondBaseIndex != -1 || thirdBaseIndex != -1) fieldStatus = 1;
				else fieldStatus = 0;
				if(stateInfo->globalGameInfo->period < 4) {
					if(stateInfo->localGameInfo->gAI.strikes == 0) {
						aiBattingStyle = 1;
						aiRunningBaseRunners = 0;
						aiRunningBatter = 0;
					} else if(stateInfo->localGameInfo->gAI.strikes == 1) {
						if(fieldStatus == 0) {
							if(isFast == 0) {
								aiBattingStyle = 2;
								aiRunningBaseRunners = 0;
								aiRunningBatter = 1;
							} else {
								aiBattingStyle = 0;
								aiRunningBaseRunners = 0;
								aiRunningBatter = 1;
							}
						} else if(fieldStatus == 1) {
							if(hasPower == 1) {
								if(isFast == 0) {
									aiBattingStyle = 1;
									aiRunningBaseRunners = 1;
									aiRunningBatter = 0;
								} else {
									aiBattingStyle = 1;
									aiRunningBaseRunners = 1;
									aiRunningBatter = 1;
								}
							} else {
								if(isFast == 0) {
									aiBattingStyle = 2;
									aiRunningBaseRunners = 0;
									aiRunningBatter = 1;
								} else {
									aiBattingStyle = 0;
									aiRunningBaseRunners = 0;
									aiRunningBatter = 1;
								}
							}
						} else if(fieldStatus == 2) {
							if(hasPower == 1) {
								if(isFast == 0) {
									aiBattingStyle = 1;
									aiRunningBaseRunners = 1;
									aiRunningBatter = 0;
								} else {
									aiBattingStyle = 1;
									aiRunningBaseRunners = 1;
									aiRunningBatter = 1;
								}
							} else {
								if(isFast == 0) {
									aiBattingStyle = 0;
									aiRunningBaseRunners = 1;
									aiRunningBatter = 0;
								} else {
									aiBattingStyle = 0;
									aiRunningBaseRunners = 1;
									aiRunningBatter = 1;
								}
							}
						}
					} else if(stateInfo->localGameInfo->gAI.strikes == 2) {
						if(fieldStatus == 0) {
							if(isFast == 0) {
								aiBattingStyle = 2;
								aiRunningBaseRunners = 0;
								aiRunningBatter = 1;
							} else {
								aiBattingStyle = 0;
								aiRunningBaseRunners = 0;
								aiRunningBatter = 1;
							}
						} else if(fieldStatus == 1) {
							aiBattingStyle = 2;
							aiRunningBaseRunners = 0;
							aiRunningBatter = 1;
						} else if(fieldStatus == 2) {
							if(hasPower == 1) {
								aiBattingStyle = 1;
								aiRunningBaseRunners = 1;
								aiRunningBatter = 1;
							} else {
								aiBattingStyle = 0;
								aiRunningBaseRunners = 1;
								aiRunningBatter = 1;
							}
						}
					}
				} else {
					if(stateInfo->localGameInfo->gAI.strikes == 0 || stateInfo->localGameInfo->gAI.strikes == 1) {
						aiBattingStyle = 1;
						aiRunningBaseRunners = 0;
						aiRunningBatter = 0;
					} else {
						if(hasPower == 1) {
							aiBattingStyle = 1;
							aiRunningBaseRunners = 1;
							aiRunningBatter = 1;
						} else {
							aiBattingStyle = 0;
							aiRunningBaseRunners = 1;
							aiRunningBatter = 0;
						}
					}
				}
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
					        stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.safeOnBaseIndex[i]].bTPI.isOnBase == 1 &&
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
		else if(stateInfo->localGameInfo->pRAI.batterReady == 1 && stateInfo->localGameInfo->pRAI.pitchInAir == 0 && stateInfo->localGameInfo->gAI.ballHome == 0) {
			if(aiRunningBaseRunners == 1) {
				int i;
				for(i = 1; i < 3; i++) {
					if(stateInfo->localGameInfo->pII.safeOnBaseIndex[i] != -1 &&
					        stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.safeOnBaseIndex[i]].bTPI.leading == 1 &&
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
			float v_x = (float)fabs(stateInfo->localGameInfo->ballInfo.velocity.x);
			float v_y = stateInfo->localGameInfo->ballInfo.velocity.y;
			float g = GRAVITY;
			float t = v_y*2/g;
			float offset = v_x*t;
			if(offset > PLATE_WIDTH/2 && aiWrongPitch == 0) {
				aiWrongPitch = 1;
			}
			if(aiWrongPitch == 1) {
				// batter isnt handled here
				// this code will make baserunners come back if wrong pitch is pitched
				for(i = 1; i < BASE_COUNT; i++) {
					int index = stateInfo->localGameInfo->pII.safeOnBaseIndex[i];
					if(index != -1 && stateInfo->localGameInfo->playerInfo[index].bTPI.goingForward == 1 && aiBaseRunnerKeyDown[i] == 0 &&
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
					int random = rand()%4 + 2;
					aiDecidedAngle = (float)random / 20.0f;
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
					int random;
					int i;
					int leadBase = -1;
					for(i = 0; i < BASE_COUNT; i++) {
						int index = stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[i];
						if(index != -1) {
							if(stateInfo->localGameInfo->playerInfo[index].bTPI.base > leadBase) {
								leadBase = stateInfo->localGameInfo->playerInfo[index].bTPI.base;
							}
						}
					}
					if(leadBase == 2) {
						random = -rand()%16;
						aiDecidedAngle = (float)random / 45.0f;
					} else if(leadBase == 1) {
						random = rand()%16;
						aiDecidedAngle = (float)random / 45.0f;
					} else {
						random = rand()%33;
						random = random - 16;
						aiDecidedAngle = (float)random / 45.0f;
					}
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
					int random = rand()%5;
					random = random - 2;
					aiDecidedAngle = (float)random / 20.0f;
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
					if(stateInfo->localGameInfo->playerInfo[runnerIndex].bTPI.base == i) {
						if(runnerIndex != index) {
							shouldRun = 0;
						}
					}
				}
			}
			// if everything ok, initiate running.
			if(shouldRun == 1 && isDoubleClickingOk == 1 && aiBaseRunnerLock[i] == AI_NO_LOCK &&
			        aiBaseRunnerKeyDown[i] == 0 && index != -1 && stateInfo->localGameInfo->playerInfo[index].bTPI.goingForward != 1 &&
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

}
// we move towards the target position by simulating key presses.
static void moveControlledPlayerToLocation(StateInfo* stateInfo, Vector3D* target)
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

void flushKeys(StateInfo* stateInfo)
{
	int i;
	for(i = 0; i < KEY_COUNT; i++) {
		stateInfo->keyStates->imitateKeyPress[i] = 0;
	}
}

static void throwBallToBase(StateInfo* stateInfo, int base)
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
