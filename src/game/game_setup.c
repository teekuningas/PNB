#include "game_setup.h"
#include "common_logic.h"
#include "rules_pure/player_utils.h"
#include <string.h>

void initializeGameFromMenu(StateInfo* stateInfo, const GameSetup* gameSetup, unsigned int* rng_seed)
{
	stateInfo->stopSoundEffect = SOUND_MENU;
	stateInfo->screen = SCREEN_GAME;
	stateInfo->changeScreen = 1;
	stateInfo->updated = 0;

	// Set teams and controls for all game modes
	stateInfo->match->scoreboard.teams[0].value = gameSetup->team1 + 1;
	stateInfo->match->scoreboard.teams[1].value = gameSetup->team2 + 1;
	stateInfo->match->scoreboard.teams[0].control = gameSetup->team1_control;
	stateInfo->match->scoreboard.teams[1].control = gameSetup->team2_control;
	stateInfo->match->scoreboard.halfInningsInPeriod = gameSetup->halfInningsInPeriod;

	if (gameSetup->gameMode == GAME_MODE_NORMAL) {
		stateInfo->match->scoreboard.inning = 0;
		stateInfo->match->scoreboard.period = 0;
		stateInfo->match->scoreboard.playsFirst = gameSetup->playsFirst;
		for (int i = 0; i < 2; i++) {
			stateInfo->match->scoreboard.teams[i].runs = 0;
			stateInfo->match->scoreboard.teams[i].period0Runs = 0;
			stateInfo->match->scoreboard.teams[i].period1Runs = 0;
			stateInfo->match->scoreboard.teams[i].period2Runs = 0;
			stateInfo->match->scoreboard.teams[i].period3Runs = 0;
		}
	}

	if (gameSetup->gameMode == GAME_MODE_NORMAL || gameSetup->gameMode == GAME_MODE_SUPER_INNING) {
		stateInfo->match->scoreboard.playsFirst = gameSetup->playsFirst;
	}

	// Clear betweenPitchState when starting a new game (prevents state corruption from previous game)
	stateInfo->match->betweenPitchState.catchHasBeenMade = 0;
	stateInfo->match->betweenPitchState.hasBallHitGround = 0;
	stateInfo->match->betweenPitchState.outOfBounds = 0;
	stateInfo->match->betweenPitchState.resolutionProcessed = 0;

	if (gameSetup->gameMode == GAME_MODE_HOMERUN_CONTEST) {
		int half = gameSetup->homerun_choice_count;
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < half; j++) {
				stateInfo->match->scoreboard.teams[0].batterRunnerIndices[i][j] = gameSetup->homerun_choices1[i][j];
				stateInfo->match->scoreboard.teams[1].batterRunnerIndices[i][j] = gameSetup->homerun_choices2[i][j];
			}
			if (stateInfo->match->scoreboard.period > 4) {
				for (int j = half; j < MAX_HOMERUN_PAIRS; j++) {
					stateInfo->match->scoreboard.teams[0].batterRunnerIndices[i][j] = -1;
					stateInfo->match->scoreboard.teams[1].batterRunnerIndices[i][j] = -1;
				}
			}
		}
		stateInfo->match->scoreboard.pairCount = half;
		stateInfo->match->homeRunContestState.runnerBatterPairCounter = 0;
	} else {
		stateInfo->match->scoreboard.teams[0].batterOrderIndex = 0;
		stateInfo->match->scoreboard.teams[1].batterOrderIndex = 0;
		memcpy(stateInfo->match->scoreboard.teams[0].batterOrder, gameSetup->team1_batting_order, sizeof(gameSetup->team1_batting_order));
		memcpy(stateInfo->match->scoreboard.teams[1].batterOrder, gameSetup->team2_batting_order, sizeof(gameSetup->team2_batting_order));
	}

	loadMutableWorldSettings(stateInfo, rng_seed);
}

void returnToGame(StateInfo* stateInfo, unsigned int* rng_seed)
{
	stateInfo->stopSoundEffect = SOUND_MENU;
	stateInfo->screen = SCREEN_GAME;
	stateInfo->changeScreen = 1;
	stateInfo->updated = 0;
	loadMutableWorldSettings(stateInfo, rng_seed);
}

void applyFoulPlayReset(StateInfo* stateInfo, unsigned int* rng_seed)
{
	MatchSession* game = stateInfo->match;

	// Reset standard game state
	initializeBallInfo(game);
	initializeActionInfo(game);
	initializeTemporaryGameAnalysisInfo(game);
	initializeIndexInformation(game);
	initializePRAIInformation(game);
	initializeSpatialPlayerInformation(game, stateInfo->fieldPositions, rng_seed);
	initializeNonCriticalPlayerInformation(game);

	if (game->scoreboard.period >= 4) {
		// Homerun Contest special initialization
		initializeCriticalBattingTeamInformation(game);
		setRunnerAndBatter(game, &game->scoreboard, stateInfo->fieldPositions);
	} else {
		// Restore players to their bases at the start of the pitch
		for (int j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
			if (game->referee.battingPlayers[j].baseAtPitchStart != BASE_NONE) {
				BaseID restoreBase = game->referee.battingPlayers[j].baseAtPitchStart;

				// 1. Restore Player State and ID
				if (restoreBase == BASE_HOME) {
					game->playerInfo[j].bTPI.state = PLAYER_STATE_AT_BAT;
				} else {
					game->playerInfo[j].bTPI.state = PLAYER_STATE_ON_BASE;
				}
				game->playerInfo[j].bTPI.baseId = restoreBase;

				// 2. Restore Safety Status (Always safe at original base after foul)
				game->referee.battingPlayers[j].currentSafetyBase = restoreBase;

				// 3. Clear transient rule states
				game->referee.battingPlayers[j].pendingWoundState = WOUND_STATE_NONE;
				game->referee.battingPlayers[j].woundingType = WOUNDING_TYPE_NONE;

				// 4. Handle scoring from 3rd base (foul fly walk case)
				if (game->playerInfo[j].bTPI.baseId == BASE_HOME_SCORED) {
					int battingTeamIndex = (game->scoreboard.inning + game->scoreboard.playsFirst + game->scoreboard.period) % 2;
					game->scoreboard.teams[battingTeamIndex].runs += 1;
					game->halfInningState.runsInTheInning += 1;
					game->halfInningState.event = EVENT_RUN_SCORED;
					game->playerInfo[j].bTPI.baseId = BASE_NONE;

					if (game->halfInningState.runsInTheInning % 2 == 0) {
						game->playerCounters.nonJokerPlayersLeft = PLAYERS_IN_TEAM;
					}
				}
				// 5. Handle the Batter
				else if (game->playerInfo[j].bTPI.baseId == BASE_HOME) {
					if (game->referee.strikesAtPitchStart == 2) {
						// 3rd Strike OUT (§33)
						game->halfInningState.strikes = 3;
						game->halfInningState.outs += 1;
						game->playerInfo[j].bTPI.state = PLAYER_STATE_OUT;
						game->playerInfo[j].bTPI.baseId = BASE_NONE;
						game->referee.battingPlayers[j].currentSafetyBase = BASE_NONE;
						game->referee.battingPlayers[j].baseAtPitchStart = BASE_NONE;
					} else {
						// Continue batting with incremented strikes
						game->halfInningState.strikes = game->referee.strikesAtPitchStart + 1;
						prepareBatter(game);
					}
				}
				// 6. Restore Physical Locations for field runners
				else if (game->playerInfo[j].bTPI.baseId == BASE_FIRST) {
					game->playerInfo[j].tPI.location.x = stateInfo->fieldPositions->firstBaseRun.x;
					game->playerInfo[j].tPI.location.z = stateInfo->fieldPositions->firstBaseRun.z;
				} else if (game->playerInfo[j].bTPI.baseId == BASE_SECOND) {
					game->playerInfo[j].tPI.location.x = stateInfo->fieldPositions->secondBaseRun.x;
					game->playerInfo[j].tPI.location.z = stateInfo->fieldPositions->secondBaseRun.z;
				} else if (game->playerInfo[j].bTPI.baseId == BASE_THIRD) {
					game->playerInfo[j].tPI.location.x = stateInfo->fieldPositions->thirdBaseRun.x;
					game->playerInfo[j].tPI.location.z = stateInfo->fieldPositions->thirdBaseRun.z;
				}
			} else {
				// Restore OUT/SCORED states to avoid re-triggering animations
				if (game->referee.battingPlayers[j].isOut) {
					game->playerInfo[j].bTPI.state = PLAYER_STATE_OUT;
					game->playerInfo[j].bTPI.baseId = BASE_NONE;
				} else if (game->referee.battingPlayers[j].hasScored) {
					game->playerInfo[j].bTPI.state = PLAYER_STATE_SCORED;
					game->playerInfo[j].bTPI.baseId = BASE_NONE;
				}
			}
		}
	}
}