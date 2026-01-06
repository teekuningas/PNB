#include "test_scenario_runs.h"
#include "fixtures.h"
#include "../test_helpers.h"
#include "game_setup.h"
#include "common_logic.h"
#include "game_analysis.h"

static int test_runner_scores_from_third_base_scenario() {
    // Setup: Runner on third, batter hits
    StateInfo* state = setup_test_state();
    setup_runner_at_third_base(state);
    
    int runnerIndex = 0; // The runner is player 0 from the fixture

    // Batter hits
    state->localGameInfo->pII.batterIndex = 1;
    state->localGameInfo->playerInfo[1].bTPI.baseId = BASE_NONE;
    state->localGameInfo->pRAI.batHit = 1;
    state->localGameInfo->ballInfo.moving = 1;
    
    state->localGameInfo->gameControl.checkForRun = 1;
    state->localGameInfo->ballInfo.hasHitGround = 1;
    
    // Runner on 3rd runs home
    state->localGameInfo->playerInfo[runnerIndex].bTPI.baseId = BASE_HOME_SCORED;

    unsigned int seed = 0;
    MenuInfo menu = {0};
    gameAnalysis(state, &menu, &seed);

    int runs = state->localGameInfo->gameState.runsInTheInning;
    cleanup_test_state(state);
    ASSERT_EQ(1, runs, "Runner should have scored a run");
    return TEST_PASSED;
}

void run_scenario_runs_tests() {
    RUN_TEST(test_runner_scores_from_third_base_scenario);
}
