#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include <stdio.h>
#include <math.h>

/**
 * TEST: Out of Bounds Reset
 *
 * Scenario:
 * 1. Runner at 1st Base.
 * 2. Batter hits a ball very far out of bounds (foul area).
 * 3. Runner advances to 2nd Base and gains safety while ball is in air.
 * 4. Ball lands out of bounds.
 * 5. Game detects Out of Bounds.
 * 6. Game resets positions (Foul Play).
 * 7. Runner should be returned to 1st Base (original base at pitch start).
 */
int test_full_out_of_bounds_reset(void)
{
    ScenarioContext* ctx = create_scenario();

    // 1. Setup PHYSICAL state: Runner at 1st Base, 70% toward 2nd
    // This ensures they reach 2nd base before the ball (150 frames flight) lands.
    place_runner_at_base(ctx, 0, BASE_FIRST, 0.7f);

    // Setup batter just so game state is valid
    setup_batter_at_home(ctx, 1);

    // 2. Initialize referee from physical state
    initialize_referee_from_physical_state(ctx);

    // 3. Emit pitchReleased to snapshot baseAtPitchStart
    snapshot_pitch_start_state(ctx);

    // 4. Position fielders away
    move_pitcher_away(ctx);

    printf(
        "[TEST] Runner 0 Start: baseId=%d, currentSafety=%d\n", ctx->state->match->playerInfo[0].bTPI.baseId,
        ctx->state->rules->referee.battingPlayers[0].currentSafetyBase
    );

    // 5. Hit ball far out of bounds
    // Target: (200, 0, 200) is likely out of bounds
    Vector3D home = ctx->state->fieldPositions->pitchPlate;
    Vector3D foulTarget = {200.0f, 0.0f, 200.0f};

    // We use hit_fly_ball to put it in the air with a high arc (long flight time)
    hit_fly_ball_to_location(ctx, home, foulTarget);

    printf("[TEST] Ball hit towards (%.1f, %.1f, %.1f).\n", foulTarget.x, foulTarget.y, foulTarget.z);

    // 6. Runner runs to 2nd Base
    trigger_player_run_to_next_base(ctx, 0, BASE_FIRST);
    printf("[TEST] Runner 0 triggered to run to 2nd Base.\n");

    int reachedSecond = 0;
    int outOfBoundsDetected = 0;
    int resetDetected = 0;

    // Simulate loop
    for (int frame = 0; frame < 1500; frame++) {
        simulate_frames(ctx, 1);

        PlayerInfo* runner = &ctx->state->match->playerInfo[0];

        // Check if runner reached 2nd base
        if (!reachedSecond && runner->bTPI.baseId == BASE_SECOND) {
            int currentSafety = ctx->state->rules->referee.battingPlayers[0].currentSafetyBase;
            if (currentSafety == BASE_SECOND) {
                reachedSecond = 1;
                printf(
                    "[TEST] Frame %d: Runner 0 reached 2nd Base. baseId=%d, currentSafety=%d (SAFE)\n", frame,
                    runner->bTPI.baseId, currentSafety
                );
            } else {
                // Log even if safety not yet granted
                static int lastLogFrame = -1;
                if (frame != lastLogFrame) {
                    printf(
                        "[TEST] Frame %d: Runner 0 at 2nd Base. baseId=%d, currentSafety=%d (Waiting for safety...)\n",
                        frame, runner->bTPI.baseId, currentSafety
                    );
                    lastLogFrame = frame;
                }
            }
        }

        // Check for Out of Bounds Event
        if (!outOfBoundsDetected && ctx->state->rules->referee.foulState != FOUL_STATE_NONE) {
            outOfBoundsDetected = 1;
            printf("[TEST] Frame %d: Out of Bounds declared! Ball has landed.\n", frame);
        }

        // Check for Reset (Runner returned to 1st base)
        // Reset happens when outOfBounds flag is cleared? Or just players moved?
        // "Foul Play" usually resets players to baseAtPitchStart.
        if (outOfBoundsDetected && runner->bTPI.baseId == BASE_FIRST) {
            int currentSafety = ctx->state->rules->referee.battingPlayers[0].currentSafetyBase;
            // Ensure it's not just a glitch, but he is SAFE there
            if (currentSafety == BASE_FIRST) {
                resetDetected = 1;
                printf(
                    "[TEST] Frame %d: Runner 0 returned to 1st Base. baseId=%d, currentSafety=%d (Reset successful)\n",
                    frame, runner->bTPI.baseId, currentSafety
                );
                break;
            }
        }
    }

    cleanup_scenario(ctx);

    ASSERT_EQ(1, reachedSecond, "Runner should have reached 2nd Base before ball landed");
    ASSERT_EQ(1, outOfBoundsDetected, "Game should have declared Out of Bounds");
    ASSERT_EQ(1, resetDetected, "Runner should be returned to 1st Base after reset");

    return TEST_PASSED;
}
