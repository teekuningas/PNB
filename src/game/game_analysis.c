/*
 * this module tries to tackle rule-related things.
 */

#include "globals.h"
#include "game_analysis.h"
#include "common_logic.h"
#include "menu_types.h"

#define BASE_RADIUS 2.0f
#define WOUNDING_CATCH_THRESHOLD (1.0f * (1 / (UPDATE_INTERVAL*1.0f/1000)))
#define OUT_OF_BOUNDS_THRESHOLD (2.0f * (1 / (UPDATE_INTERVAL*1.0f/1000)))

static int woundingCatchCounter;
static int outOfBoundsCounter;
static int endOfInningCounter;
static int nextPairCounter;
static int foulPlayEventFlag;
static int homeRunCameraCounter;

static void checkForOuts(StateInfo* stateInfo);
static void checkIfNextBatterDecision(StateInfo* stateInfo);
static void strikesAndBalls(StateInfo* stateInfo);
static void checkIfEndOfInning(StateInfo* stateInfo, MenuInfo* menuInfo);
static void woundingCatchEffects(StateInfo* stateInfo);
static void foulPlay(StateInfo* stateInfo);
static void checkForRuns(StateInfo* stateInfo);
static void checkIfNextPair(StateInfo* stateInfo);


void initGameAnalysis(StateInfo* stateInfo)
{
	// init some variables only used here.
	woundingCatchCounter = -1;
	outOfBoundsCounter = 0;
	endOfInningCounter = -1;
	nextPairCounter = -1;
	foulPlayEventFlag = 0;
	homeRunCameraCounter = -1;
}

void gameAnalysis(StateInfo* stateInfo, MenuInfo* menuInfo)
{
	if(stateInfo->localGameInfo->gAI.initLocals > 0) {
		initGameAnalysis(stateInfo);
		stateInfo->localGameInfo->gAI.initLocals++;
		if(stateInfo->localGameInfo->gAI.initLocals == INIT_LOCALS_COUNT) {
			stateInfo->localGameInfo->gAI.initLocals = 0;
		}
	}
	// when player from third base starts running, we change camera view. when the situation is over we
	// wait 50 update frames, before moving to normal camera
	if(homeRunCameraCounter >= 0) {
		homeRunCameraCounter++;
		if(homeRunCameraCounter > 50) {
			stateInfo->localGameInfo->gAI.homeRunCameraFlag = 0;
			homeRunCameraCounter = -1;
		}
	}

	checkForOuts(stateInfo);
	checkIfNextBatterDecision(stateInfo);
	strikesAndBalls(stateInfo);
	woundingCatchEffects(stateInfo);
	foulPlay(stateInfo);
	checkForRuns(stateInfo);
	checkIfEndOfInning(stateInfo, menuInfo);
	checkIfNextPair(stateInfo);

}

static void checkForOuts(StateInfo* stateInfo)
{
	// for the check of end of inning we set this to 0 on every update and if ball happens to be on home base then we put it to 1.
	stateInfo->localGameInfo->gAI.ballHome = 0;
	// so here we are going to check if we ball is at some base and if that will imply with other conditions that
	// there will be an out and player is to be removed from the field.
	if(stateInfo->localGameInfo->pII.hasBallIndex != -1) {
		float dx;
		float dz;
		int i;
		// if canMakeRunOfHonor changes to 0, we will update that 0 to corresponding flag in stateInfo
		int canMakeRunOfHonor = 1;
		for(i = 0; i < BASE_COUNT; i++) {
			int smallEnough = 0;
			// so for every base we check if ball is near enough and if some player has it
			if( i > 0) {
				dx = stateInfo->localGameInfo->ballInfo.location.x - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.catcherOnBaseIndex[i]].tPI.homeLocation.x;
				dz = stateInfo->localGameInfo->ballInfo.location.z - stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.catcherOnBaseIndex[i]].tPI.homeLocation.z;
				if(isVectorSmallEnoughCircleXZ(dx, dz, BASE_RADIUS) == 1) smallEnough = 1;
			} else {
				// home base's structure is a bit different so we check if ball is inside some homeline-middlepoint centered
				// disk and at the homebase side of the homeline.
				dx = stateInfo->localGameInfo->ballInfo.location.x - stateInfo->fieldPositions->pitchPlate.x;
				dz = stateInfo->localGameInfo->ballInfo.location.z - HOME_LINE_Z;
				if(stateInfo->localGameInfo->ballInfo.location.z > HOME_LINE_Z && isVectorSmallEnoughCircleXZ(dx, dz, HOME_RADIUS)) {
					smallEnough = 1;
					stateInfo->localGameInfo->gAI.ballHome = 1;
					if(homeRunCameraCounter == -1 && stateInfo->localGameInfo->gAI.homeRunCameraFlag == 1 &&
					        (stateInfo->localGameInfo->pII.safeOnBaseIndex[3] == -1 ||
					         stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.safeOnBaseIndex[3]].
					         bTPI.isOnBase == 1)) {
						homeRunCameraCounter = 0;
					}
				}
			}
			// if so, the next phase is to check if there is a runner that will be affected by this.
			if(smallEnough == 1) {
				int j;
				canMakeRunOfHonor = 0;
				for(j = 0; j < BASE_COUNT; j++) {
					int index = stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[j];
					if(index != -1) {
						int baseIndex;
						// remove safety from last base?
						// happens if player is out of base and ball arrives the previous one.
						if(stateInfo->localGameInfo->pII.safeOnBaseIndex[i] == index) {
							if(stateInfo->localGameInfo->playerInfo[index].bTPI.isOnBase == 0) {
								// in case player was leading of coming back, we now force him to continue
								// to next base
								runToNextBase(stateInfo, stateInfo->localGameInfo->pII.safeOnBaseIndex[i], i);
								// put safety to -1, also makes it so that the player cant be controlled until he arrives the
								// next base
								stateInfo->localGameInfo->pII.safeOnBaseIndex[i] = -1;
								// if player was batter of the homebase, then he cannot come back to do batting anymore.
								// unless of course it somehow was possible to do foul play and have ball that at the homebase
								// when batter leaves the base. but that doesnt matter as batterIndex will be set from the scratch
								// when handling out of bounds situation.
								if(stateInfo->localGameInfo->pII.batterIndex == index) stateInfo->localGameInfo->pII.batterIndex = -1;

							}
						}
						// here we check for actual outs.
						// set baseIndex to previous base, because player will have that base
						// when trying to run to this base we are checking.
						if(i > 0) baseIndex = i - 1;
						else baseIndex = 3;

						if(stateInfo->localGameInfo->playerInfo[index].bTPI.base == baseIndex) {
							// no way to get checking this if isOnBase == 1, as if player's base is the previous base
							// and he is on the base, he is safe. if he is not safe, he will automatically also
							// not be on a base as he is running to enxt one.
							if(stateInfo->localGameInfo->playerInfo[index].bTPI.isOnBase == 0) {
								// cant make these normal outs if out of bounds or taking free walk
								if(stateInfo->localGameInfo->playerInfo[index].bTPI.takingFreeWalk == 0 &&
								        stateInfo->localGameInfo->gAI.outOfBounds == 0) {
									// we set player's out flag so that his movement is easy to control
									stateInfo->localGameInfo->playerInfo[index].bTPI.out = 1;
									// send a message to screen
									stateInfo->localGameInfo->gAI.gameInfoEvent = 1;
									// add out
									stateInfo->localGameInfo->gAI.outs += 1;
									// remove player from the array
									stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[j] = -1;
									stateInfo->localGameInfo->gAI.battingTeamPlayersOnFieldCount--;
									// and walk him out.
									movePlayerOut(stateInfo, index);
									// if we was safe on previous base, now he is not anymore.
									if(stateInfo->localGameInfo->pII.safeOnBaseIndex[baseIndex] == index) {
										stateInfo->localGameInfo->pII.safeOnBaseIndex[baseIndex] = -1;
									}
									// if he was a batter, he is not anymore.
									if(stateInfo->localGameInfo->pII.batterIndex == index) stateInfo->localGameInfo->pII.batterIndex = -1;
								}
							}
						}
						// if the batter manages to get over second base he still has chance
						// to make run even if ball has been to basecatchers with the exception of
						// third base.
						if(stateInfo->localGameInfo->playerInfo[index].bTPI.base >= 2 &&
						        stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase == 0 && i != 3) {
							canMakeRunOfHonor = 1;
						}

					}
				}
			}

		}
		if(canMakeRunOfHonor == 0) {
			stateInfo->localGameInfo->gAI.canMakeRunOfHonor = 0;
		}
	}

}

static void checkIfNextBatterDecision(StateInfo* stateInfo)
{
	// so this function's idea is to make progress in selecting a new batter if old one's gone.
	// so this will be called only once when possible.
	if(stateInfo->globalGameInfo->period >= 4) {

	} else if(stateInfo->localGameInfo->pII.batterIndex == -1 && stateInfo->localGameInfo->gAI.waitingForBatterDecision == 0 &&
	          endOfInningCounter == -1) {
		// there have to be a player available
		if(stateInfo->localGameInfo->gAI.nonJokerPlayersLeft + stateInfo->localGameInfo->gAI.jokersLeft > 0) {
			// have to check that there is only three players in the field too and that it is not a out of bounds situation.
			if(stateInfo->localGameInfo->gAI.battingTeamPlayersOnFieldCount < BASE_COUNT && stateInfo->localGameInfo->gAI.outOfBounds == 0) {
				// also we cannot know yet if it will be out of position situation so we have to wait that the ball will land
				// in some way.
				if(stateInfo->localGameInfo->ballInfo.hasHitGround == 1 || stateInfo->localGameInfo->gAI.firstCatchMade == 1) {
					// if that happens we can now start.
					int battingTeamIndex = (stateInfo->globalGameInfo->
					                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
					// this will give work to action_invocatin.c and action_implementation.c
					stateInfo->localGameInfo->gAI.waitingForBatterDecision = 1;
					// we just select the batterSelectionIndex here. if there are nonJokerPlayerLeft, we
					// just select the next batter in order there. if not, we select the first joker we find that is still unused.
					// one of these must be true, as we checked there is joker or non-joker left before.
					if(stateInfo->localGameInfo->gAI.nonJokerPlayersLeft != 0) {
						stateInfo->localGameInfo->pII.batterSelectionIndex =
						    stateInfo->globalGameInfo->teams[battingTeamIndex].batterOrder[stateInfo->globalGameInfo->teams[battingTeamIndex].batterOrderIndex];
					} else {
						int i;
						for(i = 0; i < JOKER_COUNT; i++) {
							if(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.jokerIndices[i]].bTPI.joker == 1) {
								stateInfo->localGameInfo->pII.batterSelectionIndex =
								    stateInfo->localGameInfo->pII.jokerIndices[i];
								break;
							}
						}
					}
				}
			}
		} else {
			stateInfo->localGameInfo->gAI.noMorePlayers = 1;
		}
	}
}
// so here we are just updating strikes and balls related stuff. batter cant have more than 3 strikes, so something must be
// done to that, and if pitcher pitches balls, that isnt allowed without some compensation either.
static void strikesAndBalls(StateInfo* stateInfo)
{
	// so if there are three strikes
	if(stateInfo->localGameInfo->gAI.strikes == 3) {
		// we force running of batter
		if(stateInfo->localGameInfo->pII.safeOnBaseIndex[0] != -1) {
			runToNextBase(stateInfo, stateInfo->localGameInfo->pII.safeOnBaseIndex[0], 0);
			stateInfo->localGameInfo->pII.batterIndex = -1;
			// and set safeOnBaseIndex to 0 as there is no one safe anymore.
			stateInfo->localGameInfo->pII.safeOnBaseIndex[0] = -1;
		}
	}
	// we calculate the player and the base he has right to go freely only once, and that is when
	// the ball happens. if player moves to next base and user after that decides to make the free walk
	// that wont have any effect.
	if(stateInfo->localGameInfo->gAI.freeWalkCalculationMade == 0) {
		if(stateInfo->localGameInfo->gAI.battingTeamPlayersOnFieldCount == 1) {
			// if only one player on the field, thats the batter, and then free walks can be made after one pitch.
			if(stateInfo->localGameInfo->gAI.balls >= 1) {
				// calculate the index and the base.
				calculateFreeWalk(stateInfo);
				// and tell action_implementation.c to take care of the rest.
				stateInfo->localGameInfo->gAI.waitingForFreeWalkDecision = 1;

			}
		} else {
			// otherwise there is some non-batter leadrunner and he can have free walks after too balls.
			if(stateInfo->localGameInfo->gAI.balls >= 2) {
				// calculate the index and the base.
				calculateFreeWalk(stateInfo);
				// and tell action_implementation.c to take care of the rest.
				stateInfo->localGameInfo->gAI.waitingForFreeWalkDecision = 1;
			}
		}
		stateInfo->localGameInfo->gAI.freeWalkCalculationMade = 1;
	} else {
		// so that if player just ran without taking hiw free walk, and got wounded or out, then stop asking
		// for the free walk.
		if(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->gAI.freeWalkIndex].bTPI.wounded == 1 ||
		        stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->gAI.freeWalkIndex].bTPI.out == 1) {
			stateInfo->localGameInfo->gAI.waitingForFreeWalkDecision = 0;
		}
	}
}
// so in game_manipulation we set woundingCatch flag to 1 when ball is caught after being hit by a bat and flying
// directly to glove.
// difficulty here is that we dont want it to wound player if the ball is dropped to ground by catching player
// after a short time from catching moment.
// so we have to use a counter to wait until this short time has gone and then we'll declare it as a real wound.
static void woundingCatchEffects(StateInfo* stateInfo)
{
	// so we check the flag, if its true and then set counter to zero to start counting and
	// also set hitsGroundToUnWound to 0 so that we can see if that changes in this short time period.
	if(stateInfo->localGameInfo->gAI.woundingCatch == 1 && stateInfo->localGameInfo->gAI.woundingCatchHandled == 0) {
		int i;
		woundingCatchCounter = 0;
		stateInfo->localGameInfo->ballInfo.hitsGroundToUnWound = 0;
		stateInfo->localGameInfo->gAI.woundingCatchHandled = 1;

		for(i = 0; i < BASE_COUNT; i++) {
			// so we check every batting team player.
			int index = stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[i];
			if(index != -1) {
				// if player is taking a free walk its always not wound. if not and ball is out of base,
				// its a wound, its also wound if the player has arrived the next base already.
				if((stateInfo->localGameInfo->playerInfo[index].bTPI.isOnBase == 0 ||
				        stateInfo->localGameInfo->playerInfo[index].bTPI.base != stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase) &&
				        stateInfo->localGameInfo->playerInfo[index].bTPI.takingFreeWalk == 0) {
					stateInfo->localGameInfo->playerInfo[index].bTPI.woundedApply = 1;
				}
			}
		}


	}
	if(woundingCatchCounter >= 0) {
		int threshold;
		woundingCatchCounter++;
		// and we extend the time a bit if ball is not with the player anymore.
		if(stateInfo->localGameInfo->pII.hasBallIndex == -1) {
			threshold = (int)(2*WOUNDING_CATCH_THRESHOLD);
		} else threshold = (int)WOUNDING_CATCH_THRESHOLD;
		// if unWounding happens, then stop the counter and continue the game normally.
		if(stateInfo->localGameInfo->ballInfo.hitsGroundToUnWound == 1) {
			int i;
			woundingCatchCounter = -1;
			for(i = 0; i < BASE_COUNT; i++) {
				int index = stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[i];
				if(index != -1) {
					stateInfo->localGameInfo->playerInfo[index].bTPI.woundedApply = 0;
				}
			}
		}
		// otherwise there is a real possibility for wounding
		// and we check if there are players that are out of base etc at that moment.
		if(woundingCatchCounter > threshold) {
			int i;
			for(i = 0; i < BASE_COUNT; i++) {
				// so we check every batting team player.
				int index = stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[i];
				if(index != -1) {
					if(stateInfo->localGameInfo->playerInfo[index].bTPI.woundedApply == 1) {
						int base = stateInfo->localGameInfo->playerInfo[index].bTPI.base;
						// set wounded=1 so that movement will be handled correctly.
						stateInfo->localGameInfo->playerInfo[index].bTPI.wounded = 1;
						// info to screen.
						stateInfo->localGameInfo->gAI.gameInfoEvent = 2;
						// no safe on previous base anymore.
						if(stateInfo->localGameInfo->pII.safeOnBaseIndex[base] == index) {
							stateInfo->localGameInfo->pII.safeOnBaseIndex[base] = -1;
						}
						// try to avoid out by running if not on next base yet.
						// rest of the wound handling code will be in the base arrivals.
						if(base == stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase) {
							runToNextBase(stateInfo, index, base);
						}
						// if already on the next base, just remove from the base. wounded already set.
						else {
							stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[i] = -1;
							stateInfo->localGameInfo->gAI.battingTeamPlayersOnFieldCount--;
							movePlayerOut(stateInfo, index);
						}
						stateInfo->localGameInfo->playerInfo[index].bTPI.woundedApply = 0;

					}
				}
			}
			stateInfo->localGameInfo->gAI.woundingCatch = 0;
			woundingCatchCounter = -1;
		}
	}
}
// so in case of foul play, we will stop the game
// return players to their original bases and start again with the screen of pitcher getting ball.
static void foulPlay(StateInfo* stateInfo)
{
	// so if outOfBounds == 1 which has been checked and set when ball lands in game_manipulation
	if(stateInfo->localGameInfo->gAI.outOfBounds == 1) {
		// we use a counter so that there is some time to realize what happened.
		outOfBoundsCounter += 1;
		// and send some info to screen. and do that only once.
		if(foulPlayEventFlag == 0) {
			stateInfo->localGameInfo->gAI.gameInfoEvent = 4;
			foulPlayEventFlag = 1;
		}
		if(outOfBoundsCounter > OUT_OF_BOUNDS_THRESHOLD) {
			int j;
			outOfBoundsCounter = 0;
			foulPlayEventFlag = 0;
			stateInfo->localGameInfo->gAI.outOfBounds = 0;

			// so now initialize everything like in beginning of the inning except important non-volatile stuff
			// like runs in the inning and outs. we cannot initialize originalBases either as players' bases
			// have to bet set to their originalBases here later on.
			initializeBallInfo(stateInfo);
			initializeActionInfo(stateInfo);
			initializeTemporaryGameAnalysisInfo(stateInfo);

			initializeIndexInformation(stateInfo);
			initializePRAIInformation(stateInfo);
			initializeSpatialPlayerInformation(stateInfo);

			initializeNonCriticalPlayerInformation(stateInfo);

			if(stateInfo->globalGameInfo->period >= 4) {
				// when running through homerun-batting contest, we have to
				// do a bit special initialization as our setup in setRunnerAndBatter()
				// depends on the field being empty.
				int i;
				for(i = 0; i < BASE_COUNT; i++) {
					stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[i] = -1;
				}
				initializeCriticalBattingTeamInformation(stateInfo);
				setRunnerAndBatter(stateInfo);
			} else {
				// every players' locations etc got initialized just now. so at the moment our batting team players
				// will all be around home base. But we dont want all players to be there
				// so we have to do some modifications accoding to originalbases.
				for(j = 0; j < BASE_COUNT; j++) {
					int index = stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[j];
					if(index != -1) {
						// so we set every batting team player 's, who was on the field, bases to originalBases
						// and set them to be at a base. if his originalBase was -1, it means
						// that when the pitch was pitched, he had not safety on any base
						// and he will be out.
						if(stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase == -1) {
							stateInfo->localGameInfo->gAI.gameInfoEvent = 1;
							stateInfo->localGameInfo->gAI.outs += 1;

							stateInfo->localGameInfo->playerInfo[index].bTPI.base = -1;
							stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[j] = -1;
							stateInfo->localGameInfo->gAI.battingTeamPlayersOnFieldCount--;

							continue;
						} else {
							stateInfo->localGameInfo->playerInfo[index].bTPI.base =
							    stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase;
							stateInfo->localGameInfo->playerInfo[index].bTPI.isOnBase = 1;
						}

						// in case that player was taking a free walk from third base when this happened
						if(stateInfo->localGameInfo->playerInfo[index].bTPI.base == 4) {
							int battingTeamIndex = (stateInfo->globalGameInfo->
							                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
							// we will get a run.
							stateInfo->globalGameInfo->teams[battingTeamIndex].runs += 1;
							stateInfo->localGameInfo->gAI.runsInTheInning += 1;
							// and send a message that run was made to screen.
							stateInfo->localGameInfo->gAI.gameInfoEvent = 3;
							// and remove player from the field.
							stateInfo->localGameInfo->playerInfo[index].bTPI.base = -1;
							stateInfo->localGameInfo->playerInfo[index].bTPI.isOnBase = 0;
							stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[j] = -1;
							stateInfo->localGameInfo->gAI.battingTeamPlayersOnFieldCount--;
							// always when two runs is got, we will get a new round of batters.
							if(stateInfo->localGameInfo->gAI.runsInTheInning%2 == 0) {
								stateInfo->localGameInfo->gAI.nonJokerPlayersLeft = PLAYERS_IN_TEAM;
							}
						}
						// if this player is a batter
						else if(stateInfo->localGameInfo->playerInfo[index].bTPI.base == 0) {
							if(stateInfo->localGameInfo->gAI.strikes == 3) {
								// if out of bounds follows his third strike, we well get a out.
								stateInfo->localGameInfo->gAI.gameInfoEvent = 1;
								stateInfo->localGameInfo->gAI.outs += 1;
								// remove from the field.
								stateInfo->localGameInfo->playerInfo[index].bTPI.base = -1;
								stateInfo->localGameInfo->playerInfo[index].bTPI.isOnBase = 0;
								stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[j] = -1;
								stateInfo->localGameInfo->gAI.battingTeamPlayersOnFieldCount--;
								// new batter needed.
								stateInfo->localGameInfo->pII.batterIndex = -1;
							} else {
								// otherwise, this player will continue batting.
								stateInfo->localGameInfo->pII.batterIndex = index;
								// preparing left to function that will do it just fine.
								prepareBatter(stateInfo);
							}
						}
						// other bases straightforwardly.
						else if(stateInfo->localGameInfo->playerInfo[index].bTPI.base == 1) {
							stateInfo->localGameInfo->playerInfo[index].tPI.location.x =
							    stateInfo->fieldPositions->firstBaseRun.x;
							stateInfo->localGameInfo->playerInfo[index].tPI.location.z =
							    stateInfo->fieldPositions->firstBaseRun.z;
							stateInfo->localGameInfo->pII.safeOnBaseIndex[1] = index;
						} else if(stateInfo->localGameInfo->playerInfo[index].bTPI.base == 2) {
							stateInfo->localGameInfo->playerInfo[index].tPI.location.x =
							    stateInfo->fieldPositions->secondBaseRun.x;
							stateInfo->localGameInfo->playerInfo[index].tPI.location.z =
							    stateInfo->fieldPositions->secondBaseRun.z;
							stateInfo->localGameInfo->pII.safeOnBaseIndex[2] = index;
						} else if(stateInfo->localGameInfo->playerInfo[index].bTPI.base == 3) {
							stateInfo->localGameInfo->playerInfo[index].tPI.location.x =
							    stateInfo->fieldPositions->thirdBaseRun.x;
							stateInfo->localGameInfo->playerInfo[index].tPI.location.z =
							    stateInfo->fieldPositions->thirdBaseRun.z;
							stateInfo->localGameInfo->pII.safeOnBaseIndex[3] = index;
						}
					}
				}
			}
		}
	}
}
// runs are checked in a delayed way. We wait that ball lands by being catched or by hitting the ground before
// we decide if player has made a run by arriving homebase or arriving third base.
static void checkForRuns(StateInfo* stateInfo)
{
	if(stateInfo->localGameInfo->gAI.checkForRun == 1) {
		// check runs only after we know if the runner could have been wounded.
		if((stateInfo->localGameInfo->gAI.firstCatchMade == 1 ||
		        stateInfo->localGameInfo->ballInfo.hasHitGround == 1) && woundingCatchCounter == -1 &&
		        endOfInningCounter == -1) {
			// if its out of bounds, no love
			if(stateInfo->localGameInfo->gAI.outOfBounds == 0) {
				int j;
				int battingTeamIndex = (stateInfo->globalGameInfo->
				                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
				int catchingTeamIndex = (battingTeamIndex+1)%2;
				for(j = 0; j < BASE_COUNT; j++) {
					int index = stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[j];
					if(index != -1) {
						int runMade = 0;
						// so if player is at homebase
						if(stateInfo->localGameInfo->playerInfo[index].bTPI.base == 4) {
							// remove player from the field.
							stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[j] = -1;
							stateInfo->localGameInfo->gAI.battingTeamPlayersOnFieldCount--;
							stateInfo->localGameInfo->playerInfo[index].bTPI.base = -1;
							// if he isn't wounded
							if(stateInfo->localGameInfo->playerInfo[index].bTPI.wounded == 0) {
								// add a run
								stateInfo->globalGameInfo->teams[battingTeamIndex].runs += 1;
								stateInfo->localGameInfo->gAI.runsInTheInning += 1;
								runMade = 1;
								if(stateInfo->localGameInfo->gAI.runsInTheInning%2 == 0) {
									stateInfo->localGameInfo->gAI.nonJokerPlayersLeft = PLAYERS_IN_TEAM;
									stateInfo->localGameInfo->gAI.noMorePlayers = 0;
								}
								// set info to screen
								stateInfo->localGameInfo->gAI.gameInfoEvent = 3;
							}

						}
						// if the player arrives third base and has been batter with this same pitch.
						else if(stateInfo->localGameInfo->playerInfo[index].bTPI.base == 3 &&
						        stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase == 0 &&
						        stateInfo->localGameInfo->gAI.canMakeRunOfHonor == 1) {
							// cant have run if wounded
							// nor if he has made a run before. otherwise could happen that
							// he arrives third base and makes run and the runner previously on third base run
							// home and then we will get two runs for a total of 3.
							if(stateInfo->localGameInfo->playerInfo[index].bTPI.wounded == 0 &&
							        stateInfo->localGameInfo->playerInfo[index].bTPI.hasMadeRunOnThirdBase == 0) {
								stateInfo->localGameInfo->playerInfo[index].bTPI.hasMadeRunOnThirdBase = 1;
								stateInfo->globalGameInfo->teams[battingTeamIndex].runs += 1;
								stateInfo->localGameInfo->gAI.runsInTheInning += 1;
								runMade = 1;
								if(stateInfo->localGameInfo->gAI.runsInTheInning%2 == 0) {
									stateInfo->localGameInfo->gAI.nonJokerPlayersLeft = PLAYERS_IN_TEAM;
								}
								stateInfo->localGameInfo->gAI.gameInfoEvent = 3;
							}
						}
						// if run was made and and this was last half-inning of the period and batting team
						// has more runs than fielding team, then fielding team cannot win this period anymore
						// and we'll quit.
						if(runMade == 1) {
							if(stateInfo->globalGameInfo->period < 4) {
								if((stateInfo->globalGameInfo->inning + 1)%stateInfo->globalGameInfo->inningsInPeriod == 0 ||
								        stateInfo->globalGameInfo->inning + 1 == stateInfo->globalGameInfo->inningsInPeriod * 2 + 2) {
									if(stateInfo->globalGameInfo->teams[battingTeamIndex].runs >
									        stateInfo->globalGameInfo->teams[catchingTeamIndex].runs) {
										stateInfo->localGameInfo->gAI.endPeriod = 1;
									}
									if(stateInfo->globalGameInfo->inning + 1 == stateInfo->globalGameInfo->inningsInPeriod*2 &&
									        stateInfo->globalGameInfo->teams[battingTeamIndex].period0Runs >
									        stateInfo->globalGameInfo->teams[catchingTeamIndex].period0Runs &&
									        stateInfo->globalGameInfo->teams[catchingTeamIndex].runs ==
									        stateInfo->globalGameInfo->teams[battingTeamIndex].runs ) {
										stateInfo->localGameInfo->gAI.endPeriod = 1;
									}
								}
							} else {
								if((stateInfo->globalGameInfo->inning + 1)%2 == 0) {
									if(stateInfo->globalGameInfo->teams[battingTeamIndex].runs >
									        stateInfo->globalGameInfo->teams[catchingTeamIndex].runs) {
										stateInfo->localGameInfo->gAI.endPeriod = 1;
									}
								}
							}
						}
					}
				}
			}
			// we set checkForRun to 0 after ball has landed.
			stateInfo->localGameInfo->gAI.checkForRun = 0;
		}
	}
}


static void checkIfEndOfInning(StateInfo* stateInfo, MenuInfo* menuInfo)
{
	// if three outs or
	// no more players to bat. set flag on the player selection to indicate that no more players left.
	// then if ball comes to pitcher, then we can quit this inning.
	if(stateInfo->localGameInfo->gAI.outs >= 3 || (stateInfo->localGameInfo->gAI.noMorePlayers == 1 &&
	        stateInfo->localGameInfo->gAI.ballHome == 1) || stateInfo->localGameInfo->gAI.endPeriod == 1 ||
	        (stateInfo->globalGameInfo->period >= 4 && stateInfo->localGameInfo->gAI.runnerBatterPairCounter >=
	         stateInfo->globalGameInfo->pairCount)) {

		if(endOfInningCounter == -1) {
			endOfInningCounter = 0;
			// so that user wont be prompted for this after inning has ended but screen hasnt changed yet.
			stateInfo->localGameInfo->gAI.waitingForBatterDecision = 0;
			stateInfo->localGameInfo->gAI.gameInfoEvent = 7;
		}
	}
	if(endOfInningCounter != -1) {
		endOfInningCounter++;
	}
	// basically here we just list the different kind of ending alternatives and figure out if this is one of them.
	if(endOfInningCounter > 200) {
		int battingTeamIndex = (stateInfo->globalGameInfo->
		                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
		int catchingTeamIndex = (battingTeamIndex+1)%2;

		endOfInningCounter = -1;
		stateInfo->globalGameInfo->inning++;
		// if first period ending
		if(stateInfo->globalGameInfo->inning == stateInfo->globalGameInfo->inningsInPeriod ||
		        (stateInfo->globalGameInfo->inning == stateInfo->globalGameInfo->inningsInPeriod - 1 &&
		         stateInfo->globalGameInfo->teams[catchingTeamIndex].runs >
		         stateInfo->globalGameInfo->teams[battingTeamIndex].runs)) {
			int i;
			stateInfo->globalGameInfo->period = 1;
			for(i = 0; i < 2; i++) {
				stateInfo->globalGameInfo->teams[i].period0Runs = stateInfo->globalGameInfo->teams[i].runs;
				stateInfo->globalGameInfo->teams[i].runs = 0;
			}
			if(stateInfo->globalGameInfo->inning == stateInfo->globalGameInfo->inningsInPeriod - 1) {
				stateInfo->globalGameInfo->inning++; // have to skip the last half-inning
			}
			menuInfo->mode = MENU_ENTRY_INTER_PERIOD;
			stateInfo->screen = MAIN_MENU;
			stateInfo->changeScreen = 1;
			stateInfo->updated = 0;
		}
		// if second period ending
		else if(stateInfo->globalGameInfo->inning == stateInfo->globalGameInfo->inningsInPeriod*2 ||
		        (stateInfo->globalGameInfo->inning == stateInfo->globalGameInfo->inningsInPeriod*2 - 1 &&
		         (stateInfo->globalGameInfo->teams[catchingTeamIndex].runs >
		          stateInfo->globalGameInfo->teams[battingTeamIndex].runs || (stateInfo->globalGameInfo->teams[catchingTeamIndex].period0Runs >
		                  stateInfo->globalGameInfo->teams[battingTeamIndex].period0Runs && stateInfo->globalGameInfo->teams[catchingTeamIndex].runs ==
		                  stateInfo->globalGameInfo->teams[battingTeamIndex].runs )))) {
			int i;
			int team0period0runs = stateInfo->globalGameInfo->teams[0].period0Runs;
			int team0period1runs = stateInfo->globalGameInfo->teams[0].runs;
			int team1period0runs = stateInfo->globalGameInfo->teams[1].period0Runs;
			int team1period1runs = stateInfo->globalGameInfo->teams[1].runs;
			// is the game over already?
			if( team0period0runs>=team1period0runs && team0period1runs>=team1period1runs &&
			        (team0period0runs != team1period0runs || team0period1runs != team1period1runs)) {
				menuInfo->mode = MENU_ENTRY_GAME_OVER;
				stateInfo->globalGameInfo->winner = 0;
			} else if( team0period0runs<=team1period0runs && team0period1runs<=team1period1runs &&
			           (team0period0runs != team1period0runs || team0period1runs != team1period1runs)) {
				menuInfo->mode = MENU_ENTRY_GAME_OVER;
				stateInfo->globalGameInfo->winner = 1;
			} else {
				stateInfo->globalGameInfo->period = 2;
				menuInfo->mode = MENU_ENTRY_SUPER_INNING;
			}
			if(stateInfo->globalGameInfo->inning == stateInfo->globalGameInfo->inningsInPeriod*2 - 1) {
				stateInfo->globalGameInfo->inning++; // have to skip the last half-inning
			}
			for(i = 0; i < 2; i++) {
				stateInfo->globalGameInfo->teams[i].period1Runs = stateInfo->globalGameInfo->teams[i].runs;
				stateInfo->globalGameInfo->teams[i].runs = 0;
			}
			stateInfo->screen = MAIN_MENU;
			stateInfo->changeScreen = 1;
			stateInfo->updated = 0;
		}
		// if super inning ending
		else if(stateInfo->globalGameInfo->inning == stateInfo->globalGameInfo->inningsInPeriod*2 + 2) {
			int i;
			// is the game over already?
			if(stateInfo->globalGameInfo->teams[0].runs > stateInfo->globalGameInfo->teams[1].runs) {
				menuInfo->mode = MENU_ENTRY_GAME_OVER;
				stateInfo->globalGameInfo->winner = 0;
			} else if(stateInfo->globalGameInfo->teams[0].runs < stateInfo->globalGameInfo->teams[1].runs) {
				menuInfo->mode = MENU_ENTRY_GAME_OVER;
				stateInfo->globalGameInfo->winner = 1;
			}
			// if not, we move to homerun-batting contest
			else {
				stateInfo->globalGameInfo->period = 4;
				menuInfo->mode = MENU_ENTRY_HOMERUN_CONTEST;
			}

			for(i = 0; i < 2; i++) {
				stateInfo->globalGameInfo->teams[i].period2Runs = stateInfo->globalGameInfo->teams[i].runs;
				stateInfo->globalGameInfo->teams[i].runs = 0;
			}

			stateInfo->screen = MAIN_MENU;
			stateInfo->changeScreen = 1;
			stateInfo->updated = 0;
		}
		// is homerun-batting contest moving to next stage or ending
		else if(stateInfo->globalGameInfo->period >= 4 && (stateInfo->globalGameInfo->inning)%2 == 0) {
			int i;
			// is the game over already?
			if(stateInfo->globalGameInfo->teams[0].runs > stateInfo->globalGameInfo->teams[1].runs) {
				menuInfo->mode = MENU_ENTRY_GAME_OVER;
				stateInfo->globalGameInfo->winner = 0;
			} else if(stateInfo->globalGameInfo->teams[0].runs < stateInfo->globalGameInfo->teams[1].runs) {
				menuInfo->mode = MENU_ENTRY_GAME_OVER;
				stateInfo->globalGameInfo->winner = 1;
			} else {
				// +=2 because we want to use 4, 6, 8... for homerun batting contest periods
				// as we dont want to mess the team ordering when
				// calculating those battingTeamIndices.
				stateInfo->globalGameInfo->period+=2;
				menuInfo->mode = MENU_ENTRY_HOMERUN_CONTEST;
			}

			for(i = 0; i < 2; i++) {
				stateInfo->globalGameInfo->teams[i].period3Runs += stateInfo->globalGameInfo->teams[i].runs;
				stateInfo->globalGameInfo->teams[i].runs = 0;
			}

			stateInfo->screen = MAIN_MENU;
			stateInfo->changeScreen = 1;
			stateInfo->updated = 0;
		}
		if(stateInfo->screen != MAIN_MENU) loadMutableWorldSettings(stateInfo);
	}
}

static void checkIfNextPair(StateInfo* stateInfo)
{
	if(stateInfo->globalGameInfo->period >= 4) {

		// this pair has used its turn when:
		// - player at the third base is no longer in the field and batter cant make run of honor
		// - batterIndex == -1 and ball is at home( and player can make no run of honor ). after three strikes this happens automatically.
		// - or if free walks have been used
		// in this situation runner is always at battingTeamOnFieldIndices[0] so we just have to check that.
		if((stateInfo->localGameInfo->gAI.ballHome == 1 && stateInfo->localGameInfo->pII.batterIndex == -1 &&
		        stateInfo->localGameInfo->gAI.canMakeRunOfHonor == 0) ||
		        (stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[0] == -1 &&
		         stateInfo->localGameInfo->gAI.canMakeRunOfHonor == 0) ||
		        stateInfo->localGameInfo->gAI.forceNextPair == 1) {
			if(nextPairCounter == -1) {
				nextPairCounter = 0;
				// send message only if its not end of inning also.
				if(endOfInningCounter == -1) {
					stateInfo->localGameInfo->gAI.gameInfoEvent = 8;
				}
			}
		}
		if(nextPairCounter != -1) {
			nextPairCounter++;
		}
		if(nextPairCounter > 200) {
			// set to -2 so that we avoid this being called twice. it will be set to -1 in the beginning of the next pair
			nextPairCounter = -2;
			stateInfo->localGameInfo->gAI.runnerBatterPairCounter++;
			// if equality holds, ending of inning will load the settings.
			if(stateInfo->localGameInfo->gAI.runnerBatterPairCounter != stateInfo->globalGameInfo->pairCount) {
				int pairsLeft = stateInfo->globalGameInfo->pairCount - stateInfo->localGameInfo->gAI.runnerBatterPairCounter;
				int battingTeamIndex = (stateInfo->globalGameInfo->
				                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
				int catchingTeamIndex = (battingTeamIndex+1)%2;
				int battingRuns = stateInfo->globalGameInfo->teams[battingTeamIndex].runs;
				int catchingRuns = stateInfo->globalGameInfo->teams[catchingTeamIndex].runs;
				// this will allow game to end if catching team has too many runs for batting team ever to catch up.
				if((stateInfo->globalGameInfo->inning+1)%2 == 0 && pairsLeft*2 + battingRuns < catchingRuns) {
					stateInfo->localGameInfo->gAI.endPeriod = 1;
				} else {
					loadMutableWorldSettings(stateInfo);
				}
			}
		}
	}
}
