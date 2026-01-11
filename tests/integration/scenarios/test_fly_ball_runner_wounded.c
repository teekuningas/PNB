#include "../scenario_builder.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include <stdio.h>
#include <math.h>

/**
 * TEST 3: Runner wounded on fly ball
 */
int test_full_fly_ball_runner_wounded(void)
{
	ScenarioContext* ctx = create_scenario();
	place_runner_at_base(ctx, 0, BASE_FIRST, 0.0f);
	setup_batter_at_home(ctx, 1);

	Vector3D away = {100.0f, 0.0f, 100.0f};
	ctx->state->localGameInfo->playerInfo[12].tPI.location = away;
	ctx->state->localGameInfo->playerInfo[13].tPI.location = away;

	int fielderIdx = 15;
	Vector3D fielderLoc = ctx->state->fieldPositions->thirdBase;
	ctx->state->localGameInfo->playerInfo[fielderIdx].tPI.location = fielderLoc;
	ctx->state->localGameInfo->playerInfo[fielderIdx].tPI.homeLocation = fielderLoc;

	trigger_player_run_to_next_base(ctx, 0, BASE_FIRST);
	hit_fly_ball_to_location(ctx, ctx->state->fieldPositions->pitchPlate, fielderLoc);

	int woundDetected = 0;
	for (int frame = 0; frame <= 800; frame += 50) {
		if (frame > 0) simulate_frames(ctx, 50);
		if (ctx->state->localGameInfo->playerInfo[0].bTPI.state == PLAYER_STATE_WOUNDED ||
		        ctx->state->localGameInfo->playerInfo[0].bTPI.baseId == BASE_NONE) {
			woundDetected = 1;
			break;
		}
	}

	cleanup_scenario(ctx);
	ASSERT_EQ(1, woundDetected, "Runner should be wounded");
	return TEST_PASSED;
}
