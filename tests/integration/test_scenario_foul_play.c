#include "test_scenario_foul_play.h"
#include "fixtures.h"
#include "../test_helpers.h"
#include "game_setup.h"
#include "common_logic.h"
#include "game_analysis.h"
#include "base_logic.h"

/**
 * Scenario: §35 Returning to base after foul play.
 * Runner A is on 1st base (originalBase = 1).
 * Runner A runs to 2nd base.
 * Foul play (out of bounds) occurs.
 * Result: Runner A should be returned to 1st base and state set to SAFE_ON_BASE.
 */
static int test_foul_play_returns_runner_to_base() {
    StateInfo* state = setup_test_state();
    unsigned int seed = 0;
    GameSetup setup = {0};
    setup.launchType = GAME_LAUNCH_NEW;
    setup.gameMode = GAME_MODE_NORMAL;
    setup.team1 = 0;
    setup.team2 = 1;
    initializeGameFromMenu(state, &setup, &seed);
    loadMutableWorldSettings(state, &seed);

    int runnerA = 0;
    state->localGameInfo->pII.battingTeamOnFieldIndices[0] = runnerA;
    state->localGameInfo->playerInfo[runnerA].bTPI.baseId = BASE_FIRST;
    state->localGameInfo->playerInfo[runnerA].bTPI.originalBase = BASE_FIRST;
    set_test_player_state(state, runnerA, PLAYER_STATE_SAFE_ON_BASE);

    // Runner A moves towards 2nd
    state->localGameInfo->playerInfo[runnerA].bTPI.baseId = BASE_SECOND;
    set_test_player_state(state, runnerA, PLAYER_STATE_RUNNING);

    // Trigger Foul Play
    state->localGameInfo->gameState.outOfBounds = 1;
    state->localGameInfo->gameFlowState.outOfBoundsCounter = 1000; // Force immediate reset
    
    MenuInfo menu = {0};
    gameAnalysis(state, &menu, &seed); 

    // Verify runner is back at 1st base
    ASSERT_EQ(BASE_FIRST, state->localGameInfo->playerInfo[runnerA].bTPI.baseId, "Runner should be back at 1st base after foul play");
    ASSERT_EQ(PLAYER_STATE_SAFE_ON_BASE, state->localGameInfo->playerInfo[runnerA].bTPI.state, "Runner should be SAFE_ON_BASE after foul play");

    cleanup_test_state(state);
    return TEST_PASSED;
}

/**
 * Scenario: §35 Batter hit foul on 3rd strike -> OUT.
 * Runner A is batter (originalBase = 0).
 * 2 strikes already.
 * Batter hits foul.
 * Result: Batter should be OUT.
 */
static int test_foul_play_third_strike_out() {
    StateInfo* state = setup_test_state();
    unsigned int seed = 0;
    GameSetup setup = {0};
    setup.launchType = GAME_LAUNCH_NEW;
    setup.gameMode = GAME_MODE_NORMAL;
    setup.team1 = 0;
    setup.team2 = 1;
    initializeGameFromMenu(state, &setup, &seed);
    loadMutableWorldSettings(state, &seed);

    int batter = 0;
    state->localGameInfo->pII.batterIndex = batter;
    state->localGameInfo->pII.battingTeamOnFieldIndices[0] = batter;
    state->localGameInfo->playerInfo[batter].bTPI.baseId = BASE_HOME;
    state->localGameInfo->playerInfo[batter].bTPI.originalBase = BASE_HOME;
    set_test_player_state(state, batter, PLAYER_STATE_AT_BAT);

    // 3 strikes (logic checks if strikes == 3)
    // Wait, in game_analysis.c: "else if(stateInfo->localGameInfo->playerInfo[index].bTPI.baseId == BASE_HOME) { if(stateInfo->localGameInfo->gameState.strikes == 3) {"
    state->localGameInfo->gameState.strikes = 3;

    // Trigger Foul Play
    state->localGameInfo->gameState.outOfBounds = 1;
    state->localGameInfo->gameFlowState.outOfBoundsCounter = 1000;
    
    MenuInfo menu = {0};
    gameAnalysis(state, &menu, &seed); 

    ASSERT_EQ(1, state->localGameInfo->gameState.outs, "Batter should be out after 3rd strike foul");
    
    // Check if batter removed from field
    int found = 0;
    for(int k=0; k<4; k++) if(state->localGameInfo->pII.battingTeamOnFieldIndices[k] == batter) found = 1;
    ASSERT_EQ(0, found, "Batter should be removed from field after out");

    cleanup_test_state(state);
    return TEST_PASSED;
}

void run_scenario_foul_play_tests() {
    RUN_TEST(test_foul_play_returns_runner_to_base);
    RUN_TEST(test_foul_play_third_strike_out);
}
