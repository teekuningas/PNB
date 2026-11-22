#include "scenario_builder.h"
#include "rules_state.h"
#include "world.h"
#include "test_helpers.h"
#include "all_contracts.h"

/**
 * CONTRACT: Once the referee has decided the inning is ending
 * (end_of_inning_state != NONE), pending runs must NOT be cashed in.
 *
 * This prevents runs from being awarded during the ~200 frame
 * "END OF INNING" display timer.
 *
 * Preconditions:
 *   - end_of_inning_state = DETECTED (timer running)
 *   - A player has has_pending_run = 1
 *   - Ball has hit ground (trigger for resolve_pending_runs)
 *
 * Expected: Run is NOT awarded. Scoreboard unchanged.
 */
int test_no_pending_runs_during_end_of_inning(void)
{
    ScenarioContext* ctx = create_scenario();

    setup_batter_at_home(ctx, 1);
    move_pitcher_away(ctx);
    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);

    // Set up the end-of-inning state (already decided)
    ctx->state->rules->referee.end_of_inning_state = END_INNING_STATE_DETECTED;
    ctx->state->rules->referee.end_of_inning_timer = 50;

    // Set up a pending run ready to be cashed in
    ctx->state->rules->referee.batting_players[0].has_pending_run = 1;
    ctx->state->rules->referee.batting_players[0].base_at_pitch_start = BASE_FIRST;
    ctx->state->match->players[0].batting.presence.base_id = BASE_HOME;
    ctx->state->match->players[0].batting.presence.state = PLAYER_STATE_ON_BASE;

    // Ball has hit ground (trigger for resolve_pending_runs)
    ctx->state->rules->between_pitch.has_ball_hit_ground = 1;
    ctx->state->rules->between_pitch.bat_outcome = BAT_OUTCOME_HIT;

    int runsBefore = ctx->state->rules->scoreboard.teams[0].runs;

    // Run 1 frame
    simulate_frames(ctx, 1);

    // Assert: no run was awarded
    ASSERT_EQ(
        runsBefore, ctx->state->rules->scoreboard.teams[0].runs, "Pending runs must NOT be cashed during end-of-inning"
    );

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

/**
 * CONTRACT: Once the referee has decided the inning is ending,
 * free walk runs must NOT be awarded.
 *
 * Preconditions:
 *   - end_of_inning_state = DETECTED
 *   - free_walk_accepted event fires
 *   - Free walk from 3rd base (would normally score a run)
 *
 * Expected: Run is NOT awarded. Scoreboard unchanged.
 */
int test_no_free_walk_runs_during_end_of_inning(void)
{
    ScenarioContext* ctx = create_scenario();

    setup_batter_at_home(ctx, 1);
    place_runner_at_base(ctx, 0, BASE_THIRD, 0.0f);
    move_pitcher_away(ctx);
    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);

    // Set up the end-of-inning state (already decided)
    ctx->state->rules->referee.end_of_inning_state = END_INNING_STATE_DETECTED;
    ctx->state->rules->referee.end_of_inning_timer = 50;

    // Set up free walk acceptance from 3rd base (would normally score)
    ctx->state->match->events.free_walk_accepted = 1;
    ctx->state->match->free_walk.runner = 0;
    ctx->state->match->free_walk.from = BASE_THIRD;

    int runsBefore = ctx->state->rules->scoreboard.teams[0].runs;

    // Run 1 frame
    simulate_frames(ctx, 1);

    // Assert: no run was awarded
    ASSERT_EQ(
        runsBefore, ctx->state->rules->scoreboard.teams[0].runs,
        "Free walk runs must NOT be awarded during end-of-inning"
    );

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
