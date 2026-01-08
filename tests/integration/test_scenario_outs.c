#include "test_scenario_outs.h"
#include "fixtures.h"
#include "../test_helpers.h"
#include "game_setup.h"
#include "common_logic.h"
#include "rules_outs.h"
#include "game_analysis.h"

static int test_runner_forced_out_at_first_base_scenario() {
    // Setup: Batter hits and runs to first base, ball beats them there
    StateInfo* state = setup_test_state();
    
    // Initialize game but don't use setup_runner_at_first_base
    unsigned int seed = 0;
    GameSetup setup = {0};
    setup.launchType = GAME_LAUNCH_NEW;
    setup.gameMode = GAME_MODE_NORMAL;
    setup.team1 = 0;
    setup.team2 = 1;
    setup.team1_control = 0;
    setup.team2_control = 2;
    setup.halfInningsInPeriod = 4;
    setup.playsFirst = 0;
    initializeGameFromMenu(state, &setup, &seed);
    initGameAnalysis(&(state->localGameInfo->gameFlowState));
    
    // Batter (player 0) at home, no safety yet (normal batter state)
    state->localGameInfo->playerInfo[0].bTPI.baseId = BASE_HOME;
    state->localGameInfo->referee.battingPlayers[0].currentSafetyBase = BASE_NONE;  // No safety yet!
    state->localGameInfo->pII.batterIndex = 0;
    
    // Simulate a hit
    state->localGameInfo->pRAI.batHit = 1;
    state->localGameInfo->ballInfo.moving = 1;
    set_test_player_state(state, 0, PLAYER_STATE_RUNNING);

    // Move ball to first base (beats the batter there)
    state->localGameInfo->ballInfo.location = state->fieldPositions->firstBase;
    state->localGameInfo->pII.hasBallIndex = 13; // Give ball to first baseman

    // Now check for outs.
    MenuInfo menu = {0};
    gameAnalysis(state, &menu, NULL);

    int outs = state->localGameInfo->gameState.outs;
    cleanup_test_state(state);
    ASSERT_EQ(1, outs, "Batter should be forced out at first base");
    return TEST_PASSED;
}

void run_scenario_outs_tests() {
    RUN_TEST(test_runner_forced_out_at_first_base_scenario);
}
