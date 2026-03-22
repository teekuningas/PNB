#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"

/**
 * CONTRACT: When catchMade fires and a runner has advanced past their
 * pitch-start base, the referee starts wounding evaluation and marks
 * the vulnerable runner as WOUND_MARKED.
 *
 * This tests the Event → Decision chain:
 *   catchMade (GameEvents) → woundingEvaluationActive (RefereeState)
 *                          → WOUND_MARKED (per-player status)
 *
 * Preconditions:
 *   - Runner 0 at second base, but baseAtPitchStart was first (advanced)
 *   - batHit = 1 (ball came from bat)
 *   - Ball not yet hit ground (hasBallHitGround = 0)
 *   - No prior catch this pitch (catchHasBeenMade = 0)
 *   - catchMade event fires
 *
 * Expected: Referee starts evaluation, runner marked WOUND_MARKED.
 */
int test_referee_starts_wounding_on_catch(void)
{
    ScenarioContext* ctx = create_scenario();

    // Place runner at first base, batter at home
    place_runner_at_base(ctx, 0, BASE_FIRST, 0.0f);
    setup_batter_at_home(ctx, 1);
    move_pitcher_away(ctx);

    // Place a fielder near third to hold the ball after "catching" it
    int fielderIdx = 15;
    Vector3D fielderLoc = ctx->state->fieldPositions->thirdBase;
    ctx->state->match->playerInfo[fielderIdx].tPI.location = fielderLoc;

    // Initialize legal state and snapshot pitch-start positions
    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);
    // After snapshot: runner 0 has baseAtPitchStart = BASE_FIRST

    // Advance runner physically past their pitch-start base (simulate they ran)
    ctx->state->match->playerInfo[0].bTPI.state = PLAYER_STATE_RUNNING;
    ctx->state->match->playerInfo[0].bTPI.baseId = BASE_SECOND;
    ctx->state->match->playerInfo[0].tPI.location = ctx->state->fieldPositions->secondBase;

    // Set contract preconditions: fly ball caught
    ctx->state->match->gameEvents.catchMade = 1;
    ctx->state->match->betweenPitchState.batOutcome = BAT_OUTCOME_HIT;
    ctx->state->match->pII.hasBallIndex = fielderIdx; // Fielder has ball (stabilizes physics)

    // Run exactly 1 pipeline frame
    simulate_frames(ctx, 1);

    // Assert: referee started wounding evaluation
    ASSERT_EQ(1, ctx->state->match->referee.woundingEvaluationActive, "Wounding evaluation should start on catch");

    // Assert: runner 0 is marked as vulnerable
    ASSERT_EQ(
        PLAYER_STATUS_WOUND_MARKED, ctx->state->match->referee.battingPlayers[0].status,
        "Advanced runner should be WOUND_MARKED"
    );

    // Assert: batter (at home = pitch-start base) should NOT be marked
    ASSERT_TRUE(
        ctx->state->match->referee.battingPlayers[1].status != PLAYER_STATUS_WOUND_MARKED,
        "Batter at home (safe) should not be wound-marked"
    );

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
