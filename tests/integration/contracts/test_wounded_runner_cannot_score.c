#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "player_utils.h"

/**
 * CONTRACT: A runner with referee status >= WOUND_MARKED cannot score a run,
 * even if their physical state hasn't been consolidated to WOUNDED yet.
 *
 * This is the regression test for Bug #9 (wounded runner scores from 3rd).
 *
 * Root cause: update_runs Case B checked physical bTPI.state == WOUNDED,
 * but consolidation (which sets physical state) runs AFTER the referee.
 * A WOUND_PENDING runner arriving at home had ON_BASE physical state
 * → is_regular_run returned true → illegitimate run scored.
 *
 * Fix: Also check referee->battingPlayers[i].status >= WOUND_MARKED.
 *
 * Scenario:
 *   1. Runner physically at HOME_SCORED with ON_BASE physical state
 *      (consolidation hasn't run yet to set WOUNDED)
 *   2. Referee status = WOUND_PENDING (doomed after catch)
 *   3. woundingEvaluationFinished = 1, catchHasBeenMade = 1
 *   4. update_runs Case B fires → must NOT score a run
 */
int test_wounded_runner_cannot_score_run(void)
{
    ScenarioContext* ctx = create_scenario();

    // Place runner 0 at home_scored position physically (simulating arrival)
    // with referee status WOUND_PENDING (doomed but not yet physically wounded)
    ctx->state->match->playerInfo[0].bTPI.state = PLAYER_STATE_ON_BASE;
    ctx->state->match->playerInfo[0].bTPI.baseId = BASE_HOME_SCORED;
    ctx->state->match->playerInfo[0].tPI.location = ctx->state->fieldPositions->pitchPlate;

    // Place batter at home (inactive for this test)
    setup_batter_at_home(ctx, 1);
    move_pitcher_away(ctx);

    // Initialize referee and snapshot
    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);

    // Set runner's pitch-start base to HOME (so HOME_SCORED would be a valid "run" path)
    ctx->state->rules->referee.battingPlayers[0].baseAtPitchStart = BASE_HOME;

    // Key state: referee already decided this runner is doomed
    ctx->state->rules->referee.battingPlayers[0].status = PLAYER_STATUS_WOUND_PENDING;

    // Wounding evaluation is finished (timer expired, all marked)
    ctx->state->rules->referee.woundingEvaluationFinished = 1;
    ctx->state->rules->referee.woundingEvaluationActive = 0;

    // Ball was caught (this is WHY they're wounded)
    ctx->state->rules->betweenPitchState.catchHasBeenMade = 1;
    ctx->state->rules->betweenPitchState.hasBallHitGround = 0;
    ctx->state->rules->betweenPitchState.batOutcome = BAT_OUTCOME_HIT;

    // A fielder has the ball
    int fielderIdx = 15;
    ctx->state->match->pII.hasBallIndex = fielderIdx;
    ctx->state->match->playerInfo[fielderIdx].tPI.location = ctx->state->fieldPositions->secondBase;

    // Record runs before
    int battingTeamIndex = get_batting_team_index(&ctx->state->rules->scoreboard);
    int runsBefore = ctx->state->rules->scoreboard.teams[battingTeamIndex].runs;

    // Run 1 frame — update_runs Case B should fire but NOT award a run
    simulate_frames(ctx, 1);

    // Assert: no run scored
    int runsAfter = ctx->state->rules->scoreboard.teams[battingTeamIndex].runs;
    ASSERT_EQ(runsBefore, runsAfter, "Wounded runner (WOUND_PENDING) must not score a run");

    // Assert: runner was NOT marked as having scored
    ASSERT_EQ(
        0, ctx->state->rules->referee.battingPlayers[0].hasScored, "Wounded runner should not have hasScored set"
    );

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
