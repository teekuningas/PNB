#include "game_setup.h"
#include "common_logic.h"
#include "rules_pure/player_utils.h"
#include "referee.h"
#include <string.h>

void initialize_game_from_menu(StateInfo* stateInfo, const GameSetup* gameSetup, unsigned int* rng_seed)
{
    stateInfo->stopSoundEffect = SOUND_MENU;
    stateInfo->screen = SCREEN_GAME;
    stateInfo->changeScreen = 1;
    stateInfo->updated = 0;

    // Set teams and controls for all game modes
    stateInfo->rules->scoreboard.teams[0].value = gameSetup->team1 + 1;
    stateInfo->rules->scoreboard.teams[1].value = gameSetup->team2 + 1;
    stateInfo->rules->scoreboard.teams[0].control = gameSetup->team1_control;
    stateInfo->rules->scoreboard.teams[1].control = gameSetup->team2_control;
    stateInfo->rules->scoreboard.halfInningsInPeriod = gameSetup->halfInningsInPeriod;

    if (gameSetup->gameMode == GAME_MODE_NORMAL) {
        stateInfo->rules->scoreboard.inning = 0;
        stateInfo->rules->scoreboard.period = 0;
        stateInfo->rules->scoreboard.playsFirst = gameSetup->playsFirst;
        for (int i = 0; i < 2; i++) {
            stateInfo->rules->scoreboard.teams[i].runs = 0;
            stateInfo->rules->scoreboard.teams[i].period0Runs = 0;
            stateInfo->rules->scoreboard.teams[i].period1Runs = 0;
            stateInfo->rules->scoreboard.teams[i].period2Runs = 0;
            stateInfo->rules->scoreboard.teams[i].period3Runs = 0;
        }
    }

    if (gameSetup->gameMode == GAME_MODE_NORMAL || gameSetup->gameMode == GAME_MODE_SUPER_INNING) {
        stateInfo->rules->scoreboard.playsFirst = gameSetup->playsFirst;
    }

    // Initialize batterOrder for ALL game modes.
    // CRITICAL: batterOrder defines the jersey numbers (1-9 for regulars, 0 for jokers)
    // via initialize_inning_permanent_player_information() in common_logic.c.
    //
    // In GAME_MODE_HOMERUN_CONTEST:
    //   - batterOrder is NOT used to determine who bats (that uses batterRunnerIndices)
    //   - BUT it IS used to assign jersey numbers and joker status
    //   - Players keep their jersey numbers from the super inning
    //   - For test fixtures starting directly in homerun contest, we use default [0,1,2,...,11]
    //
    // batterOrder[0..8]  → players who get jerseys 1-9 (JOKER_REGULAR)
    // batterOrder[9..11] → players who get jersey 0 (JOKER_AVAILABLE)
    stateInfo->rules->scoreboard.teams[0].batterOrderIndex = 0;
    stateInfo->rules->scoreboard.teams[1].batterOrderIndex = 0;
    memcpy(
        stateInfo->rules->scoreboard.teams[0].batterOrder, gameSetup->team1_batting_order,
        sizeof(gameSetup->team1_batting_order)
    );
    memcpy(
        stateInfo->rules->scoreboard.teams[1].batterOrder, gameSetup->team2_batting_order,
        sizeof(gameSetup->team2_batting_order)
    );

    if (gameSetup->gameMode == GAME_MODE_HOMERUN_CONTEST) {
        int half = gameSetup->homerun_choice_count;
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < half; j++) {
                stateInfo->rules->scoreboard.teams[0].batterRunnerIndices[i][j] = gameSetup->homerun_choices1[i][j];
                stateInfo->rules->scoreboard.teams[1].batterRunnerIndices[i][j] = gameSetup->homerun_choices2[i][j];
            }
            if (stateInfo->rules->scoreboard.period > 4) {
                for (int j = half; j < MAX_HOMERUN_PAIRS; j++) {
                    stateInfo->rules->scoreboard.teams[0].batterRunnerIndices[i][j] = -1;
                    stateInfo->rules->scoreboard.teams[1].batterRunnerIndices[i][j] = -1;
                }
            }
        }
        stateInfo->rules->scoreboard.pairCount = half;
        stateInfo->rules->homeRunContestState.runnerBatterPairCounter = 0;
    }

    // reset_for_new_half_inning is called via update_game_screen -> loadGameScreenSettings
    // because we set changeScreen = 1
}

void return_to_game(StateInfo* stateInfo, unsigned int* rng_seed)
{
    stateInfo->stopSoundEffect = SOUND_MENU;
    stateInfo->screen = SCREEN_GAME;
    stateInfo->changeScreen = 1;
    stateInfo->updated = 0;

    // Initialization will be handled by update_game_screen() → loadGameScreenSettings()
    // This makes the flow consistent with initialize_game_from_menu()
}