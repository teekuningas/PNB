#include "../scenario_builder.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include <stdio.h>
#include <math.h>
#include "rules_pure/player_utils.h"

/**
 * TEST 2: Batter forced out at first base
 */
int test_full_batter_forced_out_at_first(void)
{
	ScenarioContext* ctx = create_scenario();
	setup_batter_at_home(ctx, 0);

	Vector3D firstBase = ctx->state->fieldPositions->firstBase;
	ctx->state->localGameInfo->playerInfo[13].tPI.location = firstBase;
	ctx->state->localGameInfo->playerInfo[13].tPI.homeLocation = firstBase;

	Vector3D throwFrom = {5.0f, 1.5f, -10.0f};
	throw_ball_to_base(ctx, throwFrom, BASE_FIRST);

	ctx->state->localGameInfo->pRAI.batHit = 1;
	ctx->state->localGameInfo->pRAI.batterCanAdvance = 1;
	trigger_player_run_to_next_base(ctx, 0, BASE_HOME);

	int outs = 0;
	for (int frame = 0; frame <= 300; frame += 50) {
		if (frame > 0) simulate_frames(ctx, 50);
		outs = ctx->state->localGameInfo->gameState.outs;
		if (outs > 0) break;
	}

	ASSERT_EQ(1, outs, "Batter should be forced out at first base");
	ASSERT_EQ(-1, get_active_batter_index(ctx->state->localGameInfo), "batterIndex (deduced) should be reset to -1 after out");

	cleanup_scenario(ctx);
	return TEST_PASSED;
}
