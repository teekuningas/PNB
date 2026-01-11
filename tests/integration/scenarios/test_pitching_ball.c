#include "../scenario_builder.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include <stdio.h>

/**
 * TEST: Pitching Ball Resolution
 *
 * Verifies that a pitch thrown wide of the plate results in a Ball.
 */
int test_full_pitching_ball(void)
{
	ScenarioContext* ctx = create_scenario();
	setup_batter_at_home(ctx, 0);

	// Move Lukkari away
	ctx->state->localGameInfo->playerInfo[12].tPI.location.x = 100.0f;

	printf("[TEST] Pitching BALL (Target X=1.0, PlateWidth=0.75)\n");
	perform_pitch(ctx, 1.0f); // Wide right

	int ballDetected = 0;
	for (int i = 0; i < 200; i++) {
		simulate_frames(ctx, 1);

		if (ctx->state->localGameInfo->gameState.balls == 1) {
			ballDetected = 1;
			printf("[TEST] Frame %d: Ball detected! Count: %d-%d\n",
			       i, ctx->state->localGameInfo->gameState.balls, ctx->state->localGameInfo->gameState.strikes);
			break;
		}
	}

	cleanup_scenario(ctx);
	ASSERT_EQ(1, ballDetected, "Should detect Ball 1");
	return TEST_PASSED;
}
