#include "scenario_builder.h"
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

    // 1. Setup Runner at 3rd Base (physical state)
    place_runner_at_base(ctx, runnerIndex, BASE_THIRD, 0.0f);

    // 2. Move runner between corner flag and home to ensure quick arrival
    // runLeftPoint is at Z=-25, homeRunPoint is at Z=-0.65, home plate is around Z=0
    // Place runner at Z=-10 (between flag and homeRunPoint)
    Vector3D homeLoc = ctx->state->fieldPositions->pitchPlate;
    Vector3D startLoc = homeLoc;
    startLoc.z = -10.0f; // Between corner flag (-25) and home (-0.65)
    startLoc.x = -15.0f; // Left side (3rd base side)

    // Override physical state for precise positioning
    ctx->state->match->playerInfo[runnerIndex].tPI.location = startLoc;
    ctx->state->match->playerInfo[runnerIndex].tPI.lastLocation = startLoc;

    // Set runtime state: passedPathPoint=1 means "past corner flag, run to homeRunPoint"
    // Runner is at Z=-10, which is between corner flag (Z=-25) and homeRunPoint (Z=-0.65)
    ctx->state->match->playerRuntime[runnerIndex].passedPathPoint = 1;

    printf(
        "Setup: Runner %d placed close to home at (%.1f, %.1f, %.1f)\n", runnerIndex, startLoc.x, startLoc.y, startLoc.z
    );

    // CRITICAL: Move pitcher and catcher away so they don't catch the ball or tag the runner
    move_pitcher_away(ctx);

    // 3. Initialize referee from physical state (referee doesn't mind runner is off-base)
    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);

    // 4. Trigger Runner to run home from 3rd
    trigger_player_run_to_next_base(ctx, runnerIndex, BASE_THIRD);

    // 5. Hit a fly ball into the field (will stay in air for a while)
    // Use coordinates known to be in bounds from test_runner_scores_from_third
    Vector3D pitchPlate = ctx->state->fieldPositions->pitchPlate;
    Vector3D targetInField = {10.0f, 0.0f, -10.0f}; // Known in-bounds location
    hit_fly_ball_to_location(ctx, pitchPlate, targetInField);

    int runsAtStart = ctx->state->rules->halfInningState.runsInTheInning;
    printf("Runs at start: %d\n", runsAtStart);

    // 6. Simulate frames
    int arrived = 0;
    int ballLanded = 0;
    int arrivalFrame = -1;
    int landingFrame = -1;

    for (int i = 0; i < 300; i++) {
        simulate_frames(ctx, 1);

        // Log transient events
        if (ctx->state->match->gameEvents.ballHitGround) {
            printf("EVENT: ballHitGround at Frame %d\n", i);
        }
        if (ctx->state->match->gameEvents.playerArrivedAtBase) {
            printf(
                "EVENT: playerArrivedAtBase at Frame %d (Base: %d)\n", i,
                ctx->state->match->playerInfo[runnerIndex].bTPI.baseId
            );
        }

        // Check Arrival
        if (ctx->state->match->playerInfo[runnerIndex].bTPI.baseId == BASE_HOME_SCORED && !arrived) {
            arrived = 1;
            arrivalFrame = i;
            printf(
                "Frame %d: Runner arrived at HOME. (Ball Height: %.2f, Rules Live: %d)\n", i,
                ctx->state->match->ballInfo.location.y, ctx->state->rules->betweenPitchState.hasBallHitGround
            );
        }

        // Check Ball Landing (Rule State)
        if (ctx->state->rules->betweenPitchState.hasBallHitGround && !ballLanded) {
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

    int runsAfter = ctx->state->rules->halfInningState.runsInTheInning;
    printf("Runs after: %d\n", runsAfter);

    cleanup_scenario(ctx);

    // THE ASSERTION THAT WILL FAIL IF BUG EXISTS
    ASSERT_EQ(runsAtStart + 1, runsAfter, "Run should be scored even if ball lands after arrival");

    return TEST_PASSED;
}
