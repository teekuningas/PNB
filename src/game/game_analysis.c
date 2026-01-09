/*
 * this module tries to tackle rule-related things.
 */

#include <stdio.h>
#include "globals.h"
#include "game_analysis.h"
#include "common_logic.h"
#include "menu_types.h"
#include "rules_outs.h"
#include "rules_runs.h"
#include "rules_strikes.h"
#include "base_logic.h"
#include "base_control.h"

#define BASE_RADIUS 2.0f
#define WOUNDING_CATCH_THRESHOLD (1.0f * (1 / (UPDATE_INTERVAL*1.0f/1000)))
#define OUT_OF_BOUNDS_THRESHOLD (2.0f * (1 / (UPDATE_INTERVAL*1.0f/1000)))

static void checkIfNextBatterDecision(StateInfo* stateInfo);
static void strikesAndBalls(StateInfo* stateInfo);
static void checkIfEndOfInning(StateInfo* stateInfo, MenuInfo* menuInfo, unsigned int* rng_seed);
static void woundingCatchEffects(StateInfo* stateInfo);
static void foulPlay(StateInfo* stateInfo, unsigned int* rng_seed);
static void checkIfNextPair(StateInfo* stateInfo, unsigned int* rng_seed);

static void populateGameConclusion(StateInfo* stateInfo, int winner)
{
	stateInfo->gameConclusion->winner = winner;
	stateInfo->gameConclusion->isCupGame = stateInfo->globalGameInfo->isCupGame;
	stateInfo->gameConclusion->period0Runs[0] = stateInfo->globalGameInfo->teams[0].period0Runs;
	stateInfo->gameConclusion->period0Runs[1] = stateInfo->globalGameInfo->teams[1].period0Runs;
	stateInfo->gameConclusion->period1Runs[0] = stateInfo->globalGameInfo->teams[0].period1Runs;
	stateInfo->gameConclusion->period1Runs[1] = stateInfo->globalGameInfo->teams[1].period1Runs;
	stateInfo->gameConclusion->period2Runs[0] = stateInfo->globalGameInfo->teams[0].period2Runs;
	stateInfo->gameConclusion->period2Runs[1] = stateInfo->globalGameInfo->teams[1].period2Runs;
	stateInfo->gameConclusion->period3Runs[0] = stateInfo->globalGameInfo->teams[0].period3Runs;
	stateInfo->gameConclusion->period3Runs[1] = stateInfo->globalGameInfo->teams[1].period3Runs;
}

void initGameAnalysis(GameFlowState* gameFlowState)
{
	// init some variables only used here.
	gameFlowState->outOfBoundsCounter = 0;
	gameFlowState->endOfInningCounter = -1;
	gameFlowState->nextPairCounter = -1;
	gameFlowState->foulPlayEventFlag = 0;
	gameFlowState->homeRunCameraCounter = -1;
}

void gameAnalysis(StateInfo* stateInfo, MenuInfo* menuInfo, unsigned int* rng_seed)
{
	// Reset per-frame flags
	stateInfo->localGameInfo->gameFlowState.ballHome = 0;

	// when player from third base starts running, we change camera view. when the situation is over we
	// wait 50 update frames, before moving to normal camera
	if(stateInfo->localGameInfo->gameFlowState.homeRunCameraCounter >= 0) {
		stateInfo->localGameInfo->gameFlowState.homeRunCameraCounter++;
		if(stateInfo->localGameInfo->gameFlowState.homeRunCameraCounter > 50) {
			stateInfo->localGameInfo->cameraState.homeRunCameraFlag = 0;
			stateInfo->localGameInfo->gameFlowState.homeRunCameraCounter = -1;
		}
	}

	checkIfNextBatterDecision(stateInfo);
	woundingCatchEffects(stateInfo);
	foulPlay(stateInfo, rng_seed);
	strikesAndBalls(stateInfo);
	checkIfEndOfInning(stateInfo, menuInfo, rng_seed);
	checkIfNextPair(stateInfo, rng_seed);

}

static void checkIfNextBatterDecision(StateInfo* stateInfo)
{
	// so this function's idea is to make progress in selecting a new batter if old one's gone.
	// so this will be called only once when possible.
	if(stateInfo->globalGameInfo->period >= 4) {

	} else if(stateInfo->localGameInfo->pII.batterIndex == -1 && stateInfo->localGameInfo->gameControl.waitingForBatterDecision == 0 &&
	          stateInfo->localGameInfo->gameFlowState.endOfInningCounter == -1) {
		// there have to be a player available
		if(stateInfo->localGameInfo->playerCounters.nonJokerPlayersLeft + stateInfo->localGameInfo->playerCounters.jokersLeft > 0) {
			// have to check that there is only three players in the field too and that it is not a out of bounds situation.
			if(count_active_batting_players(stateInfo->localGameInfo->playerInfo) < BASE_COUNT && stateInfo->localGameInfo->gameState.outOfBounds == 0) {
				// also we cannot know yet if it will be out of position situation so we have to wait that the ball will land
				// in some way.
				if(stateInfo->localGameInfo->ballInfo.hasHitGround == 1 || stateInfo->localGameInfo->gameControl.firstCatchMade == 1) {
					// if that happens we can now start.
					int battingTeamIndex = (stateInfo->globalGameInfo->
					                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
					// this will give work to action_invocatin.c and action_implementation.c
					stateInfo->localGameInfo->gameControl.waitingForBatterDecision = 1;
					// we just select the batterSelectionIndex here. if there are nonJokerPlayerLeft, we
					// just select the next batter in order there. if not, we select the first joker we find that is still unused.
					// one of these must be true, as we checked there is joker or non-joker left before.
					if(stateInfo->localGameInfo->playerCounters.nonJokerPlayersLeft != 0) {
						stateInfo->localGameInfo->pII.batterSelectionIndex =
						    stateInfo->globalGameInfo->teams[battingTeamIndex].batterOrder[stateInfo->globalGameInfo->teams[battingTeamIndex].batterOrderIndex];
					} else {
						int i;
						for(i = 0; i < JOKER_COUNT; i++) {
							if(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->pII.jokerIndices[i]].bTPI.joker == JOKER_AVAILABLE) {
								stateInfo->localGameInfo->pII.batterSelectionIndex =
								    stateInfo->localGameInfo->pII.jokerIndices[i];
								break;
							}
						}
					}
				}
			}
		} else {
			stateInfo->localGameInfo->playerCounters.noMorePlayers = 1;
		}
	}
}
// so here we are just updating strikes and balls related stuff. batter cant have more than 3 strikes, so something must be
// done to that, and if pitcher pitches balls, that isnt allowed without some compensation either.
static void strikesAndBalls(StateInfo* stateInfo)
{
	// so if there are three strikes
	if(should_change_batter_on_strikes(&(stateInfo->localGameInfo->gameState))) {
		// We restore automatic force running to resolve control ambiguity.
		// The batter is now "forced" to run by the rules.
		int index = get_base_controller(stateInfo->localGameInfo, BASE_HOME);
		if(index != -1) {
			runToNextBase(stateInfo->localGameInfo, stateInfo->fieldPositions, index, BASE_HOME);
			// Remove safety from home base
		}

		// Reset strikes so this doesn't re-trigger every frame
		stateInfo->localGameInfo->gameState.strikes = 0;
	}
	// we calculate the player and the base he has right to go freely only once, and that is when
	// the ball happens. if player moves to next base and user after that decides to make the free walk
	// that wont have any effect.
	if(stateInfo->localGameInfo->gameControl.freeWalkCalculationMade == 0) {
		if(count_active_batting_players(stateInfo->localGameInfo->playerInfo) == 1) {
			// if only one player on the field, thats the batter, and then free walks can be made after one pitch.
			if(stateInfo->localGameInfo->gameState.balls >= 1) {
				// calculate the index and the base.
				calculateFreeWalk(stateInfo->localGameInfo);
				// and tell action_implementation.c to take care of the rest.
				stateInfo->localGameInfo->gameControl.waitingForFreeWalkDecision = 1;

			}
		} else {
			// otherwise there is some non-batter leadrunner and he can have free walks after too balls.
			if(stateInfo->localGameInfo->gameState.balls >= 2) {
				// calculate the index and the base.
				calculateFreeWalk(stateInfo->localGameInfo);
				// and tell action_implementation.c to take care of the rest.
				stateInfo->localGameInfo->gameControl.waitingForFreeWalkDecision = 1;
			}
		}
		stateInfo->localGameInfo->gameControl.freeWalkCalculationMade = 1;
	} else {
		// so that if player just ran without taking hiw free walk, and got wounded or out, then stop asking
		// that question.
		if(stateInfo->localGameInfo->gameControl.waitingForFreeWalkDecision == 1) {
			if(stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->gameControl.freeWalkIndex].bTPI.state == PLAYER_STATE_WOUNDED ||
			        stateInfo->localGameInfo->playerInfo[stateInfo->localGameInfo->gameControl.freeWalkIndex].bTPI.state == PLAYER_STATE_OUT) {
				stateInfo->localGameInfo->gameControl.waitingForFreeWalkDecision = 0;
			}
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
	if(stateInfo->localGameInfo->referee.woundingCatchPending == 1 && stateInfo->localGameInfo->referee.woundingCatchHandled == 0) {
		int i;
		stateInfo->localGameInfo->referee.woundingCatchTimer = 0;
		stateInfo->localGameInfo->ballInfo.hitsGroundToUnWound = 0;
		stateInfo->localGameInfo->referee.woundingCatchHandled = 1;

		for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
			// so we check every batting team player.
			int index = i;
			if(stateInfo->localGameInfo->playerInfo[index].bTPI.baseId != BASE_NONE) {
				// if player is taking a free walk its always not wound. if not and ball is out of base,
				// its a wound, its also wound if the player has arrived the next base already.
				if(!player_is_safe_from_fly(stateInfo->localGameInfo->playerInfo[index].bTPI.state,
				                            stateInfo->localGameInfo->playerInfo[index].bTPI.baseId,
				                            stateInfo->localGameInfo->referee.battingPlayers[index].baseAtPitchStart)) {
					stateInfo->localGameInfo->referee.woundingPlayersMarked[index] = 1;
					// Milestone 12: Snapshot base at catch moment
					stateInfo->localGameInfo->referee.battingPlayers[index].woundingSourceBase = stateInfo->localGameInfo->playerInfo[index].bTPI.baseId;

				} else {
				}
			}
		}


	}
	if(stateInfo->localGameInfo->referee.woundingCatchTimer >= 0) {
		int threshold;
		stateInfo->localGameInfo->referee.woundingCatchTimer++;
		// and we extend the time a bit if ball is not with the player anymore.
		if(stateInfo->localGameInfo->pII.hasBallIndex == -1) {
			threshold = (int)(2*WOUNDING_CATCH_THRESHOLD);
		} else threshold = (int)WOUNDING_CATCH_THRESHOLD;
		// if unWounding happens, then stop the counter and continue the game normally.
		if(stateInfo->localGameInfo->ballInfo.hitsGroundToUnWound == 1) {
			int i;
			stateInfo->localGameInfo->referee.woundingCatchTimer = -1;
			stateInfo->localGameInfo->referee.woundingCatchPending = 0;
			for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
				stateInfo->localGameInfo->referee.woundingPlayersMarked[i] = 0;
			}
		}
		// otherwise there is a real possibility for wounding
		// and we check if there are players that are out of base etc at that moment.
		if(stateInfo->localGameInfo->referee.woundingCatchTimer > threshold) {
			int i;
			for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
				// so we check every batting team player.
				int index = i;
				if(stateInfo->localGameInfo->playerInfo[index].bTPI.baseId != BASE_NONE) {
					if(stateInfo->localGameInfo->referee.woundingPlayersMarked[index] == 1) {
						BaseID baseId = stateInfo->localGameInfo->playerInfo[index].bTPI.baseId;

						// info to screen.
						stateInfo->localGameInfo->gameState.event = EVENT_WOUNDED;

						// Referee Update (Milestone 12)
						stateInfo->localGameInfo->referee.battingPlayers[index].hasPendingWound = 1;
						// Don't overwrite if Referee already marked it as TUPLAHAAVA (during shared safety phase)
						if (stateInfo->localGameInfo->referee.battingPlayers[index].woundingType != WOUNDING_TYPE_TUPLAHAAVA) {
							stateInfo->localGameInfo->referee.battingPlayers[index].woundingType = WOUNDING_TYPE_NORMAL;
						}
						// woundingSourceBase was already snapshotted at catch moment
						stateInfo->localGameInfo->referee.battingPlayers[index].baseAtLastEvent = baseId;

						// Only remove safety if it's NOT a Tuplahaava (double wound).
						// Tuplahaava allows the rear runner to retain safety at the previous base until they reach safety.
						if (stateInfo->localGameInfo->referee.battingPlayers[index].woundingType != WOUNDING_TYPE_TUPLAHAAVA) {
							stateInfo->localGameInfo->referee.battingPlayers[index].currentSafetyBase = BASE_NONE;
						}

						// Make sure they're running toward next base to try to avoid OUT
						// If they were LEADING, this forces them to run
						// If they were already RUNNING, this ensures they keep running
						// runToNextBase(stateInfo->localGameInfo, stateInfo->fieldPositions, index, baseId);
						// User: Referee don't need to force the player to run at wound.
						// The player/AI layer should handle this "Panic Run".

						stateInfo->localGameInfo->referee.woundingPlayersMarked[index] = 0;

					}
				}
			}
			stateInfo->localGameInfo->referee.woundingCatchPending = 0;
			stateInfo->localGameInfo->referee.woundingCatchTimer = -1;
		}
	}
}
// so in case of foul play, we will stop the game
// return players to their original bases and start again with the screen of pitcher getting ball.
static void foulPlay(StateInfo* stateInfo, unsigned int* rng_seed)
{
	// so if outOfBounds == 1 which has been checked and set when ball lands in game_manipulation
	if(stateInfo->localGameInfo->gameState.outOfBounds == 1) {
		// we use a counter so that there is some time to realize what happened.
		stateInfo->localGameInfo->gameFlowState.outOfBoundsCounter += 1;
		// and send some info to screen. and do that only once.
		if(stateInfo->localGameInfo->gameFlowState.foulPlayEventFlag == 0) {
			stateInfo->localGameInfo->gameState.event = EVENT_OUT_OF_BOUNDS;
			stateInfo->localGameInfo->gameFlowState.foulPlayEventFlag = 1;
		}
		if(stateInfo->localGameInfo->gameFlowState.outOfBoundsCounter > OUT_OF_BOUNDS_THRESHOLD) {
			int j;
			stateInfo->localGameInfo->gameFlowState.outOfBoundsCounter = 0;
			stateInfo->localGameInfo->gameFlowState.foulPlayEventFlag = 0;
			stateInfo->localGameInfo->gameState.outOfBounds = 0;

			// so now initialize everything like in beginning of the inning except important non-volatile stuff
			// like runs in the inning and outs. we cannot initialize baseAtPitchStart snapshots either as players' bases
			// have to bet set to their baseAtPitchStart here later on.
			initializeBallInfo(stateInfo->localGameInfo);
			initializeActionInfo(stateInfo->localGameInfo);
			initializeTemporaryGameAnalysisInfo(stateInfo->localGameInfo);

			initializeIndexInformation(stateInfo->localGameInfo);
			initializePRAIInformation(stateInfo->localGameInfo);
			initializeSpatialPlayerInformation(stateInfo->localGameInfo, stateInfo->fieldPositions, rng_seed);

			initializeNonCriticalPlayerInformation(stateInfo->localGameInfo);

			if(stateInfo->globalGameInfo->period >= 4) {
				// when running through homerun-batting contest, we have to
				// do a bit special initialization as our setup in setRunnerAndBatter()
				// depends on the field being empty.
				initializeCriticalBattingTeamInformation(stateInfo->localGameInfo);
				setRunnerAndBatter(stateInfo->localGameInfo, stateInfo->globalGameInfo, stateInfo->fieldPositions);
			} else {
				// every players' locations etc got initialized just now. so at the moment our batting team players
				// will all be around home base. But we dont want all players to be there
				// so we have to do some modifications accoding to baseAtPitchStart snapshots.
				for(j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
					int index = j;
					// Referee Restore (Milestone 12)
					// check if this player was active on field (had a valid baseAtPitchStart)
					if(stateInfo->localGameInfo->referee.battingPlayers[index].baseAtPitchStart != BASE_NONE) {
						// so we set every batting team player 's, who was on the field, bases to baseAtPitchStart
						// and set them to be at a base.
						stateInfo->localGameInfo->playerInfo[index].bTPI.state = PLAYER_STATE_ON_BASE;
						BaseID restoreBase = stateInfo->localGameInfo->referee.battingPlayers[index].baseAtPitchStart;
						stateInfo->localGameInfo->playerInfo[index].bTPI.baseId = restoreBase;

						// Restore Safety Status from Referee
						if(stateInfo->localGameInfo->referee.battingPlayers[index].hadSafetyAtPitchStart) {
							stateInfo->localGameInfo->referee.battingPlayers[index].currentSafetyBase = restoreBase;
						} else {
							stateInfo->localGameInfo->referee.battingPlayers[index].currentSafetyBase = BASE_NONE;
						}

						// Clear Referee Temporary States
						stateInfo->localGameInfo->referee.battingPlayers[index].hasPendingWound = 0;
						stateInfo->localGameInfo->referee.battingPlayers[index].woundingType = WOUNDING_TYPE_NONE;

						// in case that player was taking a free walk from third base when this happened
						if(stateInfo->localGameInfo->playerInfo[index].bTPI.baseId == BASE_HOME_SCORED) {
							int battingTeamIndex = (stateInfo->globalGameInfo->
							                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
							// we will get a run.
							stateInfo->globalGameInfo->teams[battingTeamIndex].runs += 1;
							stateInfo->localGameInfo->gameState.runsInTheInning += 1;
							// and send a message that run was made to screen.
							stateInfo->localGameInfo->gameState.event = EVENT_RUN_SCORED;
							// and remove player from the field.
							stateInfo->localGameInfo->playerInfo[index].bTPI.baseId = BASE_NONE;
							// always when two runs is got, we will get a new round of batters.
							if(stateInfo->localGameInfo->gameState.runsInTheInning%2 == 0) {
								stateInfo->localGameInfo->playerCounters.nonJokerPlayersLeft = PLAYERS_IN_TEAM;
							}
						}
						// if this player is a batter
						else if(stateInfo->localGameInfo->playerInfo[index].bTPI.baseId == BASE_HOME) {
							// Check if this foul results in a 3rd strike using the snapshot from pitch start.
							// This avoids the race condition where strikesAndBalls might have already reset the counter.
							if(stateInfo->localGameInfo->referee.strikesAtPitchStart == 2) {
								// It was 2 strikes, this foul is the 3rd -> OUT.
								stateInfo->localGameInfo->gameState.strikes = 3; // Ensure it's visually 3 (though about to be reset by out)
								stateInfo->localGameInfo->gameState.outs += 1;
								// remove from the field.
								stateInfo->localGameInfo->playerInfo[index].bTPI.baseId = BASE_NONE;
								// Player is OUT, so they lose safety rights to the base.
								stateInfo->localGameInfo->referee.battingPlayers[index].currentSafetyBase = BASE_NONE;
								// Clear pitch start snapshot so they aren't resurrected by future foul plays
								stateInfo->localGameInfo->referee.battingPlayers[index].baseAtPitchStart = BASE_NONE;

								// new batter needed.
								stateInfo->localGameInfo->pII.batterIndex = -1;
							} else {
								// otherwise, this player will continue batting.
								// Restore strikes to (start + 1) because this foul counts as a strike.
								stateInfo->localGameInfo->gameState.strikes = stateInfo->localGameInfo->referee.strikesAtPitchStart + 1;

								stateInfo->localGameInfo->pII.batterIndex = index;
								// preparing left to function that will do it just fine.
								prepareBatter(stateInfo->localGameInfo);
							}
						}
						// other bases straightforwardly.
						else if(stateInfo->localGameInfo->playerInfo[index].bTPI.baseId == BASE_FIRST) {
							stateInfo->localGameInfo->playerInfo[index].tPI.location.x =
							    stateInfo->fieldPositions->firstBaseRun.x;
							stateInfo->localGameInfo->playerInfo[index].tPI.location.z =
							    stateInfo->fieldPositions->firstBaseRun.z;
						} else if(stateInfo->localGameInfo->playerInfo[index].bTPI.baseId == BASE_SECOND) {
							stateInfo->localGameInfo->playerInfo[index].tPI.location.x =
							    stateInfo->fieldPositions->secondBaseRun.x;
							stateInfo->localGameInfo->playerInfo[index].tPI.location.z =
							    stateInfo->fieldPositions->secondBaseRun.z;
						} else if(stateInfo->localGameInfo->playerInfo[index].bTPI.baseId == BASE_THIRD) {
							stateInfo->localGameInfo->playerInfo[index].tPI.location.x =
							    stateInfo->fieldPositions->thirdBaseRun.x;
							stateInfo->localGameInfo->playerInfo[index].tPI.location.z =
							    stateInfo->fieldPositions->thirdBaseRun.z;
						}
					}
				}
			}
		}
	}
}

static void checkIfEndOfInning(StateInfo* stateInfo, MenuInfo* menuInfo, unsigned int* rng_seed)
{
	// if three outs or
	// no more players to bat. set flag on the player selection to indicate that no more players left.
	// then if ball comes to pitcher, then we can quit this inning.
	if(stateInfo->localGameInfo->gameState.outs >= 3 || (stateInfo->localGameInfo->playerCounters.noMorePlayers == 1 &&
	        stateInfo->localGameInfo->gameFlowState.ballHome == 1) || stateInfo->localGameInfo->gameState.endPeriod == 1 ||
	        (stateInfo->globalGameInfo->period >= 4 && stateInfo->localGameInfo->gameModeState.runnerBatterPairCounter >=
	         stateInfo->globalGameInfo->pairCount)) {

		if(stateInfo->localGameInfo->gameFlowState.endOfInningCounter == -1) {
			stateInfo->localGameInfo->gameFlowState.endOfInningCounter = 0;
			// so that user wont be prompted for this after inning has ended but screen hasnt changed yet.
			stateInfo->localGameInfo->gameControl.waitingForBatterDecision = 0;
			stateInfo->localGameInfo->gameState.event = EVENT_INNING_ENDING;
		}
	}
	if(stateInfo->localGameInfo->gameFlowState.endOfInningCounter != -1) {
		stateInfo->localGameInfo->gameFlowState.endOfInningCounter++;
	}
	// basically here we just list the different kind of ending alternatives and figure out if this is one of them.
	if(stateInfo->localGameInfo->gameFlowState.endOfInningCounter > 200) {
		int battingTeamIndex = (stateInfo->globalGameInfo->
		                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
		int catchingTeamIndex = (battingTeamIndex+1)%2;

		stateInfo->localGameInfo->gameFlowState.endOfInningCounter = -1;
		stateInfo->globalGameInfo->inning++;
		// if first period ending
		if(stateInfo->globalGameInfo->inning == stateInfo->globalGameInfo->halfInningsInPeriod ||
		        (stateInfo->globalGameInfo->inning == stateInfo->globalGameInfo->halfInningsInPeriod - 1 &&
		         stateInfo->globalGameInfo->teams[catchingTeamIndex].runs >
		         stateInfo->globalGameInfo->teams[battingTeamIndex].runs)) {
			int i;
			stateInfo->globalGameInfo->period = 1;
			for(i = 0; i < 2; i++) {
				stateInfo->globalGameInfo->teams[i].period0Runs = stateInfo->globalGameInfo->teams[i].runs;
				stateInfo->globalGameInfo->teams[i].runs = 0;
			}
			if(stateInfo->globalGameInfo->inning == stateInfo->globalGameInfo->halfInningsInPeriod - 1) {
				stateInfo->globalGameInfo->inning++; // have to skip the last half-inning
			}
			menuInfo->mode = MENU_ENTRY_INTER_PERIOD;
			stateInfo->screen = SCREEN_MAIN_MENU;
			stateInfo->changeScreen = 1;
			stateInfo->updated = 0;
		}
		// if second period ending
		else if(stateInfo->globalGameInfo->inning == stateInfo->globalGameInfo->halfInningsInPeriod*2 ||
		        (stateInfo->globalGameInfo->inning == stateInfo->globalGameInfo->halfInningsInPeriod*2 - 1 &&
		         (stateInfo->globalGameInfo->teams[catchingTeamIndex].runs >
		          stateInfo->globalGameInfo->teams[battingTeamIndex].runs || (stateInfo->globalGameInfo->teams[catchingTeamIndex].period0Runs >
		                  stateInfo->globalGameInfo->teams[battingTeamIndex].period0Runs && stateInfo->globalGameInfo->teams[catchingTeamIndex].runs ==
		                  stateInfo->globalGameInfo->teams[battingTeamIndex].runs )))) {
			int i;
			for(i = 0; i < 2; i++) {
				stateInfo->globalGameInfo->teams[i].period1Runs = stateInfo->globalGameInfo->teams[i].runs;
			}

			int team0period0runs = stateInfo->globalGameInfo->teams[0].period0Runs;
			int team0period1runs = stateInfo->globalGameInfo->teams[0].period1Runs;
			int team1period0runs = stateInfo->globalGameInfo->teams[1].period0Runs;
			int team1period1runs = stateInfo->globalGameInfo->teams[1].period1Runs;
			// is the game over already?
			if( team0period0runs>=team1period0runs && team0period1runs>=team1period1runs &&
			        (team0period0runs != team1period0runs || team0period1runs != team1period1runs)) {
				int winner = 0;
				populateGameConclusion(stateInfo, winner);
				menuInfo->mode = MENU_ENTRY_GAME_OVER;
			} else if( team0period0runs<=team1period0runs && team0period1runs<=team1period1runs &&
			           (team0period0runs != team1period0runs || team0period1runs != team1period1runs)) {
				int winner = 1;
				populateGameConclusion(stateInfo, winner);
				menuInfo->mode = MENU_ENTRY_GAME_OVER;
			} else {
				stateInfo->globalGameInfo->period = 2;
				menuInfo->mode = MENU_ENTRY_SUPER_INNING;
			}
			if(stateInfo->globalGameInfo->inning == stateInfo->globalGameInfo->halfInningsInPeriod*2 - 1) {
				stateInfo->globalGameInfo->inning++; // have to skip the last half-inning
			}
			for(i = 0; i < 2; i++) {
				stateInfo->globalGameInfo->teams[i].runs = 0;
			}
			stateInfo->screen = SCREEN_MAIN_MENU;
			stateInfo->changeScreen = 1;
			stateInfo->updated = 0;
		}
		// if super inning ending
		else if(stateInfo->globalGameInfo->inning == stateInfo->globalGameInfo->halfInningsInPeriod*2 + 2) {
			int i;
			for(i = 0; i < 2; i++) {
				stateInfo->globalGameInfo->teams[i].period2Runs = stateInfo->globalGameInfo->teams[i].runs;
			}
			// is the game over already?
			if(stateInfo->globalGameInfo->teams[0].runs > stateInfo->globalGameInfo->teams[1].runs) {
				int winner = 0;
				populateGameConclusion(stateInfo, winner);
				menuInfo->mode = MENU_ENTRY_GAME_OVER;
			} else if(stateInfo->globalGameInfo->teams[0].runs < stateInfo->globalGameInfo->teams[1].runs) {
				int winner = 1;
				populateGameConclusion(stateInfo, winner);
				menuInfo->mode = MENU_ENTRY_GAME_OVER;
			}
			// if not, we move to homerun-batting contest
			else {
				stateInfo->globalGameInfo->period = 4;
				menuInfo->mode = MENU_ENTRY_HOMERUN_CONTEST;
			}

			for(i = 0; i < 2; i++) {
				stateInfo->globalGameInfo->teams[i].runs = 0;
			}

			stateInfo->screen = SCREEN_MAIN_MENU;
			stateInfo->changeScreen = 1;
			stateInfo->updated = 0;
		}
		// is homerun-batting contest moving to next stage or ending
		else if(stateInfo->globalGameInfo->period >= 4 && (stateInfo->globalGameInfo->inning)%2 == 0) {
			int i;
			for(i = 0; i < 2; i++) {
				stateInfo->globalGameInfo->teams[i].period3Runs += stateInfo->globalGameInfo->teams[i].runs;
			}
			// is the game over already?
			if(stateInfo->globalGameInfo->teams[0].period3Runs > stateInfo->globalGameInfo->teams[1].period3Runs) {
				int winner = 0;
				populateGameConclusion(stateInfo, winner);
				menuInfo->mode = MENU_ENTRY_GAME_OVER;
			} else if(stateInfo->globalGameInfo->teams[0].period3Runs < stateInfo->globalGameInfo->teams[1].period3Runs) {
				int winner = 1;
				populateGameConclusion(stateInfo, winner);
				menuInfo->mode = MENU_ENTRY_GAME_OVER;
			} else {
				// +=2 because we want to use 4, 6, 8... for homerun batting contest periods
				// as we dont want to mess the team ordering when
				// calculating those battingTeamIndices.
				stateInfo->globalGameInfo->period+=2;
				menuInfo->mode = MENU_ENTRY_HOMERUN_CONTEST;
			}

			for(i = 0; i < 2; i++) {
				stateInfo->globalGameInfo->teams[i].runs = 0;
			}

			stateInfo->screen = SCREEN_MAIN_MENU;
			stateInfo->changeScreen = 1;
			stateInfo->updated = 0;
		}
		if(stateInfo->screen != SCREEN_MAIN_MENU) loadMutableWorldSettings(stateInfo, rng_seed);
	}
}

static void checkIfNextPair(StateInfo* stateInfo, unsigned int* rng_seed)
{
	if(stateInfo->globalGameInfo->period >= 4) {

		// this pair has used its turn when:
		// - player at the third base is no longer in the field and batter cant make run of honor
		// - batterIndex == -1 and ball is at home( and player can make no run of honor ). after three strikes this happens automatically.
		// - or if free walks have been used
		// in this situation runner is always at battingTeamOnFieldIndices[0] so we just have to check that.
		int runnerAtThirdIndex = get_base_controller(stateInfo->localGameInfo, BASE_THIRD);
		if((stateInfo->localGameInfo->gameFlowState.ballHome == 1 && stateInfo->localGameInfo->pII.batterIndex == -1 &&
		        stateInfo->localGameInfo->gameModeState.canMakeRunOfHonor == 0) ||
		        (runnerAtThirdIndex == -1 &&
		         stateInfo->localGameInfo->gameModeState.canMakeRunOfHonor == 0) ||
		        stateInfo->localGameInfo->gameModeState.forceNextPair == 1) {
			if(stateInfo->localGameInfo->gameFlowState.nextPairCounter == -1) {
				stateInfo->localGameInfo->gameFlowState.nextPairCounter = 0;
				// send message only if its not end of inning also.
				if(stateInfo->localGameInfo->gameFlowState.endOfInningCounter == -1) {
					stateInfo->localGameInfo->gameState.event = EVENT_NEXT_PAIR;
				}
			}
		}
		if(stateInfo->localGameInfo->gameFlowState.nextPairCounter != -1) {
			stateInfo->localGameInfo->gameFlowState.nextPairCounter++;
		}
		if(stateInfo->localGameInfo->gameFlowState.nextPairCounter > 200) {
			// set to -2 so that we avoid this being called twice. it will be set to -1 in the beginning of the next pair
			stateInfo->localGameInfo->gameFlowState.nextPairCounter = -2;
			stateInfo->localGameInfo->gameModeState.runnerBatterPairCounter++;
			// if equality holds, ending of inning will load the settings.
			if(stateInfo->localGameInfo->gameModeState.runnerBatterPairCounter != stateInfo->globalGameInfo->pairCount) {
				int pairsLeft = stateInfo->globalGameInfo->pairCount - stateInfo->localGameInfo->gameModeState.runnerBatterPairCounter;
				int battingTeamIndex = (stateInfo->globalGameInfo->
				                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
				int catchingTeamIndex = (battingTeamIndex+1)%2;
				int battingRuns = stateInfo->globalGameInfo->teams[battingTeamIndex].runs;
				int catchingRuns = stateInfo->globalGameInfo->teams[catchingTeamIndex].runs;
				// this will allow game to end if catching team has too many runs for batting team ever to catch up.
				if((stateInfo->globalGameInfo->inning+1)%2 == 0 && pairsLeft*2 + battingRuns < catchingRuns) {
					stateInfo->localGameInfo->gameState.endPeriod = 1;
				} else {
					loadMutableWorldSettings(stateInfo, rng_seed);
				}
			}
		}
	}
}
