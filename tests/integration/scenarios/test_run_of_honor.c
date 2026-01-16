#include "../scenario_builder.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include <stdio.h>
#include <math.h>

/**
 * TEST: Batter hits ground ball and runs all the way to third base for Run of Honor (§42 Kunniajuoksu)
 *
 * Simplified scenario:
 * 1. Batter at home
 * 2. Hit ground ball (batHit=1, hasHitGround=1)
 * 3. Ball placed far away so runner can safely advance
 * 4. Runner advances to each base sequentially
 * 5. Verify run of honor is scored at third base arrival
 */
int test_full_run_of_honor(void)
{
	printf("\n=== TEST: Run of Honor (Kunniajuoksu) ===\n");
	ScenarioContext* ctx = create_scenario();

	int batterIndex = 0;

	// Setup batter at home using helper
	setup_batter_at_home(ctx, batterIndex);

	// Mark as active batter with proper referee state
	ctx->state->match->playerInfo[batterIndex].bTPI.state = PLAYER_STATE_AT_BAT;
	ctx->state->match->playerInfo[batterIndex].bTPI.baseId = BASE_HOME;
	ctx->state->match->referee.battingPlayers[batterIndex].baseAtPitchStart = BASE_HOME;
	ctx->state->match->referee.battingPlayers[batterIndex].currentSafetyBase = BASE_NONE;
	ctx->state->match->referee.battingPlayers[batterIndex].runOfHonorScored = 0;
	ctx->state->match->playerRuntime[batterIndex].goingForward = 1;

	printf("Setup: Batter %d at HOME, baseAtPitchStart=%d\n",
	       batterIndex, ctx->state->match->referee.battingPlayers[batterIndex].baseAtPitchStart);

	// Place ball far in outfield (ball must have hit ground for run of honor)
	Vector3D ballLocation = {30.0f, 0.0f, 40.0f};
	place_ball_at_location(ctx, ballLocation);

	// Critical: Mark that ball was hit and has hit ground (not a fly ball!)
	ctx->state->match->pRAI.batHit = 1;
	ctx->state->match->pRAI.batterCanAdvance = 1;
	ctx->state->match->ballInfo.hasHitGround = 1;
	ctx->state->match->gameControl.catchHasBeenMade = 0;
	ctx->state->match->referee.woundingEvaluationActive = 0;

	printf("Ball at (%.1f, %.1f, %.1f), hasHitGround=%d\n",
	       ballLocation.x, ballLocation.y, ballLocation.z,
	       ctx->state->match->ballInfo.hasHitGround);

	int runsAtStart = ctx->state->match->halfInningState.runsInTheInning;
	printf("Runs at start: %d\n\n", runsAtStart);

	// === Run to FIRST ===
	printf("--- Running HOME → FIRST ---\n");
	trigger_player_run_to_next_base(ctx, batterIndex, BASE_HOME);

	int framesFirst = 0;
	while (ctx->state->match->playerInfo[batterIndex].bTPI.baseId != BASE_FIRST && framesFirst < 500) {
		simulate_frames(ctx, 1);
		framesFirst++;
	}
	printf("Arrived at FIRST (frame %d), baseId=%d\n", framesFirst,
	       ctx->state->match->playerInfo[batterIndex].bTPI.baseId);

	if (ctx->state->match->playerInfo[batterIndex].bTPI.baseId != BASE_FIRST) {
		printf("ERROR: Did not reach first base!\n");
		cleanup_scenario(ctx);
		return TEST_FAILED;
	}

	// === Run to SECOND ===
	printf("\n--- Running FIRST → SECOND ---\n");
	ctx->state->match->playerRuntime[batterIndex].goingForward = 1;
	trigger_player_run_to_next_base(ctx, batterIndex, BASE_FIRST);

	int framesSecond = 0;
	while (ctx->state->match->playerInfo[batterIndex].bTPI.baseId != BASE_SECOND && framesSecond < 500) {
		simulate_frames(ctx, 1);
		framesSecond++;
	}
	printf("Arrived at SECOND (frame %d), baseId=%d\n", framesSecond,
	       ctx->state->match->playerInfo[batterIndex].bTPI.baseId);

	if (ctx->state->match->playerInfo[batterIndex].bTPI.baseId != BASE_SECOND) {
		printf("ERROR: Did not reach second base!\n");
		cleanup_scenario(ctx);
		return TEST_FAILED;
	}

	// === Run to THIRD (RUN OF HONOR!) ===
	printf("\n--- Running SECOND → THIRD (Expecting Run of Honor!) ---\n");
	int runsBefore = ctx->state->match->halfInningState.runsInTheInning;
	printf("Runs before: %d\n", runsBefore);
	printf("runOfHonorScored flag before: %d\n",
	       ctx->state->match->referee.battingPlayers[batterIndex].runOfHonorScored);

	ctx->state->match->playerRuntime[batterIndex].goingForward = 1;
	trigger_player_run_to_next_base(ctx, batterIndex, BASE_SECOND);

	int framesThird = 0;
	int arrivedAtThird = 0;
	while (framesThird < 500) {
		simulate_frames(ctx, 1);
		framesThird++;

		if (ctx->state->match->playerInfo[batterIndex].bTPI.baseId == BASE_THIRD && !arrivedAtThird) {
			arrivedAtThird = 1;
			printf("\n*** ARRIVED AT THIRD BASE (frame %d) ***\n", framesThird);
			printf("  baseId: %d\n", ctx->state->match->playerInfo[batterIndex].bTPI.baseId);
			printf("  baseAtPitchStart: %d (HOME)\n", ctx->state->match->referee.battingPlayers[batterIndex].baseAtPitchStart);
			printf("  goingForward: %d\n", ctx->state->match->playerRuntime[batterIndex].goingForward);
			printf("  wounded: %d\n", ctx->state->match->playerInfo[batterIndex].bTPI.state == PLAYER_STATE_WOUNDED);

			// Give referee a few frames to process
			simulate_frames(ctx, 20);
			break;
		}
	}

	if (!arrivedAtThird) {
		printf("ERROR: Did not reach third base after %d frames!\n", framesThird);
		cleanup_scenario(ctx);
		return TEST_FAILED;
	}

	// === VERIFICATION ===
	printf("\n--- VERIFICATION ---\n");
	int runsAfter = ctx->state->match->halfInningState.runsInTheInning;
	int runOfHonorFlag = ctx->state->match->referee.battingPlayers[batterIndex].runOfHonorScored;
	int playerStillAtThird = (ctx->state->match->playerInfo[batterIndex].bTPI.baseId == BASE_THIRD);
	int hasScored = ctx->state->match->referee.battingPlayers[batterIndex].hasScored;

	printf("Runs after: %d (expected: %d)\n", runsAfter, runsBefore + 1);
	printf("runOfHonorScored: %d (should be 1)\n", runOfHonorFlag);
	printf("Still at third: %d (should be 1)\n", playerStillAtThird);
	printf("hasScored: %d (should be 0)\n", hasScored);

	cleanup_scenario(ctx);

	// Assertions
	ASSERT_EQ(runsBefore + 1, runsAfter, "Run of Honor should score 1 run");
	ASSERT_EQ(1, runOfHonorFlag, "runOfHonorScored flag should be set");
	ASSERT_EQ(1, playerStillAtThird, "Runner should stay at third base");
	ASSERT_EQ(0, hasScored, "hasScored should be 0");

	printf("\n=== TEST PASSED ===\n");
	return TEST_PASSED;
}
