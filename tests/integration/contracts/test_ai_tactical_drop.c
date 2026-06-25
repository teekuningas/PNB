#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "execute_actions.h"
#include "rules_pure/player_utils.h"
#include "globals.h"

/**
 * CONTRACT: the catching AI's tactical-drop intent (§30 taktinen pudotus), end to end.
 *
 * This knights the DROP intent migration (PLAN.md §4.12 sub-step 1): the AI now *declares*
 * `cTAF.drop_ball` directly instead of driving it through the deleted `dropStage` / `AI_DROP_LOCK`
 * click-sim. The pure decision (`should_ai_drop_ball`) already has a unit test; this proves the two
 * uncovered halves the migration actually touched: (1) `update_catching_ai` acts on that decision by
 * declaring the command, and (2) `execute_actions` consumes the command and the ball leaves the hand.
 *
 * Why a CONSTRUCTED state and not the sim tier: AI-vs-AI essentially never reaches the §30 conditions
 * (a fly ball under evaluation with runners settled on BOTH 2nd and 3rd and the home catcher holding
 * the ball), so the sim determinism hash was unchanged by the migration — i.e. the path was never
 * exercised. The reduced-pipeline construction helpers exist precisely to reach states the producer
 * rarely drives to; we build the exact §30 state and drive the real AI + real execution against it.
 *
 * Preconditions (the §30 tactical drop):
 *   - Runners settled ON_BASE at 2nd and 3rd, baseAtPitchStart = 2nd / 3rd (snapshotted).
 *   - The home catcher (catcherOnBaseIndex[0] = idx 12) holds the ball at home.
 *   - woundingEvaluationActive = 1 (a caught fly is being evaluated).
 *
 * Expected: the AI declares cTAF.drop_ball = ACTION_TRIGGER_START; execute_actions then drops the ball
 * (hasBallIndex → -1, ball moving) and consumes the intent (→ ACTION_IDLE).
 */
int test_ai_declares_and_executes_tactical_drop(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* match = ctx->state->match;

    // Two batting-team runners settled on 3rd and 2nd.
    place_runner_at_base(ctx, 0, BASE_THIRD, 0.0f);
    place_runner_at_base(ctx, 1, BASE_SECOND, 0.0f);

    // The home catcher (idx 12 = catcherOnBaseIndex[0]) holds the ball at home plate.
    give_ball_to_pitcher(ctx);

    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx); // sets baseAtPitchStart from current positions

    // Sanity: the home catcher still holds the ball after the snapshot frame.
    ASSERT_EQ(12, match->pII.hasBallIndex, "home catcher (idx 12) should still hold the ball");

    // The catching team must actually be AI-controlled — that is what makes ai_update dispatch to the
    // catching AI below. create_scenario sets team2 (the fielding team here, playsFirst=0) to CONTROL_AI.
    int battingTeamIndex = get_batting_team_index(&ctx->state->rules->scoreboard);
    int catchingTeamIndex = (battingTeamIndex + 1) % 2;
    ASSERT_EQ(
        CONTROL_AI, (int)ctx->state->rules->scoreboard.teams[catchingTeamIndex].control,
        "the catching team must be AI-controlled for ai_update to drive the drop"
    );

    // Simulate the caught-fly evaluation window the tactical drop lives in.
    ctx->state->rules->referee.woundingEvaluationActive = 1;

    // Drive the REAL production AI dispatch (stage 2c). Because the catching team is AI, ai_update calls
    // update_catching_ai, which—given the §30 state—declares the drop command. (The batting team is
    // human-controlled here, so no batting AI runs.) This is the function the migration changed.
    unsigned int seed = 0xD0D0D0u;
    ai_update(match, ctx->state->rules, ctx->state->fieldPositions, &seed);

    ASSERT_EQ(
        ACTION_TRIGGER_START, (int)match->aF.cTAF.drop_ball,
        "the AI-controlled catching team must DECLARE the drop command in the §30 tactical-drop state"
    );

    // The execution side consumes the declared command and the ball leaves the hand.
    execute_actions(match, ctx->state->rules, ctx->state->fieldPositions, &ctx->state->playSoundEffect);

    ASSERT_EQ(-1, match->pII.hasBallIndex, "drop must release the ball (hasBallIndex → -1)");
    ASSERT_EQ(1, match->ballInfo.moving, "dropped ball should be in motion");
    ASSERT_EQ(ACTION_IDLE, (int)match->aF.cTAF.drop_ball, "drop intent must be consumed to IDLE");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
