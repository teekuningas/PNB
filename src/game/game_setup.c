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

	// loadMutableWorldSettings is called via updateGameScreen -> loadGameScreenSettings
	// because we set changeScreen = 1
}

void returnToGame(StateInfo* stateInfo, unsigned int* rng_seed)
{
	stateInfo->stopSoundEffect = SOUND_MENU;
	stateInfo->screen = SCREEN_GAME;
	stateInfo->changeScreen = 1;
	stateInfo->updated = 0;
	loadMutableWorldSettings(stateInfo, rng_seed);

	// Emit gameInitialized event
	stateInfo->match->gameEvents.gameInitialized = 1;
}