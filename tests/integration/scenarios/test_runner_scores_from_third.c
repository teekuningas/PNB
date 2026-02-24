#include "../scenario_builder.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include <stdio.h>
#include <math.h>

/**
 * TEST 1: Runner scores from third base to home
 */
int test_full_runner_scores_from_third(void)
{
    ScenarioContext* ctx = create_scenario();

    place_runner_at_base(ctx, 0, BASE_THIRD, 0.0f);

    // CRITICAL: Move pitcher away from home BEFORE initialization
    // Otherwise pitcher will catch any ball near home plate
    move_pitcher_away(ctx);
    // Also move other fielder that might interfere

    // Initialize referee state from physical setup
    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);

    // Set runtime state AFTER initialization to avoid it being reset
    ctx->state->match->playerRuntime[0].passedPathPoint = 1;

    // Drop ball in the outfield so it hits ground naturally
    Vector3D ballTarget = {30.0f, 0.0f, 40.0f}; // Far in outfield, away from any base
    place_ball_over_location(ctx, ballTarget);

    // checkForRun removed - referee uses playerArrivedAtBase event now
    trigger_player_run_to_next_base(ctx, 0, BASE_THIRD);

    simulate_frames(ctx, 450);

    int runs = ctx->state->match->halfInningState.runsInTheInning;
    cleanup_scenario(ctx);
    ASSERT_EQ(1, runs, "Runner should have scored from third base");
    return TEST_PASSED;
}
