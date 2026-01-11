#include "../scenario_builder.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include <stdio.h>

/**
 * TEST: Pitching Strike Resolution
 *
 * Verifies that a pitch thrown to the center of the plate results in a Strike.
 */
int test_full_pitching_strike(void)
{
	ScenarioContext* ctx = create_scenario();
	setup_batter_at_home(ctx, 0);

	// Move Lukkari away
	ctx->state->localGameInfo->playerInfo[12].tPI.location.x = 100.0f;

	printf("[TEST] Pitching STRIKE (Target X=0.0)\n");
	perform_pitch(ctx, 0.0f); // Center of plate

	int strikeDetected = 0;
	for (int i = 0; i < 200; i++) {
		simulate_frames(ctx, 1);
		if (ctx->state->localGameInfo->gameState.strikes == 1) {
			strikeDetected = 1;
			printf("[TEST] Frame %d: Strike detected! Count: %d-%d\n",
			       i, ctx->state->localGameInfo->gameState.balls, ctx->state->localGameInfo->gameState.strikes);
			break;
		}
	}

	cleanup_scenario(ctx);
	ASSERT_EQ(1, strikeDetected, "Should detect Strike 1");
	return TEST_PASSED;
}
