#include "game_setup.h"
#include "tick_outputs.h"
#include "rules_state.h"
#include "session.h"
#include "rng.h"
#include "ai/ai_controller.h"
#include <string.h>

void initialize_game_from_menu(StateInfo* stateInfo, const GameSetup* gameSetup, unsigned int* rng_seed)
{
    stateInfo->stopSoundEffect = SOUND_MENU;
    stateInfo->screen = SCREEN_GAME;
    stateInfo->changeScreen = 1;
    stateInfo->updated = 0;

    // A match starts here, so this is where its random streams are born. The caller's seed is the
    // app-level stream (menus, cup simulation); the match gets two independent children of it:
    // one for the engine (World state, snapshotted) and one for the AI controller (controller-
    // private, never synced). Both stay reproducible from the caller's seed alone, and the controller's whole
    // memory is born here with its stream: it lives exactly as long as the match.
    stateInfo->match->rng_seed = rng_split(rng_seed);
    if (stateInfo->aiController != NULL) {
        init_ai_controller(stateInfo->aiController, rng_split(rng_seed));
    }

    // Set teams and controls for all game modes
    stateInfo->rules->scoreboard.teams[0].value = gameSetup->team1 + 1;
    stateInfo->rules->scoreboard.teams[1].value = gameSetup->team2 + 1;
    stateInfo->rules->scoreboard.teams[0].control = gameSetup->team1_control;
    stateInfo->rules->scoreboard.teams[1].control = gameSetup->team2_control;
    stateInfo->rules->scoreboard.half_innings_in_period = gameSetup->halfInningsInPeriod;

    if (gameSetup->gameMode == GAME_MODE_NORMAL) {
        stateInfo->rules->scoreboard.inning = 0;
        stateInfo->rules->scoreboard.period = 0;
        stateInfo->rules->scoreboard.plays_first = gameSetup->playsFirst;
        for (int i = 0; i < 2; i++) {
            stateInfo->rules->scoreboard.teams[i].runs = 0;
            stateInfo->rules->scoreboard.teams[i].period0_runs = 0;
            stateInfo->rules->scoreboard.teams[i].period1_runs = 0;
            stateInfo->rules->scoreboard.teams[i].period2_runs = 0;
            stateInfo->rules->scoreboard.teams[i].period3_runs = 0;
        }
    }

    if (gameSetup->gameMode == GAME_MODE_NORMAL || gameSetup->gameMode == GAME_MODE_SUPER_INNING) {
        stateInfo->rules->scoreboard.plays_first = gameSetup->playsFirst;
    }

    // Initialize batter_order for ALL game modes.
    // CRITICAL: batter_order defines the jersey numbers (1-9 for regulars, 0 for jokers)
    // via initialize_inning_permanent_player_information() in game_reset.c.
    //
    // In GAME_MODE_HOMERUN_CONTEST:
    //   - batter_order is NOT used to determine who bats (that uses batter_runner_indices)
    //   - BUT it IS used to assign jersey numbers and joker status
    //   - Players keep their jersey numbers from the super inning
    //   - For test fixtures starting directly in homerun contest, we use default [0,1,2,...,11]
    //
    // batter_order[0..8]  → players who get jerseys 1-9 (JOKER_REGULAR)
    // batter_order[9..11] → players who get jersey 0 (JOKER_AVAILABLE)
    stateInfo->rules->scoreboard.teams[0].batter_order_index = 0;
    stateInfo->rules->scoreboard.teams[1].batter_order_index = 0;
    memcpy(
        stateInfo->rules->scoreboard.teams[0].batter_order, gameSetup->team1_batting_order,
        sizeof(gameSetup->team1_batting_order)
    );
    memcpy(
        stateInfo->rules->scoreboard.teams[1].batter_order, gameSetup->team2_batting_order,
        sizeof(gameSetup->team2_batting_order)
    );

    if (gameSetup->gameMode == GAME_MODE_HOMERUN_CONTEST) {
        int half = gameSetup->homerun_choice_count;
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < half; j++) {
                stateInfo->rules->scoreboard.teams[0].batter_runner_indices[i][j] = gameSetup->homerun_choices1[i][j];
                stateInfo->rules->scoreboard.teams[1].batter_runner_indices[i][j] = gameSetup->homerun_choices2[i][j];
            }
            if (stateInfo->rules->scoreboard.period > 4) {
                for (int j = half; j < MAX_HOMERUN_PAIRS; j++) {
                    stateInfo->rules->scoreboard.teams[0].batter_runner_indices[i][j] = -1;
                    stateInfo->rules->scoreboard.teams[1].batter_runner_indices[i][j] = -1;
                }
            }
        }
        stateInfo->rules->scoreboard.pair_count = half;
        stateInfo->rules->homerun_contest.pair_index = 0;
    }

    // reset_for_new_half_inning is called via update_game_screen -> loadGameScreenSettings
    // because we set changeScreen = 1
}

void return_to_game(StateInfo* stateInfo)
{
    stateInfo->stopSoundEffect = SOUND_MENU;
    stateInfo->screen = SCREEN_GAME;
    stateInfo->changeScreen = 1;
    stateInfo->updated = 0;

    // Initialization will be handled by update_game_screen() → loadGameScreenSettings()
    // This makes the flow consistent with initialize_game_from_menu()
}