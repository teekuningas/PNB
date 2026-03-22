#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"

/**
 * CONTRACT: When pitchReleased fires, the referee snapshots the current
 * state as the legal baseline for this pitch.
 *
 * This tests the Event → Snapshot chain:
 *   pitchReleased (GameEvents) → baseAtPitchStart captured
 *                               → strikesAtPitchStart captured
 *                               → betweenPitchState cleared
 *                               → pending run flags cleared
 *
 * This snapshot is critical: it defines the "legal baseline" for the pitch.
 * Wounding, safety, and run-of-honor all compare against pitch-start state.
 */
int test_referee_snapshots_on_pitch_released(void)
{
    ScenarioContext* ctx = create_scenario();

    // Place runner at second base, batter at home
    place_runner_at_base(ctx, 0, BASE_SECOND, 0.0f);
    setup_batter_at_home(ctx, 1);
    move_pitcher_away(ctx);

    initialize_referee_from_physical_state(ctx);

    // Set up a non-zero strike count (simulating mid at-bat)
    ctx->state->match->halfInningState.strikes = 1;

    // Dirty the betweenPitchState (simulating residue from previous pitch)
    ctx->state->match->betweenPitchState.catchHasBeenMade = 1;
    ctx->state->match->betweenPitchState.hasBallHitGround = 1;
    ctx->state->match->betweenPitchState.batOutcome = BAT_OUTCOME_HIT;

    // Fire pitchReleased
    ctx->state->match->gameEvents.pitchReleased = 1;

    // Run 1 frame — referee should snapshot everything
    simulate_frames(ctx, 1);

    // Assert: baseAtPitchStart captured for the runner at second
    ASSERT_EQ(
        BASE_SECOND, ctx->state->match->referee.battingPlayers[0].baseAtPitchStart,
        "Runner's pitch-start base should be captured"
    );

    // Assert: batter's pitch-start base is home
    ASSERT_EQ(
        BASE_HOME, ctx->state->match->referee.battingPlayers[1].baseAtPitchStart,
        "Batter's pitch-start base should be HOME"
    );

    // Assert: strikesAtPitchStart captured
    ASSERT_EQ(1, ctx->state->match->referee.strikesAtPitchStart, "Strike count at pitch start should be captured");

    // Assert: betweenPitchState was cleared (fresh for this pitch)
    ASSERT_EQ(
        0, ctx->state->match->betweenPitchState.catchHasBeenMade, "catchHasBeenMade should be cleared on new pitch"
    );
    ASSERT_EQ(
        0, ctx->state->match->betweenPitchState.hasBallHitGround, "hasBallHitGround should be cleared on new pitch"
    );
    ASSERT_EQ(
        BAT_OUTCOME_NONE, ctx->state->match->betweenPitchState.batOutcome, "batOutcome should be cleared on new pitch"
    );

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
