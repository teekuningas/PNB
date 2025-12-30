#include "test_scenario_outs.h"
#include "fixtures.h"
#include "../test_helpers.h"
#include "game_setup.h"
#include "common_logic.h"
#include "rules_outs.h"
#include "game_analysis.h"

static int test_runner_forced_out_at_first_base_scenario() {
    // Setup: Runner on first, batter hits
    StateInfo* state = setup_test_state();
    setup_runner_at_first_base(state);
    state->localGameInfo->pII.batterIndex = 1;
    state->localGameInfo->playerInfo[0].bTPI.base = 0;
    state->localGameInfo->pII.battingTeamOnFieldIndices[0] = 0;
    
    // Simulate a hit
    state->localGameInfo->pRAI.batHit = 1;
    state->localGameInfo->ballInfo.moving = 1;
    state->localGameInfo->playerInfo[0].bTPI.isOnBase = 0;

    // Move ball to first base
    state->localGameInfo->ballInfo.location = state->fieldPositions->firstBase;
    state->localGameInfo->pII.hasBallIndex = 13; // Give ball to first baseman

    // Now check for outs.
    MenuInfo menu = {0};
    gameAnalysis(state, &menu, NULL);

    int outs = state->localGameInfo->gAI.outs;
    cleanup_test_state(state);
    ASSERT_EQ(1, outs, "Runner should be forced out at first base");
    return TEST_PASSED;
}

void run_scenario_outs_tests() {
    RUN_TEST(test_runner_forced_out_at_first_base_scenario);
}
