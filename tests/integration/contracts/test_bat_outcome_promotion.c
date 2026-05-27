#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "referee.h"

/**
 * CONTRACT: The referee promotes ballHitByBat/ballMissedByBat events to
 * betweenPitchState.batOutcome, and clears batOutcome on pitchReleased.
 *
 * This tests the Event → Decision promotion chain (Phase 3 pattern):
 *   ballHitByBat (GameEvents)   → batOutcome = BAT_OUTCOME_HIT
 *   ballMissedByBat (GameEvents) → batOutcome = BAT_OUTCOME_MISSED
 *   pitchReleased (GameEvents)   → batOutcome = BAT_OUTCOME_NONE (cleared)
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

        // Precondition: batOutcome starts clean
        ASSERT_EQ(
            BAT_OUTCOME_NONE, ctx->state->match->betweenPitchState.batOutcome, "Precondition: batOutcome should be NONE"
        );

        // Fire ballHitByBat event
        ctx->state->match->gameEvents.ballHitByBat = 1;

        // Run 1 pipeline frame
        simulate_frames(ctx, 1);

        // Assert: referee promoted to HIT
        ASSERT_EQ(
            BAT_OUTCOME_HIT, ctx->state->match->betweenPitchState.batOutcome,
            "ballHitByBat should promote to BAT_OUTCOME_HIT"
        );

        cleanup_scenario(ctx);
    }

    // === Case 2: MISSED promotion ===
    {
        ScenarioContext* ctx = create_scenario();
        initialize_referee_from_physical_state(ctx);

        // Fire ballMissedByBat event
        ctx->state->match->gameEvents.ballMissedByBat = 1;

        // Run 1 pipeline frame
        simulate_frames(ctx, 1);

        // Assert: referee promoted to MISSED
        ASSERT_EQ(
            BAT_OUTCOME_MISSED, ctx->state->match->betweenPitchState.batOutcome,
            "ballMissedByBat should promote to BAT_OUTCOME_MISSED"
        );

        cleanup_scenario(ctx);
    }

    // === Case 3: Reset on pitchReleased ===
    {
        ScenarioContext* ctx = create_scenario();
        initialize_referee_from_physical_state(ctx);

        // Dirty batOutcome to simulate residue from a previous pitch
        ctx->state->match->betweenPitchState.batOutcome = BAT_OUTCOME_HIT;

        // Fire pitchReleased (signals new pitch cycle)
        ctx->state->match->gameEvents.pitchReleased = 1;

        // Run 1 pipeline frame
        simulate_frames(ctx, 1);

        // Assert: batOutcome was cleared
        ASSERT_EQ(
            BAT_OUTCOME_NONE, ctx->state->match->betweenPitchState.batOutcome,
            "pitchReleased should reset batOutcome to NONE"
        );

        cleanup_scenario(ctx);
    }

    // === Size guard: catch BetweenPitchState struct growth ===
    // 4 fields: catchHasBeenMade (int), hasBallHitGround (int),
    //           pitchResult (PitchResult enum), batOutcome (BatOutcome enum)
    // Update clear_between_pitch_state(), the field checks above, AND this count
    // if BetweenPitchState gains new fields.
    ASSERT_EQ(
        2 * sizeof(int) + sizeof(PitchResult) + sizeof(BatOutcome), sizeof(BetweenPitchState),
        "BetweenPitchState size changed — update clear_between_pitch_state and this test"
    );

    return TEST_PASSED;
}
