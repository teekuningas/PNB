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
#include "rules_pure/player_utils.h"

#define BASE_RADIUS 2.0f

static void checkIfNextBatterDecision(StateInfo* stateInfo);
static void strikesAndBalls(StateInfo* stateInfo);
static void checkIfEndOfInning(StateInfo* stateInfo, MenuInfo* menuInfo, unsigned int* rng_seed);
static void checkIfNextPair(StateInfo* stateInfo, unsigned int* rng_seed);

static void populateGameConclusion(StateInfo* stateInfo, int winner)
{
	stateInfo->gameConclusion->winner = winner;
	stateInfo->gameConclusion->isCupGame = stateInfo->match->scoreboard.isCupGame;
	stateInfo->gameConclusion->period0Runs[0] = stateInfo->match->scoreboard.teams[0].period0Runs;
	stateInfo->gameConclusion->period0Runs[1] = stateInfo->match->scoreboard.teams[1].period0Runs;
	stateInfo->gameConclusion->period1Runs[0] = stateInfo->match->scoreboard.teams[0].period1Runs;
	stateInfo->gameConclusion->period1Runs[1] = stateInfo->match->scoreboard.teams[1].period1Runs;
	stateInfo->gameConclusion->period2Runs[0] = stateInfo->match->scoreboard.teams[0].period2Runs;
	stateInfo->gameConclusion->period2Runs[1] = stateInfo->match->scoreboard.teams[1].period2Runs;
	stateInfo->gameConclusion->period3Runs[0] = stateInfo->match->scoreboard.teams[0].period3Runs;
	stateInfo->gameConclusion->period3Runs[1] = stateInfo->match->scoreboard.teams[1].period3Runs;
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
	// Reset per-frame flags removed (ballHome handled in game_manipulation.c)

	// when player from third base starts running, we change camera view. when the situation is over we
	// wait 50 update frames, before moving to normal camera
	if(stateInfo->match->gameFlowState.homeRunCameraCounter >= 0) {
		stateInfo->match->gameFlowState.homeRunCameraCounter++;
		if(stateInfo->match->gameFlowState.homeRunCameraCounter > 50) {
			stateInfo->match->cameraState.homeRunCameraFlag = 0;
			stateInfo->match->gameFlowState.homeRunCameraCounter = -1;
		}
	}

	checkIfNextBatterDecision(stateInfo);
	strikesAndBalls(stateInfo);
	checkIfEndOfInning(stateInfo, menuInfo, rng_seed);
	checkIfNextPair(stateInfo, rng_seed);

}

static void checkIfNextBatterDecision(StateInfo* stateInfo)
{
	// so this function's idea is to make progress in selecting a new batter if old one's gone.
	// so this will be called only once when possible.
	if(stateInfo->match->scoreboard.period >= 4) {

	} else if(get_active_batter_index(stateInfo->match) == -1 &&stateInfo->match->flowControl.waitingForBatterDecision == 0 &&
	          stateInfo->match->gameFlowState.endOfInningCounter == -1) {
		// there have to be a player available
		if(stateInfo->match->playerCounters.nonJokerPlayersLeft + stateInfo->match->playerCounters.jokersLeft > 0) {
			// have to check that there is only three players in the field too and that it is not a out of bounds situation.
			if(count_active_batting_players(stateInfo->match->playerInfo) < BASE_COUNT &&stateInfo->match->betweenPitchState.outOfBounds == 0) {
				// also we cannot know yet if it will be out of position situation so we have to wait that the ball will land
				// in some way.
				if(stateInfo->match->betweenPitchState.hasBallHitGround == 1 || stateInfo->match->betweenPitchState.catchHasBeenMade == 1) {
					// if that happens we can now start.
					int battingTeamIndex = (stateInfo->match->scoreboard.					                        inning+stateInfo->match->scoreboard.playsFirst+stateInfo->match->scoreboard.period)%2;
					// this will give work to action_invocatin.c and action_implementation.c
					stateInfo->match->flowControl.waitingForBatterDecision = 1;
					// we just select the batterSelectionIndex here. if there are nonJokerPlayerLeft, we
					// just select the next batter in order there. if not, we select the first joker we find that is still unused.
					// one of these must be true, as we checked there is joker or non-joker left before.
					if(stateInfo->match->playerCounters.nonJokerPlayersLeft != 0) {
						stateInfo->match->pII.batterSelectionIndex =
						    stateInfo->match->scoreboard.teams[battingTeamIndex].batterOrder[stateInfo->match->scoreboard.teams[battingTeamIndex].batterOrderIndex];
					} else {
						int i;
						for(i = 0; i < JOKER_COUNT; i++) {
							if(stateInfo->match->playerInfo[stateInfo->match->pII.jokerIndices[i]].bTPI.joker == JOKER_AVAILABLE) {
								stateInfo->match->pII.batterSelectionIndex =
								    stateInfo->match->pII.jokerIndices[i];
								break;
							}
						}
					}
				}
			}
		} else {
			stateInfo->match->playerCounters.noMorePlayers = 1;
		}
	}
}
// so here we are just updating strikes and balls related stuff. batter cant have more than 3 strikes, so something must be
// done to that, and if pitcher pitches balls, that isnt allowed without some compensation either.
static void strikesAndBalls(StateInfo* stateInfo)
{
	// so if there are three strikes
	if(stateInfo->match->halfInningState.strikes >= 3) {
		// We restore automatic force running to resolve control ambiguity.
		// The batter is now "forced" to run by the rules.
		int index = get_base_controller(stateInfo->match, BASE_HOME);

		// Only force run if player is still there and NOT already running.
		// This prevents re-triggering every frame while preserving the 3 strikes state
		// until the next batter resets it.
		if(index != -1 &&stateInfo->match->playerInfo[index].bTPI.state != PLAYER_STATE_RUNNING) {
			runToNextBase(stateInfo->match, stateInfo->fieldPositions, index, BASE_HOME);
			// Remove safety from home base handled by Referee update_safety_status eventually
			// or explicitly here if needed, but runToNextBase sets state to RUNNING which
			// allows reconciliation logic to work.
		}
	}
	// we calculate the player and the base he has right to go freely only once, and that is when
	// the ball happens. if player moves to next base and user after that decides to make the free walk
	// that wont have any effect.
	if(stateInfo->match->flowControl.freeWalkCalculationMade == 0) {
		if(count_active_batting_players(stateInfo->match->playerInfo) == 1) {
			// if only one player on the field, thats the batter, and then free walks can be made after one pitch.
			if(stateInfo->match->halfInningState.balls >= 1) {
				// calculate the index and the base.
				calculateFreeWalk(stateInfo->match);
				// and tell action_implementation.c to take care of the rest.
				stateInfo->match->flowControl.waitingForFreeWalkDecision = 1;

			}
		} else {
			// otherwise there is some non-batter leadrunner and he can have free walks after too balls.
			if(stateInfo->match->halfInningState.balls >= 2) {
				// calculate the index and the base.
				calculateFreeWalk(stateInfo->match);
				// and tell action_implementation.c to take care of the rest.
				stateInfo->match->flowControl.waitingForFreeWalkDecision = 1;
			}
		}
		stateInfo->match->flowControl.freeWalkCalculationMade = 1;
	} else {
		// so that if player just ran without taking hiw free walk, and got wounded or out, then stop asking
		// that question.
		if(stateInfo->match->flowControl.waitingForFreeWalkDecision == 1) {
			if(stateInfo->match->playerInfo[stateInfo->match->flowControl.freeWalkIndex].bTPI.state == PLAYER_STATE_WOUNDED ||
			        stateInfo->match->playerInfo[stateInfo->match->flowControl.freeWalkIndex].bTPI.state == PLAYER_STATE_OUT) {
				stateInfo->match->flowControl.waitingForFreeWalkDecision = 0;
			}
		}
	}
}


static void checkIfEndOfInning(StateInfo* stateInfo, MenuInfo* menuInfo, unsigned int* rng_seed)
{
	// if three outs or
	// no more players to bat. set flag on the player selection to indicate that no more players left.
	// then if ball comes to pitcher, then we can quit this inning.
	if(stateInfo->match->halfInningState.outs >= 3 || (stateInfo->match->playerCounters.noMorePlayers == 1 &&
	        stateInfo->match->gameFlowState.ballHome == 1) || stateInfo->match->halfInningState.endPeriod == 1 ||
	        (stateInfo->match->scoreboard.period >= 4 &&stateInfo->match->homeRunContestState.runnerBatterPairCounter >=
	         stateInfo->match->scoreboard.pairCount)) {

		if(stateInfo->match->gameFlowState.endOfInningCounter == -1) {
			stateInfo->match->gameFlowState.endOfInningCounter = 0;
			// so that user wont be prompted for this after inning has ended but screen hasnt changed yet.
			stateInfo->match->flowControl.waitingForBatterDecision = 0;
			stateInfo->match->halfInningState.event = EVENT_INNING_ENDING;
		}
	}
	if(stateInfo->match->gameFlowState.endOfInningCounter != -1) {
		stateInfo->match->gameFlowState.endOfInningCounter++;
	}
	// basically here we just list the different kind of ending alternatives and figure out if this is one of them.
	if(stateInfo->match->gameFlowState.endOfInningCounter > 200) {
		int battingTeamIndex = (stateInfo->match->scoreboard.
		                        inning+stateInfo->match->scoreboard.playsFirst+stateInfo->match->scoreboard.period)%2;
		int catchingTeamIndex = (battingTeamIndex+1)%2;

		stateInfo->match->gameFlowState.endOfInningCounter = -1;
		stateInfo->match->scoreboard.inning++;
		// if first period ending
		if(stateInfo->match->scoreboard.inning == stateInfo->match->scoreboard.halfInningsInPeriod ||
		        (stateInfo->match->scoreboard.inning == stateInfo->match->scoreboard.halfInningsInPeriod - 1 &&
		         stateInfo->match->scoreboard.teams[catchingTeamIndex].runs >
		         stateInfo->match->scoreboard.teams[battingTeamIndex].runs)) {
			int i;
			stateInfo->match->scoreboard.period = 1;
			for(i = 0; i < 2; i++) {
				stateInfo->match->scoreboard.teams[i].period0Runs = stateInfo->match->scoreboard.teams[i].runs;
				stateInfo->match->scoreboard.teams[i].runs = 0;
			}
			if(stateInfo->match->scoreboard.inning == stateInfo->match->scoreboard.halfInningsInPeriod - 1) {
				stateInfo->match->scoreboard.inning++; // have to skip the last half-inning
			}
			menuInfo->mode = MENU_ENTRY_INTER_PERIOD;
			stateInfo->screen = SCREEN_MAIN_MENU;
			stateInfo->changeScreen = 1;
			stateInfo->updated = 0;
		}
		// if second period ending
		else if(stateInfo->match->scoreboard.inning == stateInfo->match->scoreboard.halfInningsInPeriod*2 ||
		        (stateInfo->match->scoreboard.inning == stateInfo->match->scoreboard.halfInningsInPeriod*2 - 1 &&
		         (stateInfo->match->scoreboard.teams[catchingTeamIndex].runs >
		          stateInfo->match->scoreboard.teams[battingTeamIndex].runs || (stateInfo->match->scoreboard.teams[catchingTeamIndex].period0Runs >
		                  stateInfo->match->scoreboard.teams[battingTeamIndex].period0Runs &&stateInfo->match->scoreboard.teams[catchingTeamIndex].runs ==
		                  stateInfo->match->scoreboard.teams[battingTeamIndex].runs )))) {
			int i;
			for(i = 0; i < 2; i++) {
				stateInfo->match->scoreboard.teams[i].period1Runs = stateInfo->match->scoreboard.teams[i].runs;
			}

			int team0period0runs = stateInfo->match->scoreboard.teams[0].period0Runs;
			int team0period1runs = stateInfo->match->scoreboard.teams[0].period1Runs;
			int team1period0runs = stateInfo->match->scoreboard.teams[1].period0Runs;
			int team1period1runs = stateInfo->match->scoreboard.teams[1].period1Runs;
			// is the game over already?
			if( team0period0runs>=team1period0runs &&team0period1runs>=team1period1runs &&
			        (team0period0runs != team1period0runs || team0period1runs != team1period1runs)) {
				int winner = 0;
				populateGameConclusion(stateInfo, winner);
				menuInfo->mode = MENU_ENTRY_GAME_OVER;
			} else if( team0period0runs<=team1period0runs &&team0period1runs<=team1period1runs &&
			           (team0period0runs != team1period0runs || team0period1runs != team1period1runs)) {
				int winner = 1;
				populateGameConclusion(stateInfo, winner);
				menuInfo->mode = MENU_ENTRY_GAME_OVER;
			} else {
				stateInfo->match->scoreboard.period = 2;
				menuInfo->mode = MENU_ENTRY_SUPER_INNING;
			}
			if(stateInfo->match->scoreboard.inning == stateInfo->match->scoreboard.halfInningsInPeriod*2 - 1) {
				stateInfo->match->scoreboard.inning++; // have to skip the last half-inning
			}
			for(i = 0; i < 2; i++) {
				stateInfo->match->scoreboard.teams[i].runs = 0;
			}
			stateInfo->screen = SCREEN_MAIN_MENU;
			stateInfo->changeScreen = 1;
			stateInfo->updated = 0;
		}
		// if super inning ending
		else if(stateInfo->match->scoreboard.inning == stateInfo->match->scoreboard.halfInningsInPeriod*2 + 2) {
			int i;
			for(i = 0; i < 2; i++) {
				stateInfo->match->scoreboard.teams[i].period2Runs = stateInfo->match->scoreboard.teams[i].runs;
			}
			// is the game over already?
			if(stateInfo->match->scoreboard.teams[0].runs > stateInfo->match->scoreboard.teams[1].runs) {
				int winner = 0;
				populateGameConclusion(stateInfo, winner);
				menuInfo->mode = MENU_ENTRY_GAME_OVER;
			} else if(stateInfo->match->scoreboard.teams[0].runs < stateInfo->match->scoreboard.teams[1].runs) {
				int winner = 1;
				populateGameConclusion(stateInfo, winner);
				menuInfo->mode = MENU_ENTRY_GAME_OVER;
			}
			// if not, we move to homerun-batting contest
			else {
				stateInfo->match->scoreboard.period = 4;
				menuInfo->mode = MENU_ENTRY_HOMERUN_CONTEST;
			}

			for(i = 0; i < 2; i++) {
				stateInfo->match->scoreboard.teams[i].runs = 0;
			}

			stateInfo->screen = SCREEN_MAIN_MENU;
			stateInfo->changeScreen = 1;
			stateInfo->updated = 0;
		}
		// is homerun-batting contest moving to next stage or ending
		else if(stateInfo->match->scoreboard.period >= 4 &&(stateInfo->match->scoreboard.inning)%2 == 0) {
			int i;
			for(i = 0; i < 2; i++) {
				stateInfo->match->scoreboard.teams[i].period3Runs += stateInfo->match->scoreboard.teams[i].runs;
			}
			// is the game over already?
			if(stateInfo->match->scoreboard.teams[0].period3Runs > stateInfo->match->scoreboard.teams[1].period3Runs) {
				int winner = 0;
				populateGameConclusion(stateInfo, winner);
				menuInfo->mode = MENU_ENTRY_GAME_OVER;
			} else if(stateInfo->match->scoreboard.teams[0].period3Runs < stateInfo->match->scoreboard.teams[1].period3Runs) {
				int winner = 1;
				populateGameConclusion(stateInfo, winner);
				menuInfo->mode = MENU_ENTRY_GAME_OVER;
			} else {
				// +=2 because we want to use 4, 6, 8... for homerun batting contest periods
				// as we dont want to mess the team ordering when
				// calculating those battingTeamIndices.
				stateInfo->match->scoreboard.period+=2;
				menuInfo->mode = MENU_ENTRY_HOMERUN_CONTEST;
			}

			for(i = 0; i < 2; i++) {
				stateInfo->match->scoreboard.teams[i].runs = 0;
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
	if(stateInfo->match->scoreboard.period >= 4) {

		// this pair has used its turn when:
		// - player at the third base is no longer in the field and batter cant make run of honor
		// - batterIndex == -1 and ball is at home( and player can make no run of honor ). after three strikes this happens automatically.
		// - or if free walks have been used
		// in this situation runner is always at battingTeamOnFieldIndices[0] so we just have to check that.
		int runnerAtThirdIndex = get_base_controller(stateInfo->match, BASE_THIRD);

		int runOfHonorPossible = is_run_of_honor_possible(stateInfo->match);

		if((stateInfo->match->gameFlowState.ballHome == 1 &&get_active_batter_index(stateInfo->match) == -1 &&
		        runOfHonorPossible == 0) ||
		        (runnerAtThirdIndex == -1 &&
		         runOfHonorPossible == 0) ||
		        stateInfo->match->homeRunContestState.forceNextPair == 1) {
			if(stateInfo->match->gameFlowState.nextPairCounter == -1) {
				stateInfo->match->gameFlowState.nextPairCounter = 0;
				// send message only if its not end of inning also.
				if(stateInfo->match->gameFlowState.endOfInningCounter == -1) {
					stateInfo->match->halfInningState.event = EVENT_NEXT_PAIR;
				}
			}
		}
		if(stateInfo->match->gameFlowState.nextPairCounter != -1) {
			stateInfo->match->gameFlowState.nextPairCounter++;
		}
		if(stateInfo->match->gameFlowState.nextPairCounter > 200) {
			// set to -2 so that we avoid this being called twice. it will be set to -1 in the beginning of the next pair
			stateInfo->match->gameFlowState.nextPairCounter = -2;
			stateInfo->match->homeRunContestState.runnerBatterPairCounter++;
			// if equality holds, ending of inning will load the settings.
			if(stateInfo->match->homeRunContestState.runnerBatterPairCounter != stateInfo->match->scoreboard.pairCount) {
				int pairsLeft = stateInfo->match->scoreboard.pairCount - stateInfo->match->homeRunContestState.runnerBatterPairCounter;
				int battingTeamIndex = (stateInfo->match->scoreboard.
				                        inning+stateInfo->match->scoreboard.playsFirst+stateInfo->match->scoreboard.period)%2;
				int catchingTeamIndex = (battingTeamIndex+1)%2;
				int battingRuns = stateInfo->match->scoreboard.teams[battingTeamIndex].runs;
				int catchingRuns = stateInfo->match->scoreboard.teams[catchingTeamIndex].runs;
				// this will allow game to end if catching team has too many runs for batting team ever to catch up.
				if((stateInfo->match->scoreboard.inning+1)%2 == 0 &&pairsLeft*2 + battingRuns < catchingRuns) {
					stateInfo->match->halfInningState.endPeriod = 1;
				} else {
					loadMutableWorldSettings(stateInfo, rng_seed);
				}
			}
		}
	}
}
