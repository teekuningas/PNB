#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "referee.h"

/**
 * CONTRACT: When the ball hits the ground out of bounds after a bat hit,
 * the referee transitions foulState to FOUL_STATE_DETECTED.
 *
 * This tests the Event → Decision chain:
 *   ballHitGround + batHit + out of bounds → foulState = DETECTED
 *
 * Preconditions:
 *   - batHit = 1 (ball came from bat)
 *   - Ball is at an out-of-bounds location
 *   - hasBallHitGround = 0 (first bounce)
 *   - foulState = NONE
 *   - catchHasBeenMade = 0
 *   - ballHitGround event fires
 *
 * This contract is directly relevant to Phase 3 (batHit migration):
 * the referee's foul detection reads batHit. After Phase 3 consolidates
 * batHit into GameEvents, this test proves the contract still holds.
 */
int test_foul_detected_on_out_of_bounds_hit(void)
{
    ScenarioContext* ctx = create_scenario();

    setup_batter_at_home(ctx, 1);
    move_pitcher_away(ctx);

    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);

    // Place ball clearly behind home line (out of bounds)
    // HOME_LINE_Z = -0.65f, so z = 10.0f is far behind home plate
    ctx->state->match->ballInfo.location.x = 0.0f;
    ctx->state->match->ballInfo.location.y = 0.0f;
    ctx->state->match->ballInfo.location.z = 10.0f;
    ctx->state->match->ballInfo.onGround = 1;
    ctx->state->match->ballInfo.moving = 0;
    ctx->state->match->ballInfo.velocity.x = 0.0f;
    ctx->state->match->ballInfo.velocity.y = 0.0f;
    ctx->state->match->ballInfo.velocity.z = 0.0f;

    // Set contract preconditions
    ctx->state->match->betweenPitchState.batOutcome = BAT_OUTCOME_HIT;
    ctx->state->match->gameEvents.ballHitGround = 1;

    // Verify preconditions are clean
    ASSERT_EQ(FOUL_STATE_NONE, ctx->state->match->referee.foulState, "Precondition: foulState should be NONE");
    ASSERT_EQ(0, ctx->state->match->betweenPitchState.hasBallHitGround, "Precondition: hasBallHitGround should be 0");

    // Run 1 frame
    simulate_frames(ctx, 1);

    // Assert: referee detected foul play
    ASSERT_EQ(
        FOUL_STATE_DETECTED, ctx->state->match->referee.foulState,
        "Foul state should be DETECTED after out-of-bounds hit"
    );

    // Assert: the event was recorded
    ASSERT_EQ(
        EVENT_OUT_OF_BOUNDS, ctx->state->match->halfInningState.event, "Half-inning event should be OUT_OF_BOUNDS"
    );

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
