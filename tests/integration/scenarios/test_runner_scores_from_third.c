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
	ctx->state->match->playerRuntime[0].passedPathPoint = 1;
	place_ball_at_location(ctx, (Vector3D) {
		10.0f, 0.0f, -10.0f
	});
	ctx->state->match->gameControl.checkForRun = 1;
	trigger_player_run_to_next_base(ctx, 0, BASE_THIRD);

	simulate_frames(ctx, 450);

	int runs = ctx->state->match->halfInningState.runsInTheInning;
	cleanup_scenario(ctx);
	ASSERT_EQ(1, runs, "Runner should have scored from third base");
	return TEST_PASSED;
}
