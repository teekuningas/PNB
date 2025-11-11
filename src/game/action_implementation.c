/*
	So this file specifically handles user-initiated events. This works in conjuction
	with action_invocations.c where all the input reading is done. This has also some
	dirty floating point code, but it should be closely related to something that happens
	after user presses some keys. Like batting or throwing or running.
*/

#include "globals.h"
#include "action_implementation.h"
#include "common_logic.h"

// here some constants used in the code
#define PITCH_BASE_SPEED 0.065f
#define PITCHER_MOVE_AWAY_OFFSET 0.1f + DISTANCE_FROM_HOME_LOCATION_THRESHOLD + TARGET_ACHIEVED_THRESHOLD
#define PITCH_POWER_CONSTANT 0.12f
#define PITCH_ANGLE_CONSTANT 0.15f
#define THROW_TO_BASE_DISTANCE 1.0f
#define THROW_POWER_CONSTANT 0.65f
#define THROW_DISTANCE_CONSTANT 0.0012f
#define THROW_MAX 50
#define BATTER_ANGLE_SPEED_CONSTANT 0.02f
#define BATTER_ANGLE_LIMIT PI/7
#define GENERIC_BATTER_ADVANCE_SPEED_CONSTANT 0.7f

#define PITCH_FRAME_TIME_TWEAK 3
#define DROP_BALL_CONSTANT 0.02f
#define PITCH_DOWN_MAX 9
#define PITCH_UP_MAX 13
#define ANIMATION_FREQUENCY 3
#define BAT_LOAD_MAX (4*9)
#define BAT_SWING_MAX (4*13)
#define BAT_ANIMATION_FRAME_HIT_COUNT (20*ANIMATION_FREQUENCY)
#define BAT_ANIMATION_FRAME_TOTAL_COUNT (34*ANIMATION_FREQUENCY)
#define BALL_MAX_OFFSET 1.0f
#define BUNT_THRESHOLD 20
#define VERTICAL_ANGLE_LIMIT 5

#define BUNT_ADVANCE 1.0f
#define SWING_ADVANCE 0.5f
#define SPREAD_ADVANCE 0.3f

#define BATTER_ANGLE_FIX (2*PI / 4)

// ai action flags
#define AI_NO_LOCK -1
#define AI_PITCH_LOCK 0
#define AI_THROW_LOCK 1
#define AI_DROP_LOCK 2

#define AI_WAITING_BATTER_LOCK 3
#define AI_WAITING_WALK_LOCK 4
#define AI_BATTING_LOCK 5
#define AI_CHANGE_LOCK 6

#define AI_CLICK_LOCK 7
#define AI_DOUBLE_CLICK_LOCK 8
#define AI_COME_BACK_LOCK 9
#define AI_COME_BACK_WRONG_PITCH_LOCK 10

#define TIMEOUT_CONSTANT 200

#define CLICK_BREAK_CONSTANT 3

// some static variables that are used only in action_implementation.c
// how they work is explained where they are needed, so if interested should check
// the code.
static float throwDistance;
static Vector3D throwDirection;

static unsigned int meterCounter;
static unsigned int meterCounterMax;

static int doubleClickCounter[BASE_COUNT];
static int pitchFrameTime;
static float pitchPower;
static int batterSelect;
static int battingFrameCount;
static int increaseBattingFrameCount;
static int selectedBattingPowerCount;
static int selectedBattingAngleCount;
static float batterAngle;
static int batterAngleSpeed;
static float batterAdvanceSpeed;
static float batterAdvance;
static int battingMode;
static float batterAdvanceLimit;
static int battingStopped;
static int batterMoving;
static int updateBatterLocationAndOrientation;

// to ensure that no throws going different directions at the same time and that throwing player's orientation changes correctly
static int throwGoingOn;
static int runBatFlag;

// ai
static int aiPitchStage;
static int aiDropStage;
static unsigned int aiPitchFirstLimit;
static unsigned int aiPitchSecondLimit;
static int aiThrowStage;
static int aiActionEventLock;
static int aiLockUpdate;
static int aiMoveCounter;

// we need timeouts as sometimes ai tries to do somthing too quickly before the other
// action implementation machinery is ready for it
static int aiLockTimeoutCounter;
static int aiPitchTime;
static int aiPitchPreviousTime;
static int aiBatterReadyTimer;

// batting team ai
static int aiBattingKeyDown;
static int aiActionKeyLock;

static int aiChangingKeyDown;

static int aiIncreaseKeyDown;
static int aiDecreaseKeyDown;
static int aiAngleDecided;
static float aiDecidedAngle;

static int aiWrongPitch;

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

static void genericThrowRelease(StateInfo* stateInfo);
static void genericThrowLoad(StateInfo* stateInfo, int base);
static void genericMove(StateInfo* stateInfo, int direction);
static void genericStopMove(StateInfo* stateInfo, int direction);
static void genericSlingBall(StateInfo* stateInfo, float x, float y, float z);
static void dropBall(StateInfo* stateInfo);
static void updateControlledPlayerSpeed(StateInfo* stateInfo);
static void startPitch(StateInfo* stateInfo);
static void continuePitch(StateInfo* stateInfo);
static void releasePitch(StateInfo* stateInfo);
static void changeBatter(StateInfo* stateInfo);
static void selectBatter(StateInfo* stateInfo);
static void takeFreeWalkDecision(StateInfo* stateInfo);
static void updateBatting(StateInfo* stateInfo);
static void startIncreaseBatterAngle(StateInfo* stateInfo);
static void stopIncreaseBatterAngle(StateInfo* stateInfo);
static void startDecreaseBatterAngle(StateInfo* stateInfo);
static void stopDecreaseBatterAngle(StateInfo* stateInfo);
static void selectPower(StateInfo* stateInfo);
static void selectAngle(StateInfo* stateInfo);
static void baseRun(StateInfo* stateInfo, int base);
static void updateMeters(StateInfo* stateInfo);
static void aiLogic(StateInfo* stateInfo);
static void moveControlledPlayerToLocation(StateInfo* stateInfo, Vector3D* target);
static void flushKeys(StateInfo* stateInfo);
static void throwBallToBase(StateInfo* stateInfo, int base);

void initActionImplementation(StateInfo* stateInfo)
{
	// just initialize everyone of these static variables to zero
	int i;
	throwDistance = 0;
	throwGoingOn = 0;
	throwDirection.x = 0;
	throwDirection.y = 0;
	throwDirection.z = 0;

	meterCounter = 0;
	meterCounterMax = 0;
	for(i = 0; i < BASE_COUNT; i++) {
		doubleClickCounter[i] = -1;
	}

	pitchFrameTime = 0;
	pitchPower = 0;
	batterSelect = 0;
	battingFrameCount = 0;
	increaseBattingFrameCount = 0;
	selectedBattingPowerCount = 0;
	selectedBattingAngleCount = 0;
	batterAngle = 0;
	batterAngleSpeed = 0;
	batterAdvanceSpeed = 0;
	batterAdvance = 0;
	battingMode = 0;
	batterAdvanceLimit = 0;
	battingStopped = 0;
	batterMoving = 0;
	updateBatterLocationAndOrientation = 0;
	runBatFlag = 0;

	//ai uses a few flags..

	aiPitchStage = 0;
	aiDropStage = 0;
	aiThrowStage = 0;
	aiActionEventLock = -1;
	aiLockUpdate = 0;
	aiMoveCounter = 0;
	aiLockTimeoutCounter = -1;
	aiPitchTime = -1;
	aiPitchPreviousTime = -1;
	aiPitchFirstLimit = 0;
	aiPitchSecondLimit = 0;
	aiBatterReadyTimer = -1;

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
				switch(i) {
				case 0:
					stateInfo->localGameInfo->pRAI.throwGoingToBase = 0;
					throwDirection.x = stateInfo->fieldPositions->pitcher.x - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.x;
					throwDirection.z = stateInfo->fieldPositions->pitcher.z - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.z;
					break;
				case 1:
					stateInfo->localGameInfo->pRAI.throwGoingToBase = 1;
					throwDirection.x = stateInfo->fieldPositions->firstBase.x - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.x;
					throwDirection.z = stateInfo->fieldPositions->firstBase.z - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.z;
					break;
				case 2:
					stateInfo->localGameInfo->pRAI.throwGoingToBase = 2;
					throwDirection.x = stateInfo->fieldPositions->secondBase.x - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.x;
					throwDirection.z = stateInfo->fieldPositions->secondBase.z - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.z;
					break;
				case 3:
					stateInfo->localGameInfo->pRAI.throwGoingToBase = 3;
					throwDirection.x = stateInfo->fieldPositions->thirdBase.x - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.x;
					throwDirection.z = stateInfo->fieldPositions->thirdBase.z - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.z;
					break;
				}
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
static void startIncreaseBatterAngle(StateInfo* stateInfo)
{
	stateInfo->localGameInfo->aF.bTAF.increaseBatterAngle = 2;
	// set batterAngleSpeed to 1 to indicate that the direction of the movement is cw
	batterAngleSpeed = 1;

}
static void stopIncreaseBatterAngle(StateInfo* stateInfo)
{
	stateInfo->localGameInfo->aF.bTAF.increaseBatterAngle = 0;
	// when stopping the increasing of the angle, we want not to interrupt an ongoing decreasing of the angle
	if(batterAngleSpeed != -1) {
		batterAngleSpeed = 0;
	}
	stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.lastLastLocationUpdate = 1;
}

static void startDecreaseBatterAngle(StateInfo* stateInfo)
{
	stateInfo->localGameInfo->aF.bTAF.decreaseBatterAngle = 2;
	// set batterAngleSpeed to 1 to indicate that the direction of the movement is ccw
	batterAngleSpeed = -1;

}
static void stopDecreaseBatterAngle(StateInfo* stateInfo)
{
	stateInfo->localGameInfo->aF.bTAF.decreaseBatterAngle = 0;
	// when stopping the decreasing of the angle, we want not to interrupt an ongoing increasing of the angle
	if(batterAngleSpeed != 1) {
		batterAngleSpeed = 0;
	}
	stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.lastLastLocationUpdate = 1;
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
// here is where the accepting selected player happens.
static void selectBatter(StateInfo* stateInfo)
{
	int i = 0;
	int battingTeamIndex = (stateInfo->globalGameInfo->
	                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
	// index cannot be -1 as we couldn't have got this far if it was
	int index = stateInfo->localGameInfo->pII.batterSelectionIndex;
	if(index != -1) {
		Vector3D target;
		// we set the batterSelect to be 0 so that it will be correct one next time we have the decision
		batterSelect = 0;
		// and set these to 0 as decision made.
		stateInfo->localGameInfo->gAI.waitingForBatterDecision = 0;
		stateInfo->localGameInfo->aF.bTAF.chooseBatter = 0;

		// here we look for a free spot in battingTeamOnFieldIndices[]
		// and put our new guy there. there will be a spot as we cannot get here
		// if there is more than 3 baserunners already.
		while(1) {
			if(stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[i] == -1) {
				stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[i] = index;
				break;
			} else {
				if(i < 3)
					i++;
				else
					break;
			}

		}
		// new batting team player on the field.
		stateInfo->localGameInfo->gAI.battingTeamPlayersOnFieldCount++;
		// has base of zero, is on base and original base is zero too.
		stateInfo->localGameInfo->playerInfo[index].bTPI.base = 0;
		stateInfo->localGameInfo->playerInfo[index].bTPI.isOnBase = 1;
		stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase = 0;
		// this guy will begin with 0 strikes and 0 balls.
		stateInfo->localGameInfo->gAI.strikes = 0;
		stateInfo->localGameInfo->gAI.balls = 0;
		// set batterIndex
		stateInfo->localGameInfo->pII.batterIndex = index;
		// cant advance yet
		stateInfo->localGameInfo->pRAI.batterCanAdvance = 0;
		// just set default values so that the player can have a fresh start at
		// the field.
		stateInfo->localGameInfo->playerInfo[index].bTPI.goingForward = 0;
		stateInfo->localGameInfo->playerInfo[index].bTPI.wounded = 0;
		stateInfo->localGameInfo->playerInfo[index].bTPI.woundedApply = 0;
		stateInfo->localGameInfo->playerInfo[index].bTPI.leading = 0;
		stateInfo->localGameInfo->playerInfo[index].bTPI.out = 0;
		stateInfo->localGameInfo->playerInfo[index].bTPI.passedPathPoint = 0;
		stateInfo->localGameInfo->playerInfo[index].bTPI.hasMadeRunOnThirdBase = 0;
		// if he is a (unused) joker player, mark him as used, and decrease the amount of jokers left.
		if(stateInfo->localGameInfo->playerInfo[index].bTPI.joker == 1) {
			stateInfo->localGameInfo->gAI.jokersLeft--;
			stateInfo->localGameInfo->playerInfo[index].bTPI.joker = 2;
		} else {
			// otherwise he is not a joker player and we must decrease the amount of those.
			stateInfo->localGameInfo->gAI.nonJokerPlayersLeft--;
			// also the batterIndex will increase(mod 9)
			stateInfo->globalGameInfo->teams[battingTeamIndex].batterOrderIndex = (stateInfo->globalGameInfo->teams[battingTeamIndex].batterOrderIndex + 1)%PLAYERS_IN_TEAM;
		}
		// move player to default batter ready position
		target.x = (float)(stateInfo->fieldPositions->pitchPlate.x + cos(ZERO_BATTING_ANGLE)*BATTING_RADIUS);
		target.z = (float)(stateInfo->fieldPositions->pitchPlate.z - sin(ZERO_BATTING_ANGLE)*BATTING_RADIUS);
		// move to target can take care of the rest.
		moveToTarget(stateInfo, index, &target);
	}
}

static void genericThrowRelease(StateInfo* stateInfo)
{
	if(stateInfo->localGameInfo->pII.hasBallIndex != -1) {
		float power;
		// throw not going anymore, ball already flyin'
		throwGoingOn = 0;
		// release animation
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.model = 9;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStage = 0;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStageCount = 21;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationFrequency = 2;
		// set flag to indicate that animation is still going on ( so no extra movement
		// until its over ).
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cTPI.throwRecoil = 1;

		// take power naturally from meterCounter value
		power = 1.0f*meterCounter / meterCounterMax;
		// update these values a bit
		throwDirection.x = throwDirection.x / throwDistance;
		throwDirection.z = throwDirection.z / throwDistance;
		throwDirection.y = 0.06f;
		// ... and then edit them a bit more and send them to genericSlingBall.
		genericSlingBall(stateInfo, throwDirection.x*power*THROW_POWER_CONSTANT, throwDirection.y + throwDistance*THROW_DISTANCE_CONSTANT, throwDirection.z*power*THROW_POWER_CONSTANT);
		// set lastHadBallIndex, its used for example to prevent this player of catching
		// the ball right after throwing.
		stateInfo->localGameInfo->pII.lastHadBallIndex = stateInfo->localGameInfo->pII.hasBallIndex;
		// no player has ball anymore
		stateInfo->localGameInfo->pII.hasBallIndex = -1;
		// set running flag to 0 so that orientation will change
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.running = 0;
		// set control to -1 and changePlayer to 0 as a precaution so that the player
		// wouldnt be changed right away after this, as the key
		// to do this is the same one. let the genericSlingBall handle
		// player changing.
		stateInfo->localGameInfo->pII.controlIndex = -1;
		stateInfo->localGameInfo->aF.cTAF.changePlayer = 0;
	}
}
// so this function is called when player is preparing to throw.
// throw cannot be undone after this
static void genericThrowLoad(StateInfo* stateInfo, int base)
{
	if(stateInfo->localGameInfo->pII.hasBallIndex != -1) {
		// throw distance is the euclidean distance from the base to player throwing.
		throwDistance = (float)sqrt(throwDirection.x*throwDirection.x + throwDirection.z*throwDirection.z);
		// if player is already on the base, cant throw.
		if(throwDistance > THROW_TO_BASE_DISTANCE) {
			// stop player if he is moving, moving won't look good as the animation
			// doesn't have foot movement
			if(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.moving == 1) {
				stopMovement(stateInfo, stateInfo->localGameInfo->pII.hasBallIndex);
			}
			// set the animation
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.model = 8;
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStage = 0;
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStageCount = 11;
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationFrequency = 3;
			// initialize meters.
			meterCounter = 0;
			meterCounterMax = THROW_MAX; // arbitrary decision, seems about right though
			// continue to next phase
			stateInfo->localGameInfo->aF.cTAF.throwToBase[base] = 2;
			// set the flag that is used for example to determine can you move the player.
			throwGoingOn = 1;
			// to avoid twitching when moving key is still pressed and player cant move as hes throwing
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.lastLastLocationUpdate = 1;
			// and orient player to look at the base too.
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.orientation.x = throwDirection.x;
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.orientation.z = throwDirection.z;
		} else {
			// if too close to base, terminate throwing.
			stateInfo->localGameInfo->aF.cTAF.throwToBase[base] = 0;
			stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 0;
			throwGoingOn = 0;
			stateInfo->localGameInfo->pRAI.throwGoingToBase = -1;
		}
	}
}

static void genericMove(StateInfo* stateInfo, int direction)
{
	// we can move if there is no throw going on and no pitch going on
	// .. and we have same player controlled
	if(throwGoingOn == 0 && stateInfo->localGameInfo->pRAI.pitchGoingOn == 0 &&
	        stateInfo->localGameInfo->pII.controlIndex != -1) {
		// stopping only possible when moving already going on
		// so thats the reason for this value 2
		stateInfo->localGameInfo->aF.cTAF.move[direction] = 2;

		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cTPI.movesToDirection[direction] = 1;
		// and we call this generic function that utilizes this movesToDirection to select
		// velocity and orientation for the player
		updateControlledPlayerSpeed(stateInfo);
	} else {
		stateInfo->localGameInfo->aF.cTAF.move[direction] = 0;
	}

}
static void genericStopMove(StateInfo* stateInfo, int direction)
{
	// stopping cant be done either when pitching or throwing as updateControlledPlayerSpeed can
	// have effects on player's model
	if(throwGoingOn == 0 && stateInfo->localGameInfo->pRAI.pitchGoingOn == 0 &&
	        stateInfo->localGameInfo->pII.controlIndex != -1) {
		stateInfo->localGameInfo->aF.cTAF.move[direction] = 0;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cTPI.movesToDirection[direction] = 0;
		updateControlledPlayerSpeed(stateInfo);
	} else {
		stateInfo->localGameInfo->aF.cTAF.move[direction] = 0;
	}
}


static void dropBall(StateInfo* stateInfo)
{
	// there is a possibility to drop ball if to the ground if you want. it could be convenient when
	// you want a baserunner to be able to get safe from a base for some strategical reason.
	if(stateInfo->localGameInfo->pII.hasBallIndex != -1) {
		if(throwGoingOn == 0 && stateInfo->localGameInfo->pRAI.pitchGoingOn == 0) {
			float norm;
			float dx;
			float dz;

			// players' movement will be stopped when doing this, similar to throwing.
			if(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.moving == 1) {
				stopMovement(stateInfo, stateInfo->localGameInfo->pII.hasBallIndex);
			}
			// model is set to be the basic standing without ball model.
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.model = 0;
			// and then just set a little upward-forward -directed value for ball so that we'll see the dropping
			dx = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.orientation.x;
			dz = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.orientation.z;
			norm = (float)sqrt(dx*dx+dz*dz);
			if( norm < EPSILON ) norm = 1.0f;
			dx = dx / norm;
			dz = dz / norm;
			// and use genericSlingBall again to get the ball to the world.
			genericSlingBall(stateInfo, dx*DROP_BALL_CONSTANT, DROP_BALL_CONSTANT, dz*DROP_BALL_CONSTANT);
			// and set the lastHadBallIndex so that this player cannot catch it before it hits ground
			stateInfo->localGameInfo->pII.lastHadBallIndex = stateInfo->localGameInfo->pII.hasBallIndex;
			// and no player has the ball anymore.
			stateInfo->localGameInfo->pII.hasBallIndex = -1;
		}
	}
	stateInfo->localGameInfo->aF.cTAF.dropBall = 0;
	stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 0;
}

static void updateControlledPlayerSpeed(StateInfo* stateInfo)
{
	if(stateInfo->localGameInfo->pII.controlIndex != -1) {
		// cant move when throw recoil going on.
		if(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cTPI.throwRecoil == 0) {
			float norm;
			// we select the direction by taking the difference of moves in x direction and moves in z direction
			// moves are 0 or 1, so as a net result we will get the direction where the player really should be going on
			int directionX = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cTPI.movesToDirection[1] -
			                 stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cTPI.movesToDirection[3];
			int directionZ = - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cTPI.movesToDirection[0] +
			                 stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cTPI.movesToDirection[2];
			// always when player's velocity changes, ball's velocity must change too.
			stateInfo->localGameInfo->ballInfo.needsMoveUpdate = 1;
			// if every component vanishes
			if(directionX*directionX + directionZ*directionZ == 0) {
				// set moving to zero
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.moving = 0;
				// if controlled player has also ball, set corresponding model
				// otherwise set model without ball
				if(stateInfo->localGameInfo->pII.hasBallIndex == stateInfo->localGameInfo->pII.controlIndex)
					stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.model = 1;
				else
					stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.model = 0;
				// when stopping movement, need to update last location.
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.lastLastLocationUpdate = 1;
			} else {
				// if there is a non-zero component in x or z direction
				// moving is to be 1 and we are going to have walking or running animation
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.moving = 1;
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.animationFrequency = 3;
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.animationStage = 0;
				// set player's orientation so that player faces the direction he's moving to
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].tPI.orientation.x = (float)directionX;
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].tPI.orientation.z = (float)directionZ;

				// Find norm
				norm = (float)sqrt(directionX*directionX + directionZ*directionZ);
				if (norm < EPSILON) norm = 1.0f;

				// running
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].tPI.velocity.x = (float)directionX*RUN_SPEED/norm;
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].tPI.velocity.z = (float)directionZ*RUN_SPEED/norm;
				stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.animationStageCount = 20;
				// if has ball, then running with ball model, otherwise running without ball
				if(stateInfo->localGameInfo->pII.hasBallIndex == stateInfo->localGameInfo->pII.controlIndex) {
					stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.model = 5;

				} else {
					stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.controlIndex].cPI.model = 4;
				}
			}
		}
	}
}

static void genericSlingBall(StateInfo* stateInfo, float x, float y, float z)
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

static void startPitch(StateInfo* stateInfo)
{
	/*
		To start a pitch few things must hold:
		i) pitcher must have the ball
		ii) no pitch is already going on.
		iii) batter is ready
		iv) throw is not going on. we can stop pitch and throw but cant stop throw and pitch.
		v) pitcher is close enough to pitching location.
		vi) no free walk decisions pending
	*/
	if(stateInfo->localGameInfo->pII.hasBallIndex == stateInfo->localGameInfo->pII.catcherOnBaseIndex[0] && stateInfo->localGameInfo->pRAI.pitchGoingOn == 0 &&
	        stateInfo->localGameInfo->pRAI.batterReady == 1 && throwGoingOn == 0 &&
	        stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.catcherOnBaseIndex[0]].cTPI.isNearHomeLocation == 1 &&
	        stateInfo->localGameInfo->gAI.waitingForFreeWalkDecision == 0) {
		// we stop the pitcher if we were moving with it when we started
		if(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.moving == 1) {
			stopMovement(stateInfo, stateInfo->localGameInfo->pII.hasBallIndex);
		}
		// we choose animation of pitcher crouching.
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.model = 6;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStage = 0;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStageCount = PITCH_DOWN_MAX;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationFrequency = ANIMATION_FREQUENCY;
		// and we force pitcher to this specific pitching location as the pitching can be started even if
		// pitcher is not exactly at this location.
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.x =
		    stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.homeLocation.x;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.z =
		    stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.homeLocation.z;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.lastLastLocationUpdate = 1;
		// and set the pitcher to look directly to pitchPlate's direction.
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.orientation.x = -1.0f;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.orientation.z = 0.0f;
		// ball is moved to center of the pitchPlate so that pitchs will start
		// rising from there.
		setVectorXZ(&(stateInfo->localGameInfo->ballInfo.location), 0.0f, 0.0f);


		// we enter the next stage where the meter moves and user needs to
		// select the power to continue
		stateInfo->localGameInfo->aF.cTAF.pitch = 2;
		// we set pitchGoingOn flag to 1 which will hold to the moment
		// of bat hitting ball, meter going all the way down ( no angle selected )
		// or ball hitting ground.
		stateInfo->localGameInfo->pRAI.pitchGoingOn = 1;
		// so initialize meterCounter and meterCounterMax values. synchronization with the animation here is nice
		// as it will let user press the buttons when its natural in the animation. But basically
		// we start from the point 4/13 and go to 1 on the meter.
		meterCounter = (PITCH_UP_MAX - PITCH_DOWN_MAX)*ANIMATION_FREQUENCY;
		meterCounterMax = PITCH_UP_MAX * ANIMATION_FREQUENCY;
	} else {
		// if conditions dont hold then put pitch=0 so that user can try to
		// initiate new pitch if he wants.
		stateInfo->localGameInfo->aF.cTAF.pitch = 0;
		stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 0;
	}
}
static void continuePitch(StateInfo* stateInfo)
{
	if(stateInfo->localGameInfo->pII.hasBallIndex != -1) {
		// as power is selected now, we move to the next phase of meter going down, animation
		// going from crouching to releasing and user to selecting the angle.
		stateInfo->localGameInfo->aF.cTAF.pitch = 4;
		// here we select pitchpower, and as selected it will be in the interval from
		//	(PITCH_UP_MAX - PITCH_DOWN_MAX)/PITCH_UP_MAX to 1.
		pitchPower = 1.0f*meterCounter / meterCounterMax;
		// we select the animation
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.model = 7;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationFrequency = ANIMATION_FREQUENCY;

		// current stage depends on the stage of the last animation. if user quickly selects
		// the power, the previous animation might have no time to finish, so if that happens
		// we dont want to start the releasing animation from the beginning.
		// this works quite fluently as the crouching animation's end consists of same frames
		// as releasing animation's beginning.
		//
		// so here animationStageCount is PITCH_DOWN_MAX and we minus from that
		// the number of stages that we already did. as the animation implementation in the code works so that
		// animationStage goes from 0 to stage count times frequency, to get our stage relative to
		// animationStageCount, we divide by the frequency. then we multiply by new frequency
		// to get new stage to range of 0 to PITCH_DOWN_MAX(and we extend to PITCH_UP_MAX after that)
		// times the frequency.
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStage =
		    (stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStageCount -
		     stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStage / ANIMATION_FREQUENCY) *
		    ANIMATION_FREQUENCY;
		stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.animationStageCount = PITCH_UP_MAX;

		// so now we initialize meterCounter to be what was left to the full amount in previous phase and set counterMax to full maximum.
		// on the screen this meterCounter-value is kind of reversed so that we get a nice indicator going up, indicator going down -effect.
		meterCounter = meterCounterMax - meterCounter;
		meterCounterMax = PITCH_UP_MAX * ANIMATION_FREQUENCY;
	}
}
static void releasePitch(StateInfo* stateInfo)
{
	// so here we have now selected the angle also and ball is ready to see the world.
	Vector3D target;
	float dx, dy;
	int i;
	float pitchAngle;
	// as meterCounter goes from 0 to PITCH_UP_MAX and the zero point will be at the 9/13, we minus
	// that to get the selected angle
	pitchAngle = 1.0f*meterCounter / meterCounterMax - 1.0f*PITCH_DOWN_MAX/PITCH_UP_MAX;
	// So here we set the velocity for the ball when it finally leaves the hand of the pitcher.
	// dx is going to be the error term and it doesnt depend on the power so when ball is pitched higher, the error will have more time to
	// increase
	dx = pitchAngle*PITCH_ANGLE_CONSTANT;
	// simple formula, just have base_speed so that there wont any very low pitches and then add some power if wanted.
	// it will be made so that its more difficult to hit the ball the higher the pitch is.
	dy = PITCH_BASE_SPEED + pitchPower * PITCH_POWER_CONSTANT;
	// we prepare to move the pitcher a bit
	target.x = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.x + PITCHER_MOVE_AWAY_OFFSET;
	target.z = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.z;
	// set the ball visible and tell other code that is moving so its location
	// will be updated.
	stateInfo->localGameInfo->ballInfo.visible = 1;
	stateInfo->localGameInfo->ballInfo.moving = 1;
	// set the velocity by our dx and dy
	setVectorXYZ(&(stateInfo->localGameInfo->ballInfo.velocity), dx, dy, 0);
	// ..and move the pitcher
	moveToTarget(stateInfo, stateInfo->localGameInfo->pII.hasBallIndex, &target);
	// set lastHadBallIndex so that pitcher wont catch the ball without it hitting ground first
	stateInfo->localGameInfo->pII.lastHadBallIndex = stateInfo->localGameInfo->pII.hasBallIndex; // to allow ball to avoid catching by same player when thrown
	// pitcher doesnt have the ball anymore
	stateInfo->localGameInfo->pII.hasBallIndex = -1;
	// pitch in air so that for example the batting can be
	// updated.
	stateInfo->localGameInfo->pRAI.pitchInAir = 1;
	// this flag's purpose is to take care of batter who starts running towards first base and comes back
	// during the pitch.
	runBatFlag = 0;
	// batter can advance now
	stateInfo->localGameInfo->pRAI.batterCanAdvance = 1;
	// let ai do the calculation for ball again
	aiWrongPitch = 0;
	// set camera back to normal if there was homerun camera
	stateInfo->localGameInfo->gAI.homeRunCameraFlag = 0;
	// always when pitch reaches the stage of ball going to air, we update baserunners'
	// original bases to their current bases, so that we can make decisions about
	// foul plays and wounds etc.
	for(i = 0; i < BASE_COUNT; i++) {
		int index = stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[i];
		if(index != -1) {
			int base = stateInfo->localGameInfo->playerInfo[index].bTPI.base;
			// we dont do it though in the case of free walks, as we dont want players to return previous bases
			// after taking a free walk, even if there is foul play. so thats the reason for conditions.
			// free walks set original base to base that follows the base where player was when the
			// free walk decision came available.
			if(stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase < base &&
			        !(stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase == 4 && base == 3)) {
				int done = 0;
				// no such thing as safeOnBaseIndex[4] so have to be base < 4
				// here we just make sure that player is safe on the base that is declared
				// as his originalBase.
				// if he's not, he can try gaining now originalBase by running to next one
				// or he will just get tagged if its foul play.
				if(base >= 0 && base < 4) {
					if(stateInfo->localGameInfo->pII.safeOnBaseIndex[base] != index) {
						done = 1;
						stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase = -1;
					}
				}
				if(done == 0) {
					stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase = stateInfo->localGameInfo->playerInfo[index].bTPI.base;
				}
			}
		}
	}

	// run with batting team

	for(i = 1; i < BASE_COUNT; i++) {
		if(stateInfo->localGameInfo->pRAI.willStartRunning[i] == 1) {
			int index = stateInfo->localGameInfo->pII.safeOnBaseIndex[i];
			stateInfo->localGameInfo->pRAI.willStartRunning[i] = 0;
			if(index != -1) {
				runToNextBase(stateInfo, index, i);
			}
		}
	}

	// and pitch is zero so we can try to start pitch again when necessary conditions hold
	stateInfo->localGameInfo->aF.cTAF.pitch = 0;
	stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 0;

}
// so this function is called when we decide the power
static void selectPower(StateInfo* stateInfo)
{
	// swing is set to 3 so that the meter indicator on the screen can start decreasing etc.
	stateInfo->localGameInfo->aF.bTAF.swing = 3;
	// we select meterCounter value as the power but we move it a bit so that the smallest value isnt 4/13 but instead 0.
	selectedBattingPowerCount = meterCounter - (BAT_SWING_MAX - BAT_LOAD_MAX);
	// and then for angle we start again from zero and go to the max and later on we'll scale it a bit to look nice on the screen.
	// this is used so that more power we use, faster will the indicator come down when trying too select the angle.
	meterCounter = 0;
	meterCounterMax = BAT_SWING_MAX;

	// if power is small, we use bunting animation.
	if(selectedBattingPowerCount < BUNT_THRESHOLD) {
		// and its updated here only if the player is moving already so that the animation wont start too early.
		if(batterMoving == 1) {
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.model = 15;
		}
		// will allow us to move bit further when bunting as the model needs to be closer to ball to look nice.
		batterAdvanceLimit = BUNT_ADVANCE;
		// this will take care that if we indeed were too early for the animation to start yet, the correct animation
		// will start when the batter starts to move.
		battingMode = 1;

	}
	// if power is great enough to give us good swing we'll go with it but thats the default so no need to do anything here.

}
static void selectAngle(StateInfo* stateInfo)
{
	// simple enough, enter the state of waiting for animation to end
	stateInfo->localGameInfo->aF.bTAF.swing = 5;
	// and set angle to be the meterCounter value, its processed further afterwards.
	selectedBattingAngleCount = meterCounter;
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

static void updateBatting(StateInfo* stateInfo)
{
	if(stateInfo->localGameInfo->pRAI.batterReady == 1 && stateInfo->localGameInfo->pRAI.pitchInAir == 0 && stateInfo->localGameInfo->pRAI.battingGoingOn == 0) {
		stateInfo->localGameInfo->pRAI.battingGoingOn = 1;
	}
	// so battingGoingOn goes 1 when batter arrives to its ready position and pitch is not in air.
	if(stateInfo->localGameInfo->pRAI.battingGoingOn == 1) {
		// we increase battingFrameCount from the moment the batter movement started.
		// used for animation and advancing.
		if(increaseBattingFrameCount == 1) {
			battingFrameCount += 1;
		}

		if(stateInfo->localGameInfo->pRAI.initBatter == 1) {
			// so the default is to swing, we change this to BUNT_ADVANCE or SPREAD_ADVANCE
			// if the circumstances need so
			batterAdvanceLimit = SWING_ADVANCE;
			// we start with natural speeds and positions
			batterAngle = 0.0f;
			batterAngleSpeed = 0;
			batterAdvance = 0.0f;
			batterAdvanceSpeed = 0.0f;
			// aren't moving yet
			batterMoving = 0;
			// swinging mode
			battingMode = 0;
			// animation is yet to start
			battingFrameCount = 0;
			// starting of the animation is yet to start
			increaseBattingFrameCount = 0;
			// not stopped yet
			battingStopped = 0;
			// update location and orientation once here
			updateBatterLocationAndOrientation = 1;
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->
			                                     pII.batterIndex].cPI.lastLastLocationUpdate = 1;
			// init done, so no need to do that anymore.
			stateInfo->localGameInfo->pRAI.initBatter = 0;
		}
		// update batters location and orientation when either advancing speed or angular speed
		// is nonzero. moving is 0 so location wont be updated with other players in different parts of code
		// and orientation is not updated elsewhere either as in the other part of code it is checked
		// if player's index is batterIndex before updating orientation. so its done here exclusively.
		if(batterAngleSpeed != 0 || batterAdvanceSpeed != 0) {
			// if the updated angle would be within limits, we can proceed updating the speed.
			// batterAngleSpeed is just 1, 0 or -1 and speed is really given by BATTER_ANGLE_SPEED_CONSTANT.
			if(batterAngle + batterAngleSpeed*BATTER_ANGLE_SPEED_CONSTANT <
			        BATTER_ANGLE_LIMIT && batterAngle + batterAngleSpeed*BATTER_ANGLE_SPEED_CONSTANT >
			        -BATTER_ANGLE_LIMIT ) {
				batterAngle += batterAngleSpeed*BATTER_ANGLE_SPEED_CONSTANT;
				updateBatterLocationAndOrientation = 1;
			}
			// if updated advanced location would be within limit that is originally SWING_ADVANCE but could
			// be changed to BUNT_ADVANCE given small power, then can proceed updating.
			if(batterAdvance + batterAdvanceSpeed < batterAdvanceLimit) {
				batterAdvance += batterAdvanceSpeed;
				updateBatterLocationAndOrientation = 1;
			}
		}
		// if need for update of location and orientation
		if(updateBatterLocationAndOrientation == 1) {
			float dx;
			float dz;
			float dx2;
			float dz2;
			// update lastLocation for smooth movement
			setVectorXZ(&(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].tPI.lastLocation),
			            stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].tPI.location.x,
			            stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].tPI.location.z);
			// update location with sine and cosine to new location on the circle centered at pitch plate.
			// radius will be given by batterAdvance relative to batting radius
			// angle is given by batterAngle and the default ZERO_BETTING_ANGLE
			setVectorXZ(&(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].tPI.location),
			            (float)(stateInfo->fieldPositions->pitchPlate.x + cos(ZERO_BATTING_ANGLE + batterAngle)*(BATTING_RADIUS *
			                    (1 - batterAdvance))),
			            (float)(stateInfo->fieldPositions->pitchPlate.z - sin(ZERO_BATTING_ANGLE + batterAngle)*(BATTING_RADIUS *
			                    (1 - batterAdvance))));
			// and then set the orientation of batter. here we just first select the base direction to be
			// vector from pitchplate to batter and then fix it a bit to make it look more realistic.
			dx = stateInfo->fieldPositions->pitchPlate.x - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].tPI.location.x;
			dz = stateInfo->fieldPositions->pitchPlate.x - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].tPI.location.z;
			dx2 = (float)(cos(BATTER_ANGLE_FIX)*dx - sin(BATTER_ANGLE_FIX)*dz);
			dz2 = (float)(sin(BATTER_ANGLE_FIX)*dx + cos(BATTER_ANGLE_FIX)*dz);
			setVectorXZ(&(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].tPI.orientation), dx2, dz2);

			updateBatterLocationAndOrientation = 0;
		}


		// so now can actually start thinking about advancing as the ball is in air.
		if(stateInfo->localGameInfo->pRAI.pitchInAir == 1 && runBatFlag == 0) {
			// start the animation and advancing.
			if(increaseBattingFrameCount == 0) {
				increaseBattingFrameCount = 1;
			}
			// so at the beginning swing==0. advancing and animation doesnt necessarily start
			// immediately. if pitch is very high the batting animation will take a lot shorter time
			// than what it takes for ball to get down, so the animation and advancing will start a bit
			// later. meter updating on the hand will start immediately and power selection
			// will be available too.
			if(stateInfo->localGameInfo->aF.bTAF.swing == 0) {
				float v = stateInfo->localGameInfo->ballInfo.velocity.y;
				float a = -GRAVITY;
				float s = 0;
				// note decision s=0 makes the landing point actually to be in air,
				// but thats convenient for our purposes. so here we count
				// how many frames will it take for ball to go up and down again so that we can try
				// to time our batting advancing and animation accordingly. just solve 0 = s + vt + (1/2)at^2
				// and choose the correct branch and then add a little experience-based tweak.
				pitchFrameTime = (int)((-v - sqrt(v*v - 2*a*s))/a) + PITCH_FRAME_TIME_TWEAK;
				// Here initialize meterCounter and meterCounter max in a way similar to how we initialized those in pitching.
				// relative distance from the end of meter to the indicator is the same.
				// difference is that these values are scaled a bit, to allow as slow movement of the indicator as possible
				// for batting.
				meterCounter = BAT_SWING_MAX - BAT_LOAD_MAX;
				meterCounterMax = BAT_SWING_MAX;
				// so allow user to select power
				stateInfo->localGameInfo->aF.bTAF.swing = 1;
				// and set batHit and batMiss flags to zero. these are needed in other parts of
				// code.
				stateInfo->localGameInfo->pRAI.batHit = 0;
				stateInfo->localGameInfo->pRAI.batMiss = 0;
				// and batterReady is zero now. batter isnt ready to action
				// anymore, action is with him already.
				stateInfo->localGameInfo->pRAI.batterReady = 0;



			}
			// so if the batter is still not moving, we'll try to figure out if we should be moving.
			if(batterMoving == 0) {
				// so at the moment there is just enough frames left that if we start animation and advancing now the
				// ball will be at right height for the animation look correct.
				if(battingFrameCount > pitchFrameTime - BAT_ANIMATION_FRAME_HIT_COUNT) {
					// set batterMoving flag to 1 so that we can better handle starting points of the
					// animations
					batterMoving = 1;
					// so if we are still to swing
					// select corresponding animation
					if(battingMode == 0) {
						stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.model = 14;
					}
					// to bunt, select bunting animation
					else if(battingMode == 1) {
						stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.model = 15;
					}
					// to stop the batting select the hands spread -animation
					else {
						stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.model = 16;
					}
					stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.animationStage = 0;
					stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.animationStageCount = 34;
					stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.animationFrequency = 3;
					// give player some speed to use for advancing.
					// basically we are just guessing some reasonable speed.
					// but it depends on frame hit count so its explicitly there
					// for clarity
					batterAdvanceSpeed = (float)GENERIC_BATTER_ADVANCE_SPEED_CONSTANT/BAT_ANIMATION_FRAME_HIT_COUNT;
				}
			}
			// if meter indicator reaches the limit, any batting won't happen and
			// we'll just show the animation of player spreading hands.
			if(stateInfo->localGameInfo->aF.bTAF.swing == 1) {
				if(meterCounter - (BAT_SWING_MAX - BAT_LOAD_MAX) >= BAT_LOAD_MAX) {
					// change animation to hands spreading
					if(batterMoving == 1) {
						stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.batterIndex].cPI.model = 16;
					}
					// set battingMode to 2 just in case that the indicator went off the meter so early that
					// the batter didnt even start moving yet, so that we know to choose the right one when moving starts.
					battingMode = 2;
					// advance only a small bit
					batterAdvanceLimit = SPREAD_ADVANCE;
					// set flag to indicate that batting has stopped so that we there wont be checking for if the
					// bat has hit the ball
					battingStopped = 1;
					// set swing to 5 to indicate that theres no
					// further functionality, we just wait for the animation to end.
					stateInfo->localGameInfo->aF.bTAF.swing = 5;
				}
			}

		}
		// so here we check if the animation has ended and if battingGoingOn is still on,
		// so that we dont do this but once. if it is, we set battingGoingOn to zero
		// and move the player towards ready position agian.
		if((battingFrameCount > pitchFrameTime -
		        BAT_ANIMATION_FRAME_HIT_COUNT + BAT_ANIMATION_FRAME_TOTAL_COUNT) &&
		        stateInfo->localGameInfo->pRAI.battingGoingOn == 1) {
			Vector3D target;
			stateInfo->localGameInfo->pRAI.battingGoingOn = 0;

			target.x = (float)(stateInfo->fieldPositions->pitchPlate.x + cos(ZERO_BATTING_ANGLE)*BATTING_RADIUS);
			target.z = (float)(stateInfo->fieldPositions->pitchPlate.z - sin(ZERO_BATTING_ANGLE)*BATTING_RADIUS);
			moveToTarget(stateInfo, stateInfo->localGameInfo->pII.batterIndex, &target);
		}
		// so here we check if the bat hits. this event happens always the pitch has been in air
		else if(battingFrameCount > pitchFrameTime) {
			// so here we continue only if user hasn't decided to not to bat and if we havent bat already.
			if(battingStopped == 0 && stateInfo->localGameInfo->pRAI.batHit == 0 && stateInfo->localGameInfo->pRAI.batMiss == 0) {
				// if ball doesnt go too far away to left or right
				if(stateInfo->localGameInfo->ballInfo.location.x < BALL_MAX_OFFSET && stateInfo->localGameInfo->ballInfo.location.x > -BALL_MAX_OFFSET) {
					float verticalAngle;
					float horizontalAngle;
					float power;
					float scaleNumber;
					float zeroNumber;
					// here is an interesting process of finding the vertical angle ( angle with
					// the horizontal plane ). we'll leave that as an exercise.
					scaleNumber = (float)(selectedBattingPowerCount + (BAT_SWING_MAX - BAT_LOAD_MAX));
					zeroNumber = BAT_SWING_MAX*(1.0f*selectedBattingPowerCount / scaleNumber);
					// ball's y velocity affects the vertical angle
					// y velocity in range 0.12 to 0.20
					// 7 constant that makes 0.12 to 0.20 range to be somewhere around 1.0
					verticalAngle = 7 * stateInfo->localGameInfo->ballInfo.velocity.y * (selectedBattingAngleCount - zeroNumber) * (scaleNumber / BAT_SWING_MAX);
					// 2 to make it possible to bat to every direction on the field and a bit over.
					horizontalAngle = - batterAngle * 2;
					power = (float)selectedBattingPowerCount;
					// so now we have the two angles and the power. now we just need to find a way to
					// convert them nicely to velocities for the ball. there should be other factors
					// to influence velocity too.
					if(verticalAngle > VERTICAL_ANGLE_LIMIT || verticalAngle < - VERTICAL_ANGLE_LIMIT) {
						// we can also just miss, its not so uncommon!
						stateInfo->localGameInfo->pRAI.batMiss = 1;
					} else {
						float dx, dy, dz, magnitude;
						int powerFactor;
						float alfa, theta;
						// verticalAngle in interval -5..5
						// power in interval 0..36
						// horizontalAngle -0.38..0.38
						// ball location x  -1.0 ... 1.0
						// power depends on player's power attribute.
						powerFactor = stateInfo->localGameInfo->
						              playerInfo[stateInfo->localGameInfo->
						                         pII.batterIndex].bTPI.power;
						//magnitude = (0.01f + powerFactor*0.002f)*power;
						magnitude = (0.0125f + powerFactor*0.0015f)*power;
						alfa = (verticalAngle * 2 + 5) * 0.05f;
						// x direction depends on how wrong the pitch was
						theta = horizontalAngle + 0.05f *
						        stateInfo->localGameInfo->ballInfo.location.x;
						dy = (float)(sin(alfa) * cos(theta));
						dz = - (float)(cos(alfa) * cos(theta));
						dx = (float)sin(theta);


						// make the ball fly in the air with new velocity
						genericSlingBall(stateInfo, magnitude*dx, magnitude*dy, magnitude*dz);
						// and the sound
						stateInfo->playSoundEffect = SOUND_SWING;
						// bat hits
						stateInfo->localGameInfo->pRAI.batHit = 1;
						// firstCatchMade set to zero. its used for example to condition checking for runs
						// or out of bounds events.
						stateInfo->localGameInfo->gAI.firstCatchMade = 0;
						// not a pitch anymore
						stateInfo->localGameInfo->pRAI.pitchInAir = 0;
						// pitchGoingOn goes 0 here too.
						stateInfo->localGameInfo->pRAI.pitchGoingOn = 0;
						// this batter has chance to make run now by running to third base.
						stateInfo->localGameInfo->gAI.canMakeRunOfHonor = 1;
						// no throw going on now
						stateInfo->localGameInfo->pRAI.throwGoingToBase = -1;
						// prepare for wounds
						stateInfo->localGameInfo->gAI.woundingCatch = 0;
						stateInfo->localGameInfo->gAI.woundingCatchHandled = 0;
						stateInfo->localGameInfo->gAI.batterStartedRunning = 0;

						// move the batter if wanted

						if(stateInfo->localGameInfo->pRAI.willStartRunning[0] == 1) {
							int index = stateInfo->localGameInfo->pII.safeOnBaseIndex[0];
							stateInfo->localGameInfo->pRAI.willStartRunning[0] = 0;
							if(index != -1) {
								runToNextBase(stateInfo, index, 0);
								runBatFlag = 1;
							}
						}
					}
					// always when batting,
					// we get a strike
					stateInfo->localGameInfo->gAI.strikes += 1;
					stateInfo->localGameInfo->gAI.gameInfoEvent = 5;
				}
				// if the ball went to far away and we still continued our batting
				// we just miss. set the flags, trigger the event and
				// add a strike.
				else {
					stateInfo->localGameInfo->pRAI.batMiss = 1;
					stateInfo->localGameInfo->gAI.strikes += 1;
					stateInfo->localGameInfo->gAI.gameInfoEvent = 5;
				}
			}
		}
	}
}

static void updateMeters(StateInfo* stateInfo)
{
	// when pitch has been started but power not yet selected,
	// we increase meterCounter until its in its maximum
	if(stateInfo->localGameInfo->aF.cTAF.pitch == 2) {
		if(meterCounter < meterCounterMax) {
			meterCounter += 1;
		}
		// meterValue is used to render info to screen for user.
		stateInfo->localGameInfo->pRAI.meterValue = 1.0f*meterCounter / meterCounterMax;
	}
	// when power has been selected but the angle is not yet selected,
	// we increase meterCounter until its in its maximum
	else if(stateInfo->localGameInfo->aF.cTAF.pitch == 4) {
		if(meterCounter < meterCounterMax) {
			meterCounter += 1;
		} else {
			// if counter reaches the maximum, it means animation has
			// reached its end point and indicator on the meter would go off the meter.
			// so when this happnes we terminate the pitch.
			// first we set pitch=0 so that we can start a new pitch
			stateInfo->localGameInfo->aF.cTAF.pitch = 0;
			stateInfo->localGameInfo->aF.cTAF.actionKeyLock = 0;
			// and we set pitchGoingOn to 0 to tell other functionality in the code
			// what happened.
			stateInfo->localGameInfo->pRAI.pitchGoingOn = 0;
			// ball is returned to its position with player
			stateInfo->localGameInfo->ballInfo.location.x = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.x;
			stateInfo->localGameInfo->ballInfo.location.z = stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].tPI.location.z;
			// and we choose the normal model of fielder having a ball.
			stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.hasBallIndex].cPI.model = 1;
		}
		// update what is seen on the screen.
		stateInfo->localGameInfo->pRAI.meterValue = 1.0f - 1.0f*meterCounter / meterCounterMax;
	} else if(throwGoingOn == 1) {
		if(meterCounter < meterCounterMax) {
			meterCounter += 1;
		}
		stateInfo->localGameInfo->pRAI.meterValue = 1.0f*meterCounter / meterCounterMax;
	}
	// when power has yet to be selected but is to be selected we increase the counter
	// and map the value to proper floating point value to let us show it on the screen.
	else if(stateInfo->localGameInfo->aF.bTAF.swing == 1) {
		if(meterCounter < meterCounterMax) {
			meterCounter += 1;
		}
		stateInfo->localGameInfo->pRAI.swingMeterValue = 1.0f*meterCounter / meterCounterMax;
	}
	// when power is selected but angle is to be selected
	else if(stateInfo->localGameInfo->aF.bTAF.swing == 3) {
		float upperLimit;
		float lowerLimit;
		// if the value is still valid, increase it
		if(meterCounter < meterCounterMax) {
			meterCounter += 1;
		}
		// otherwise select angle to be the maximum.
		else {
			selectedBattingAngleCount = meterCounterMax;
		}
		// to map this to the screen we have to think a bit.
		// first of all our meter value goes from 0 to BAT_SWING_MAX
		// and the selectedBattingPowerCount is only from 0 to BAT_LOAD_MAX
		// first calculation just makes it so that upperLimit is basically the position
		// of indicator where it was when the the power was selected. with maxpower it will be 1.
		// with minpower it will be 4/13
		upperLimit = (float)(selectedBattingPowerCount + (BAT_SWING_MAX - BAT_LOAD_MAX)) / BAT_SWING_MAX;
		lowerLimit = 0.0f;
		// then we just map the meterCounter value's range to upperLimit-lowerLimit and inverse it
		stateInfo->localGameInfo->pRAI.swingMeterValue = upperLimit - (1.0f*meterCounter / meterCounterMax) *
		    (upperLimit - lowerLimit);
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
		int pitcherIndex = stateInfo->localGameInfo->pII.catcherOnBaseIndex[0];
		// here we finish pitching if started.
		// here i use these weird lock timeouts. im not sure if they are necessary
		// but they could be. not gonna try anymore.
		if(aiPitchStage == 1) {
			if(aiLockTimeoutCounter == -1) {
				aiLockTimeoutCounter = 0;
			}
			if(meterCounter > aiPitchFirstLimit) {
				aiPitchStage = 2;
				stateInfo->keyStates->imitateKeyPress[KEY_2] = 0;
				aiLockTimeoutCounter = -1;
			} else {
				aiLockTimeoutCounter++;
				if(aiLockTimeoutCounter > TIMEOUT_CONSTANT) {
					aiPitchStage = 0;
					flushKeys(stateInfo);
					aiActionEventLock = AI_NO_LOCK;
					aiLockUpdate = 1;
					aiLockTimeoutCounter = -1;
				}
			}
		} else if(aiPitchStage == 2) {
			if(aiLockTimeoutCounter == -1) {
				aiLockTimeoutCounter = 0;
			}
			if(meterCounter > aiPitchSecondLimit) {
				aiPitchStage = 3;
				stateInfo->keyStates->imitateKeyPress[KEY_2] = 1;
				aiLockTimeoutCounter = -1;
			} else {
				aiLockTimeoutCounter++;
				if(aiLockTimeoutCounter > TIMEOUT_CONSTANT) {
					aiPitchStage = 0;
					flushKeys(stateInfo);
					aiActionEventLock = AI_NO_LOCK;
					aiLockUpdate = 1;
					aiLockTimeoutCounter = -1;
				}
			}
		} else if(aiPitchStage == 3) {
			aiPitchStage = 0;
			flushKeys(stateInfo);
			aiActionEventLock = AI_NO_LOCK;
			aiLockUpdate = 1;
		}
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
		// if pitcher has the ball and he is in correct position
		if(stateInfo->localGameInfo->pII.hasBallIndex == pitcherIndex &&
		        stateInfo->localGameInfo->playerInfo[pitcherIndex].cTPI.isNearHomeLocation == 1) {
			// and lets give player some time to prepare
			if(aiBatterReadyTimer > 70) {
				// try pitching.
				if(aiPitchStage == 0) {
					int i;
					int homeLocationFlag = 1;
					int pitchFlag = 0;

					aiPitchTime++;
					if(aiPitchTime >= 100) {
						pitchFlag = 1;
					}
					for(i = PLAYERS_IN_TEAM + JOKER_COUNT; i < 2*PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
						if(stateInfo->localGameInfo->playerInfo[i].cTPI.isNearHomeLocation == 0) {
							homeLocationFlag = 0;
						}
					}
					if(aiActionEventLock == AI_NO_LOCK && aiLockUpdate == 0) {
						if(homeLocationFlag == 1 && pitchFlag == 1) {
							int rand1 = rand()%15;
							int rand2 = rand()%3;
							int rand3 = rand()%10;
							int var = 0;
							if(stateInfo->localGameInfo->gAI.battingTeamPlayersOnFieldCount == 1) {
								rand1 = 0;
							} else if(stateInfo->localGameInfo->gAI.strikes != 0 && stateInfo->localGameInfo->gAI.balls == 0) {
								if(rand3 == 9) {
									var = 10;
								} else if(rand3 == 8) {
									var = -10;
								}
							}
							aiActionEventLock = AI_PITCH_LOCK;
							aiLockUpdate = 1;
							aiPitchStage = 1;
							flushKeys(stateInfo);
							stateInfo->keyStates->imitateKeyPress[KEY_2] = 1;
							aiPitchFirstLimit = (PITCH_UP_MAX - PITCH_DOWN_MAX)*ANIMATION_FREQUENCY +  5 + rand1;
							aiPitchSecondLimit = ANIMATION_FREQUENCY * PITCH_DOWN_MAX - 2 + rand2 + var;
						} else {
							// to stop player from unnecessarily moving
							flushKeys(stateInfo);
						}
					}
				}
			}
		}
		if(aiLockUpdate == 1) {
			aiLockUpdate = 0;
		}
		if(aiPitchPreviousTime == aiPitchTime) {
			aiPitchTime = 0;
		}
		aiPitchPreviousTime = aiPitchTime;
		// this batterReadyTimer is used to give human player a bit more time before AI pitches.
		if(stateInfo->localGameInfo->pRAI.batterReady == 1 &&
		        stateInfo->localGameInfo->pII.catcherOnBaseIndex[0] == stateInfo->localGameInfo->pII.hasBallIndex &&
		        aiBatterReadyTimer == -1) {
			aiBatterReadyTimer = 0;
		} else if(stateInfo->localGameInfo->pRAI.batterReady == 0) {
			aiBatterReadyTimer = -1;
		}
		if(aiBatterReadyTimer != -1) {
			aiBatterReadyTimer++;
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

static void flushKeys(StateInfo* stateInfo)
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
