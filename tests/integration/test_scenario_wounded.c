#include "test_scenario_wounded.h"
#include "fixtures.h"
#include "../test_helpers.h"
#include "game_setup.h"
#include "common_logic.h"
#include "game_analysis.h"
#include "mutable_world.h"

static int test_runner_wounded_if_off_base_when_ball_caught() {
    // Setup: Runner on first, batter hits, runner advances but ball is caught.
    StateInfo* state = setup_test_state();
    setup_runner_at_first_base(state);
    
    int runnerIndex = 0; // The runner is player 0 from the fixture

    // Batter hits
    state->localGameInfo->pII.batterIndex = 1;
    state->localGameInfo->playerInfo[1].bTPI.base = -1;
    state->localGameInfo->pII.battingTeamOnFieldIndices[1] = 1;
    state->localGameInfo->pRAI.batHit = 1;
    state->localGameInfo->ballInfo.moving = 1;
    
    // Runner on 1st starts running towards 2nd
    state->localGameInfo->playerInfo[runnerIndex].bTPI.base = 1;
    state->localGameInfo->playerInfo[runnerIndex].isSafe = 0;
    
    // Ball is caught by a fielder
    state->localGameInfo->pRAI.ballCaught = 1;
    state->localGameInfo->gAI.checkForOuts = 1;

    unsigned int seed = 0;
    gameAnalysis(state, NULL, &seed);

    int isWounded = state->localGameInfo->playerInfo[runnerIndex].bTPI.wounded;
    cleanup_test_state(state);
    ASSERT_EQ(1, isWounded, "Runner should be wounded when off base and ball is caught");
    return TEST_PASSED;
}

void run_scenario_wounded_tests() {
    RUN_TEST(test_runner_wounded_if_off_base_when_ball_caught);
}
