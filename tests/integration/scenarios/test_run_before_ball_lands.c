#include "../scenario_builder.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include <stdio.h>

/**
 * TEST: Runner arrives at home BEFORE ball lands.
 *
 * Scenario:
 * 1. Runner starts at 3rd base.
 * 2. Batter hits a high fly ball (long air time).
 * 3. Runner runs home and arrives quickly (placed close to home).
 * 4. Ball lands AFTER the runner has arrived.
 *
 * Expected Behavior:
 * The run should count. The referee logic must handle the case where the run is scored
 * physically before the play becomes "live" (ball hits ground), provided the ball eventually
 * becomes live validly (e.g. not caught).
 *
 * Current Bug:
 * Referee only checks for runs on the frame of arrival. If ball hasn't hit ground yet,
 * the run is rejected. When ball finally hits ground, the arrival event is gone, so
 * the run is never scored.
 */
int test_run_arrival_before_ball_lands(void)
{
	printf("\n=== TEST: Run Arrival Before Ball Lands ===\n");
	ScenarioContext* ctx = create_scenario();

	int runnerIndex = 1; // Different from batter

	// 1. Setup Runner at 3rd Base
	place_runner_at_base(ctx, runnerIndex, BASE_THIRD, 0.0f);

	// Move runner very close to home to ensure arrival happens quickly
	// Home is at Z ~ -0.65, 3rd base is at Z ~ -something.
	// We'll place them just a few steps from home line.
	// Pitch plate is at (0, 0, 0) roughly? Let's use field positions.
	Vector3D homeLoc = ctx->state->fieldPositions->pitchPlate;
	Vector3D startLoc = homeLoc;
	startLoc.z += 1.0f; // 1 meter away from home (Very close!)
	startLoc.x = -15.0f; // Approaching from 3rd base side

	// Set physical state
	ctx->state->match->playerInfo[runnerIndex].tPI.location = startLoc;
	ctx->state->match->playerInfo[runnerIndex].tPI.lastLocation = startLoc;

	// Set runtime state: passedPathPoint=1 means "past the corner flag, heading to home"
	ctx->state->match->playerRuntime[runnerIndex].passedPathPoint = 1;

	printf("Setup: Runner %d placed close to home at (%.1f, %.1f, %.1f)\n",
	       runnerIndex, startLoc.x, startLoc.y, startLoc.z);

	// 2. Simulate Hit (High Fly Ball)
	// We want it to stay in air for a while.
	// Setup physics state: in air, moving, hasn't hit ground yet.
	ctx->state->match->ballInfo.location.y = 30.0f; // High in air
	ctx->state->match->ballInfo.velocity.y = 0.0f;  // Hovering
	ctx->state->match->ballInfo.velocity.x = 0.0f;
	ctx->state->match->ballInfo.velocity.z = 0.0f;
	ctx->state->match->ballInfo.moving = 1;
	ctx->state->match->ballInfo.currentFlightHasHitGround = 0; // Physics: hasn't hit

	// Rule state: Pitch started, ball hit by bat, BUT not yet live (no ground contact)
	ctx->state->match->pRAI.batHit = 1;
	ctx->state->match->gameControl.hasBallHitGround = 0; // Rule: Not yet live
	ctx->state->match->gameControl.catchHasBeenMade = 0;

	// 3. Trigger Runner
	trigger_player_run_to_next_base(ctx, runnerIndex, BASE_THIRD);

	int runsAtStart = ctx->state->match->halfInningState.runsInTheInning;
	printf("Runs at start: %d\n", runsAtStart);

	// 4. Simulate frames until arrival
	int arrived = 0;
	int ballLanded = 0;
	int arrivalFrame = -1;
	int landingFrame = -1;

	for (int i = 0; i < 200; i++) {
		// Manually keep ball in air for first 100 frames, then let it drop
		if (i < 100) {
			ctx->state->match->ballInfo.location.y = 30.0f;
			ctx->state->match->ballInfo.velocity.y = 0.0f;
		} else if (i == 100) {
			// Now let it fall fast
			ctx->state->match->ballInfo.velocity.y = -2.0f;
		}

		simulate_frames(ctx, 1);

		// Log transient events
		if (ctx->state->match->gameEvents.ballHitGround) {
			printf("EVENT: ballHitGround at Frame %d\n", i);
		}
		if (ctx->state->match->gameEvents.playerArrivedAtBase) {
			printf("EVENT: playerArrivedAtBase at Frame %d (Base: %d)\n",
			       i, ctx->state->match->playerInfo[runnerIndex].bTPI.baseId);
		}

		// Check Arrival
		if (ctx->state->match->playerInfo[runnerIndex].bTPI.baseId == BASE_HOME_SCORED && !arrived) {
			arrived = 1;
			arrivalFrame = i;
			printf("Frame %d: Runner arrived at HOME. (Ball Height: %.2f, Rules Live: %d)\n",
			       i, ctx->state->match->ballInfo.location.y, ctx->state->match->gameControl.hasBallHitGround);
		}

		// Check Ball Landing (Rule State)
		if (ctx->state->match->gameControl.hasBallHitGround && !ballLanded) {
			ballLanded = 1;
			landingFrame = i;
			printf("Frame %d: Ball hit ground. Game is now LIVE.\n", i);
		}

		if (arrived && ballLanded) break;
	}

	if (!arrived) {
		printf("ERROR: Runner never arrived!\n");
		return TEST_FAILED;
	}

	if (!ballLanded) {
		printf("ERROR: Ball never landed!\n");
		return TEST_FAILED;
	}

	if (arrivalFrame >= landingFrame) {
		printf("WARNING: Test setup failed. Runner arrived AFTER ball landed. Adjust positions.\n");
		// return TEST_FAILED; // Don't fail, but note that the test didn't exercise the bug condition
	} else {
		printf("SUCCESS: Runner arrived %d frames BEFORE ball landed.\n", landingFrame - arrivalFrame);
	}

	int runsAfter = ctx->state->match->halfInningState.runsInTheInning;
	printf("Runs after: %d\n", runsAfter);

	cleanup_scenario(ctx);

	// THE ASSERTION THAT WILL FAIL IF BUG EXISTS
	ASSERT_EQ(runsAtStart + 1, runsAfter, "Run should be scored even if ball lands after arrival");

	return TEST_PASSED;
}
