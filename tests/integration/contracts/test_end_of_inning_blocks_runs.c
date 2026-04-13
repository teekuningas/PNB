#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "referee.h"

/**
 * CONTRACT: Once the referee has decided the inning is ending
 * (endOfInningState != NONE), pending runs must NOT be cashed in.
 *
 * This prevents runs from being awarded during the ~200 frame
 * "END OF INNING" display timer.
 *
 * Preconditions:
 *   - endOfInningState = DETECTED (timer running)
 *   - A player has hasPendingRun = 1
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
    ctx->state->match->referee.endOfInningState = END_INNING_STATE_DETECTED;
    ctx->state->match->referee.endInningTimer = 50;

    // Set up a pending run ready to be cashed in
    ctx->state->match->referee.battingPlayers[0].hasPendingRun = 1;
    ctx->state->match->referee.battingPlayers[0].baseAtPitchStart = BASE_FIRST;
    ctx->state->match->playerInfo[0].bTPI.baseId = BASE_HOME;
    ctx->state->match->playerInfo[0].bTPI.state = PLAYER_STATE_ON_BASE;

    // Ball has hit ground (trigger for resolve_pending_runs)
    ctx->state->match->betweenPitchState.hasBallHitGround = 1;
    ctx->state->match->betweenPitchState.batOutcome = BAT_OUTCOME_HIT;

    int runsBefore = ctx->state->match->scoreboard.teams[0].runs;

    // Run 1 frame
    simulate_frames(ctx, 1);

    // Assert: no run was awarded
    ASSERT_EQ(
        runsBefore, ctx->state->match->scoreboard.teams[0].runs, "Pending runs must NOT be cashed during end-of-inning"
    );

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

/**
 * CONTRACT: Once the referee has decided the inning is ending,
 * free walk runs must NOT be awarded.
 *
 * Preconditions:
 *   - endOfInningState = DETECTED
 *   - freeWalkAccepted event fires
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
    ctx->state->match->referee.endOfInningState = END_INNING_STATE_DETECTED;
    ctx->state->match->referee.endInningTimer = 50;

    // Set up free walk acceptance from 3rd base (would normally score)
    ctx->state->match->gameEvents.freeWalkAccepted = 1;
    ctx->state->match->flowControl.freeWalkIndex = 0;
    ctx->state->match->flowControl.freeWalkBase = BASE_THIRD;

    int runsBefore = ctx->state->match->scoreboard.teams[0].runs;

    // Run 1 frame
    simulate_frames(ctx, 1);

    // Assert: no run was awarded
    ASSERT_EQ(
        runsBefore, ctx->state->match->scoreboard.teams[0].runs,
        "Free walk runs must NOT be awarded during end-of-inning"
    );

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
