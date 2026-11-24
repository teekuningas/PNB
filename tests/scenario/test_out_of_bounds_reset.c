#include "scenario_builder.h"
#include "rules_state.h"
#include "intents.h"
#include "intent_channel.h"
#include "world.h"
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
int test_out_of_bounds_reset(void)
{
    ScenarioContext* ctx = create_scenario();

    // 1. Setup PHYSICAL state: Runner at 1st Base, 70% toward 2nd
    // This ensures they reach 2nd base before the ball (150 frames flight) lands.
    place_runner_at_base(ctx, 0, BASE_FIRST, 0.7f);

    // Setup batter just so game state is valid
    setup_batter_at_home(ctx, 1);

    // 2. Initialize referee from physical state
    initialize_referee_from_physical_state(ctx);

    // 3. Emit pitch_released to snapshot base_at_pitch_start
    snapshot_pitch_start_state(ctx);

    // 4. Position fielders away
    move_pitcher_away(ctx);

    printf(
        "[TEST] Runner 0 Start: baseId=%d, currentSafety=%d\n", ctx->state->match->players[0].batting.presence.base_id,
        ctx->state->rules->referee.batting_players[0].current_safety_base
    );

    // 5. Hit ball far out of bounds
    // Target: (200, 0, 200) is likely out of bounds
    Vector3D home = ctx->state->field_positions->pitch_plate;
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

        PlayerInfo* runner = &ctx->state->match->players[0];

        // Check if runner reached 2nd base
        if (!reachedSecond && runner->batting.presence.base_id == BASE_SECOND) {
            int currentSafety = ctx->state->rules->referee.batting_players[0].current_safety_base;
            if (currentSafety == BASE_SECOND) {
                reachedSecond = 1;
                printf(
                    "[TEST] Frame %d: Runner 0 reached 2nd Base. baseId=%d, currentSafety=%d (SAFE)\n", frame,
                    runner->batting.presence.base_id, currentSafety
                );
            } else {
                // Log even if safety not yet granted
                static int lastLogFrame = -1;
                if (frame != lastLogFrame) {
                    printf(
                        "[TEST] Frame %d: Runner 0 at 2nd Base. baseId=%d, currentSafety=%d (Waiting for safety...)\n",
                        frame, runner->batting.presence.base_id, currentSafety
                    );
                    lastLogFrame = frame;
                }
            }
        }

        // Check for Out of Bounds Event
        if (!outOfBoundsDetected && ctx->state->rules->referee.foul_state != FOUL_STATE_NONE) {
            outOfBoundsDetected = 1;
            printf("[TEST] Frame %d: Out of Bounds declared! Ball has landed.\n", frame);
        }

        // Check for Reset (Runner returned to 1st base)
        // Reset happens when outOfBounds flag is cleared? Or just players moved?
        // "Foul Play" usually resets players to base_at_pitch_start.
        if (outOfBoundsDetected && runner->batting.presence.base_id == BASE_FIRST) {
            int currentSafety = ctx->state->rules->referee.batting_players[0].current_safety_base;
            // Ensure it's not just a glitch, but he is SAFE there
            if (currentSafety == BASE_FIRST) {
                resetDetected = 1;
                printf(
                    "[TEST] Frame %d: Runner 0 returned to 1st Base. baseId=%d, currentSafety=%d (Reset successful)\n",
                    frame, runner->batting.presence.base_id, currentSafety
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

/**
 * §32 + foul reset: once the turn's first pitch has left the hand, a 1st-strike foul resets the
 * same batter to home with their right to leave home intact — they may commit to run before pitch 2.
 */
int test_batter_may_leave_home_after_foul_reset(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    GameRulesState* r = ctx->state->rules;

    setup_batter_at_home(ctx, 1);
    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);
    move_pitcher_away(ctx);

    // Hit a 1st-strike foul ball far out of bounds.
    Vector3D home = ctx->state->field_positions->pitch_plate;
    Vector3D foulTarget = {200.0f, 0.0f, 200.0f};
    hit_fly_ball_to_location(ctx, home, foulTarget);
    m->events.ball_hit_by_bat = 1;

    int sawFoul = 0;
    int resetCompleted = 0;
    for (int frame = 0; frame < 600; frame++) {
        simulate_frames(ctx, 1);
        if (r->referee.foul_state == FOUL_STATE_DETECTED) {
            sawFoul = 1;
        } else if (sawFoul && r->referee.foul_state == FOUL_STATE_NONE) {
            resetCompleted = 1;
            break;
        }
    }

    ASSERT_EQ(1, sawFoul, "foul must be detected");
    ASSERT_EQ(1, resetCompleted, "foul reset must complete");
    ASSERT_EQ(1, r->half_inning.strikes, "strike count is 1 after 1st-strike foul");
    ASSERT_EQ(1, r->half_inning.batter_may_leave_home, "§32 right persists across 1st-strike foul reset");
    ASSERT_EQ(PLAYER_STATE_AT_BAT, (int)m->players[1].batting.presence.state, "batter resumes AT_BAT");

    // Before any 2nd pitch is thrown, the continuing batter commits to run through the INGEST gate.
    intent_push(
        &ctx->state->channels.batting,
        (IntentMessage){.kind = INTENT_BASE_RUN, .as.base_run = {.base = BASE_HOME, .command = RUN_COMMIT}}
    );
    simulate_frames(ctx, 1);

    ASSERT_EQ(
        PLAYER_STATE_RUNNING, (int)m->players[1].batting.presence.state,
        "continuing batter may leave home after foul reset before pitch 2"
    );
    ASSERT_EQ(1, m->players[1].path.going_forward, "continuing batter runs forward toward 1st");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
