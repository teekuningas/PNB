#include "game_setup.h"
#include "common_logic.h"
#include <string.h>

void initializeGameFromMenu(StateInfo* stateInfo, const GameSetup* gameSetup)
{
	stateInfo->stopSoundEffect = SOUND_MENU;
	stateInfo->screen = SCREEN_GAME;
	stateInfo->changeScreen = 1;
	stateInfo->updated = 0;

	// Set teams and controls for all game modes
	stateInfo->globalGameInfo->teams[0].value = gameSetup->team1 + 1;
	stateInfo->globalGameInfo->teams[1].value = gameSetup->team2 + 1;
	stateInfo->globalGameInfo->teams[0].control = gameSetup->team1_control;
	stateInfo->globalGameInfo->teams[1].control = gameSetup->team2_control;
	stateInfo->globalGameInfo->halfInningsInPeriod = gameSetup->halfInningsInPeriod;

	if (gameSetup->gameMode == GAME_MODE_NORMAL) {
		stateInfo->globalGameInfo->inning = 0;
		stateInfo->globalGameInfo->period = 0;
		stateInfo->globalGameInfo->playsFirst = gameSetup->playsFirst;
		for (int i = 0; i < 2; i++) {
			stateInfo->globalGameInfo->teams[i].runs = 0;
			stateInfo->globalGameInfo->teams[i].period0Runs = 0;
			stateInfo->globalGameInfo->teams[i].period1Runs = 0;
			stateInfo->globalGameInfo->teams[i].period2Runs = 0;
			stateInfo->globalGameInfo->teams[i].period3Runs = 0;
		}
	}

	if (gameSetup->gameMode == GAME_MODE_NORMAL || gameSetup->gameMode == GAME_MODE_SUPER_INNING) {
		stateInfo->globalGameInfo->playsFirst = gameSetup->playsFirst;
	}

	if (gameSetup->gameMode == GAME_MODE_HOMERUN_CONTEST) {
		int half = gameSetup->homerun_choice_count;
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < half; j++) {
				stateInfo->globalGameInfo->teams[0].batterRunnerIndices[i][j] = gameSetup->homerun_choices1[i][j];
				stateInfo->globalGameInfo->teams[1].batterRunnerIndices[i][j] = gameSetup->homerun_choices2[i][j];
			}
			if (stateInfo->globalGameInfo->period > 4) {
				for (int j = half; j < MAX_HOMERUN_PAIRS; j++) {
					stateInfo->globalGameInfo->teams[0].batterRunnerIndices[i][j] = -1;
					stateInfo->globalGameInfo->teams[1].batterRunnerIndices[i][j] = -1;
				}
			}
		}
		stateInfo->globalGameInfo->pairCount = half;
		stateInfo->localGameInfo->gAI.runnerBatterPairCounter = 0;
	} else {
		stateInfo->globalGameInfo->teams[0].batterOrderIndex = 0;
		stateInfo->globalGameInfo->teams[1].batterOrderIndex = 0;
		memcpy(stateInfo->globalGameInfo->teams[0].batterOrder, gameSetup->team1_batting_order, sizeof(gameSetup->team1_batting_order));
		memcpy(stateInfo->globalGameInfo->teams[1].batterOrder, gameSetup->team2_batting_order, sizeof(gameSetup->team2_batting_order));
	}

	loadMutableWorldSettings(stateInfo);
}

void returnToGame(StateInfo* stateInfo)
{
	stateInfo->stopSoundEffect = SOUND_MENU;
	stateInfo->screen = SCREEN_GAME;
	stateInfo->changeScreen = 1;
	stateInfo->updated = 0;
	loadMutableWorldSettings(stateInfo);
}