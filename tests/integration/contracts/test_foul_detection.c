#include "scenario_builder.h"
#include "tick_outputs.h"
#include "rules_state.h"
#include "test_helpers.h"
#include "all_contracts.h"

/**
 * CONTRACT: When the ball hits the ground out of bounds after a bat hit,
 * the referee transitions foul_state to FOUL_STATE_DETECTED.
 *
 * This tests the Event → Decision chain:
 *   ball_hit_ground + batHit + out of bounds → foul_state = DETECTED
 *
 * Preconditions:
 *   - batHit = 1 (ball came from bat)
 *   - Ball is at an out-of-bounds location
 *   - has_ball_hit_ground = 0 (first bounce)
 *   - foul_state = NONE
 *   - catch_has_been_made = 0
 *   - ball_hit_ground event fires
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
    ctx->state->match->ball.location.x = 0.0f;
    ctx->state->match->ball.location.y = 0.0f;
    ctx->state->match->ball.location.z = 10.0f;
    ctx->state->match->ball.on_ground = 1;
    ctx->state->match->ball.moving = 0;
    ctx->state->match->ball.velocity.x = 0.0f;
    ctx->state->match->ball.velocity.y = 0.0f;
    ctx->state->match->ball.velocity.z = 0.0f;

    // Set contract preconditions
    ctx->state->rules->between_pitch.bat_outcome = BAT_OUTCOME_HIT;
    ctx->state->match->events.ball_hit_ground = 1;

    // Verify preconditions are clean
    ASSERT_EQ(FOUL_STATE_NONE, ctx->state->rules->referee.foul_state, "Precondition: foul_state should be NONE");
    ASSERT_EQ(0, ctx->state->rules->between_pitch.has_ball_hit_ground, "Precondition: has_ball_hit_ground should be 0");

    // Run 1 frame
    simulate_frames(ctx, 1);

    // Assert: referee detected foul play
    ASSERT_EQ(
        FOUL_STATE_DETECTED, ctx->state->rules->referee.foul_state,
        "Foul state should be DETECTED after out-of-bounds hit"
    );

    // Assert: the referee announced it, in the tick it decided it
    ASSERT_EQ(EVENT_OUT_OF_BOUNDS, ctx->state->announcement, "the referee should announce OUT_OF_BOUNDS");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
