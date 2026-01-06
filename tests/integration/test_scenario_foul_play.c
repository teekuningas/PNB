#include "test_scenario_foul_play.h"
#include "fixtures.h"
#include "../test_helpers.h"
#include "game_setup.h"
#include "common_logic.h"
#include "game_analysis.h"
#include "base_logic.h"

/**
 * Scenario: Foul play resets player positions.
 * Runner A is on 1st base (baseAtPitchStart = 1).
 * Runner A advances to 2nd base and is safe there.
 * Fly ball hit out of bounds (Foul play).
 * Result: Runner A should be returned to 1st base.
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

    // Runner A on 1st
    int runnerA = 0;
    state->localGameInfo->playerInfo[runnerA].bTPI.baseId = BASE_FIRST;
    state->localGameInfo->referee.battingPlayers[runnerA].baseAtPitchStart = BASE_FIRST;
    state->localGameInfo->referee.battingPlayers[runnerA].hadSafetyAtPitchStart = 1;
    state->localGameInfo->referee.battingPlayers[runnerA].currentSafetyBase = BASE_FIRST;
    state->localGameInfo->pII.baseControlIndex[1] = runnerA;
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
 * Scenario: Foul play third strike out.
 * Runner A is batter (baseAtPitchStart = 0).
 * Runner A gets 3rd strike.
 * Ball hit out of bounds (Foul play).
 * Result: Runner A should be out.
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

    // Runner A is batter
    int batter = 0;
    state->localGameInfo->pII.batterIndex = batter;
    state->localGameInfo->playerInfo[batter].bTPI.baseId = BASE_HOME;
    state->localGameInfo->referee.battingPlayers[batter].baseAtPitchStart = BASE_HOME;
    state->localGameInfo->referee.battingPlayers[batter].hadSafetyAtPitchStart = 1;
    state->localGameInfo->referee.battingPlayers[batter].currentSafetyBase = BASE_HOME;
    state->localGameInfo->pII.baseControlIndex[0] = batter;

    // 3 strikes (logic checks if strikes == 3)
    // Wait, in game_analysis.c: "else if(stateInfo->localGameInfo->playerInfo[index].bTPI.baseId == BASE_HOME) { if(stateInfo->localGameInfo->gameState.strikes == 3) {"
    state->localGameInfo->gameState.strikes = 3;

    // Trigger Foul Play
    state->localGameInfo->gameState.outOfBounds = 1;
    state->localGameInfo->gameFlowState.outOfBoundsCounter = 1000;
    
    MenuInfo menu = {0};
    gameAnalysis(state, &menu, &seed); 

    ASSERT_EQ(1, state->localGameInfo->gameState.outs, "Batter should be out after 3rd strike foul");

    cleanup_test_state(state);
    return TEST_PASSED;
}

void run_scenario_foul_play_tests() {
    RUN_TEST(test_foul_play_returns_runner_to_base);
    RUN_TEST(test_foul_play_third_strike_out);
}
