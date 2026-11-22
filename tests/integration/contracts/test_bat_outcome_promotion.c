#include "scenario_builder.h"
#include "rules_state.h"
#include "test_helpers.h"
#include "all_contracts.h"

/**
 * CONTRACT: The referee promotes ball_hit_by_bat/ball_missed_by_bat events to
 * between_pitch.bat_outcome, and clears bat_outcome on pitch_released.
 *
 * This tests the Event → Decision promotion chain (Phase 3 pattern):
 *   ball_hit_by_bat (GameEvents)   → bat_outcome = BAT_OUTCOME_HIT
 *   ball_missed_by_bat (GameEvents) → bat_outcome = BAT_OUTCOME_MISSED
 *   pitch_released (GameEvents)   → bat_outcome = BAT_OUTCOME_NONE (cleared)
 *
 * Also includes a sizeof guard for BetweenPitchState to catch struct growth
 * (same pattern as test_clear_frame_events_completeness uses for GameEvents).
 */
int test_bat_outcome_promotion(void)
{
    // === Case 1: HIT promotion ===
    {
        ScenarioContext* ctx = create_scenario();
        initialize_referee_from_physical_state(ctx);

        // Precondition: bat_outcome starts clean
        ASSERT_EQ(
            BAT_OUTCOME_NONE, ctx->state->rules->between_pitch.bat_outcome, "Precondition: bat_outcome should be NONE"
        );

        // Fire ball_hit_by_bat event
        ctx->state->match->events.ball_hit_by_bat = 1;

        // Run 1 pipeline frame
        simulate_frames(ctx, 1);

        // Assert: referee promoted to HIT
        ASSERT_EQ(
            BAT_OUTCOME_HIT, ctx->state->rules->between_pitch.bat_outcome,
            "ball_hit_by_bat should promote to BAT_OUTCOME_HIT"
        );

        cleanup_scenario(ctx);
    }

    // === Case 2: MISSED promotion ===
    {
        ScenarioContext* ctx = create_scenario();
        initialize_referee_from_physical_state(ctx);

        // Fire ball_missed_by_bat event
        ctx->state->match->events.ball_missed_by_bat = 1;

        // Run 1 pipeline frame
        simulate_frames(ctx, 1);

        // Assert: referee promoted to MISSED
        ASSERT_EQ(
            BAT_OUTCOME_MISSED, ctx->state->rules->between_pitch.bat_outcome,
            "ball_missed_by_bat should promote to BAT_OUTCOME_MISSED"
        );

        cleanup_scenario(ctx);
    }

    // === Case 3: Reset on pitch_released ===
    {
        ScenarioContext* ctx = create_scenario();
        initialize_referee_from_physical_state(ctx);

        // Dirty bat_outcome to simulate residue from a previous pitch
        ctx->state->rules->between_pitch.bat_outcome = BAT_OUTCOME_HIT;

        // Fire pitch_released (signals new pitch cycle)
        ctx->state->match->events.pitch_released = 1;

        // Run 1 pipeline frame
        simulate_frames(ctx, 1);

        // Assert: bat_outcome was cleared
        ASSERT_EQ(
            BAT_OUTCOME_NONE, ctx->state->rules->between_pitch.bat_outcome,
            "pitch_released should reset bat_outcome to NONE"
        );

        cleanup_scenario(ctx);
    }

    // === Size guard: catch BetweenPitchState struct growth ===
    // 4 fields: catch_has_been_made (int), has_ball_hit_ground (int),
    //           pitch_result (PitchResult enum), bat_outcome (BatOutcome enum)
    // Update clear_between_pitch_state(), the field checks above, AND this count
    // if BetweenPitchState gains new fields.
    ASSERT_EQ(
        2 * sizeof(int) + sizeof(PitchResult) + sizeof(BatOutcome), sizeof(BetweenPitchState),
        "BetweenPitchState size changed — update clear_between_pitch_state and this test"
    );

    return TEST_PASSED;
}
