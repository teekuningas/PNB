#include "../scenario_builder.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include <stdio.h>

/**
 * TEST: Runner arrives at home BEFORE ball is caught.
 *
 * Scenario:
 * 1. Runner starts at 3rd base.
 * 2. Batter hits a fly ball that will be caught.
 * 3. Runner runs home and arrives quickly.
 * 4. Ball is caught AFTER the runner has arrived.
 *
 * Expected Behavior:
 * The run should NOT count. The runner is wounded by the catch even though they
 * physically arrived before the catch was made. The pending run should be voided
 * when the catch is confirmed.
 */
int test_run_arrival_before_catch(void)
{
    printf("\n=== TEST: Run Arrival Before Catch (No Run) ===\n");
    ScenarioContext* ctx = create_scenario();

    int runnerIndex = 1; // Different from batter

    // 1. Setup Runner at 3rd Base (physical state)
    place_runner_at_base(ctx, runnerIndex, BASE_THIRD, 0.0f);

    // 2. Move runner between corner flag and home to ensure quick arrival
    Vector3D homeLoc = ctx->state->fieldPositions->pitchPlate;
    Vector3D startLoc = homeLoc;
    startLoc.z = -10.0f; // Between corner flag and homeRunPoint
    startLoc.x = -15.0f; // Left side (3rd base side)

    // Override physical state for precise positioning
    ctx->state->match->playerInfo[runnerIndex].tPI.location = startLoc;
    ctx->state->match->playerInfo[runnerIndex].tPI.lastLocation = startLoc;

    // Set runtime state: passedPathPoint=1 means "past corner flag, run to homeRunPoint"
    ctx->state->match->playerRuntime[runnerIndex].passedPathPoint = 1;

    printf("Setup: Runner %d placed close to home\n", runnerIndex);

    // CRITICAL: Move pitcher and catcher away
    move_pitcher_away(ctx);

    // 3. Position a fielder at first base to catch the ball
    int fielderIdx = 14;
    Vector3D fielderLoc = ctx->state->fieldPositions->firstBase;
    ctx->state->match->playerInfo[fielderIdx].tPI.location = fielderLoc;
    ctx->state->match->playerInfo[fielderIdx].tPI.homeLocation = fielderLoc;

    // 4. Initialize referee from physical state (referee doesn't mind runner is off-base)
    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);

    // 5. Trigger Runner to run home from 3rd
    trigger_player_run_to_next_base(ctx, runnerIndex, BASE_THIRD);

    // 6. Hit a fly ball to first base (will be caught)
    Vector3D pitchPlate = ctx->state->fieldPositions->pitchPlate;
    hit_fly_ball_to_location(ctx, pitchPlate, fielderLoc);

    int runsAtStart = ctx->state->match->halfInningState.runsInTheInning;
    printf("Runs at start: %d\n", runsAtStart);

    // 5. Simulate frames until both arrival and catch
    int arrived = 0;
    int ballCaught = 0;
    int woundingStarted = 0;
    int woundingFinished = 0;
    int runnerWounded = 0;
    int pendingRunSet = 0;
    int pendingRunCleared = 0;
    int arrivalFrame = -1;
    int catchFrame = -1;
    int woundingStartFrame = -1;
    int woundingFinishFrame = -1;
    int woundFrame = -1;
    int pendingRunSetFrame = -1;
    int pendingRunClearedFrame = -1;

    for (int i = 0; i < 600; i++) {
        // Check pending run state BEFORE simulation
        int hadPendingRun = ctx->state->match->referee.battingPlayers[runnerIndex].hasPendingRun;
        int hadPendingWound =
            ctx->state->match->referee.battingPlayers[runnerIndex].status >= PLAYER_STATUS_WOUND_MARKED;

        simulate_frames(ctx, 1);

        // Check pending run state AFTER simulation
        int hasPendingRunNow = ctx->state->match->referee.battingPlayers[runnerIndex].hasPendingRun;
        int hasPendingWoundNow =
            ctx->state->match->referee.battingPlayers[runnerIndex].status >= PLAYER_STATUS_WOUND_MARKED;

        // Detect pending run transitions
        if (!hadPendingRun && hasPendingRunNow && !pendingRunSet) {
            pendingRunSet = 1;
            pendingRunSetFrame = i;
            printf("Frame %d: hasPendingRun: 0 -> 1 (PENDING RUN SET)\n", i);
        }

        if (hadPendingRun && !hasPendingRunNow && !pendingRunCleared) {
            pendingRunCleared = 1;
            pendingRunClearedFrame = i;
            printf("Frame %d: hasPendingRun: 1 -> 0 (PENDING RUN CLEARED)\n", i);
        }

        // Detect pending wound transitions
        if (!hadPendingWound && hasPendingWoundNow) {
            printf("Frame %d: hasPendingWound: 0 -> 1 (WOUND MARKED)\n", i);
        }

        // Check Arrival
        if (ctx->state->match->playerInfo[runnerIndex].bTPI.baseId == BASE_HOME_SCORED && !arrived) {
            arrived = 1;
            arrivalFrame = i;
            printf(
                "Frame %d: Runner arrived at HOME (baseId=4, state=%d)\n", i,
                ctx->state->match->playerInfo[runnerIndex].bTPI.state
            );
        }

        // Check Catch
        if (ctx->state->match->pII.hasBallIndex != -1 && !ballCaught) {
            ballCaught = 1;
            catchFrame = i;
            printf("Frame %d: Ball caught by fielder %d\n", i, ctx->state->match->pII.hasBallIndex);
        }

        // Check wounding evaluation started
        if (ctx->state->match->referee.woundingEvaluationActive && !woundingStarted) {
            woundingStarted = 1;
            woundingStartFrame = i;
            printf(
                "Frame %d: Wounding evaluation STARTED (timer=%d, active=%d)\n", i,
                ctx->state->match->referee.woundingEvaluationTimer, ctx->state->match->referee.woundingEvaluationActive
            );
        }

        // Check wounding evaluation finished
        if (ctx->state->match->referee.woundingEvaluationFinished && !woundingFinished) {
            woundingFinished = 1;
            woundingFinishFrame = i;
            printf(
                "Frame %d: Wounding evaluation FINISHED (finished=%d)\n", i,
                ctx->state->match->referee.woundingEvaluationFinished
            );
            printf(
                "  Runner state=%d, hasPendingWound=%d, hasPendingRun=%d\n",
                ctx->state->match->playerInfo[runnerIndex].bTPI.state,
                ctx->state->match->referee.battingPlayers[runnerIndex].status >= PLAYER_STATUS_WOUND_MARKED,
                ctx->state->match->referee.battingPlayers[runnerIndex].hasPendingRun
            );
            printf(
                "  Runner baseId=%d, baseAtPitchStart=%d, status=%d\n",
                ctx->state->match->playerInfo[runnerIndex].bTPI.baseId,
                ctx->state->match->referee.battingPlayers[runnerIndex].baseAtPitchStart,
                ctx->state->match->referee.battingPlayers[runnerIndex].status
            );
        }

        // Check Runner wounded
        if (ctx->state->match->playerInfo[runnerIndex].bTPI.state == PLAYER_STATE_WOUNDED && !runnerWounded) {
            runnerWounded = 1;
            woundFrame = i;
            printf(
                "Frame %d: Runner WOUNDED (state=%d, baseId=%d)\n", i,
                ctx->state->match->playerInfo[runnerIndex].bTPI.state,
                ctx->state->match->playerInfo[runnerIndex].bTPI.baseId
            );
        }

        // Continue for a while after all events to let everything settle
        if (arrived && ballCaught && woundingFinished) {
            // Give it more time to see if wounding happens
            if (i > woundingFinishFrame + 100) {
                break;
            }
        }
    }

    if (!arrived) {
        printf("ERROR: Runner never arrived!\n");
        cleanup_scenario(ctx);
        return TEST_FAILED;
    }

    if (!ballCaught) {
        printf("ERROR: Ball was never caught!\n");
        cleanup_scenario(ctx);
        return TEST_FAILED;
    }

    if (arrivalFrame >= catchFrame) {
        printf("WARNING: Test setup issue - runner arrived AFTER catch\n");
        printf("  Arrival: frame %d, Catch: frame %d\n", arrivalFrame, catchFrame);
    } else {
        printf("SUCCESS: Runner arrived %d frames BEFORE ball was caught\n", catchFrame - arrivalFrame);
    }

    printf("\n=== EVENT TIMELINE ===\n");
    printf("Frame %d: Runner arrived at home\n", arrivalFrame);
    if (pendingRunSet) {
        printf(
            "Frame %d: Pending run SET (delta: %+d frames from arrival)\n", pendingRunSetFrame,
            pendingRunSetFrame - arrivalFrame
        );
    } else {
        printf("ERROR: Pending run was NEVER set!\n");
    }
    printf("Frame %d: Ball caught (delta: %+d frames from arrival)\n", catchFrame, catchFrame - arrivalFrame);
    if (woundingStarted) {
        printf(
            "Frame %d: Wounding evaluation started (delta: %+d frames from catch)\n", woundingStartFrame,
            woundingStartFrame - catchFrame
        );
    }
    if (woundingFinished) {
        printf(
            "Frame %d: Wounding evaluation finished (delta: %+d frames from start)\n", woundingFinishFrame,
            woundingFinishFrame - woundingStartFrame
        );
    }
    if (pendingRunCleared) {
        printf(
            "Frame %d: Pending run CLEARED (delta: %+d frames from evaluation finish)\n", pendingRunClearedFrame,
            pendingRunClearedFrame - woundingFinishFrame
        );
    } else {
        printf("ERROR: Pending run was NEVER cleared!\n");
    }
    if (runnerWounded) {
        printf(
            "Frame %d: Runner wounded (delta: %+d frames from evaluation finish)\n", woundFrame,
            woundFrame - woundingFinishFrame
        );
    } else {
        printf("WARNING: Runner was NEVER wounded!\n");
    }
    printf("======================\n\n");

    int runsAfter = ctx->state->match->halfInningState.runsInTheInning;
    printf("Runs after: %d (expected: 0)\n", runsAfter);

    // Check final runner state
    int finalWounded = (ctx->state->match->playerInfo[runnerIndex].bTPI.state == PLAYER_STATE_WOUNDED);
    printf("Runner wounded (final): %d (expected: 1)\n", finalWounded);
    printf("Wounding evaluation finished: %d\n", woundingFinished);

    cleanup_scenario(ctx);

    // THE ASSERTION: No run should be scored because runner was wounded by catch
    ASSERT_EQ(runsAtStart, runsAfter, "Run should NOT be scored - runner wounded by catch");

    return TEST_PASSED;
}
