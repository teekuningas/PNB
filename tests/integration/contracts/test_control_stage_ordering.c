#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "game_frame.h"
#include "rules_pure/player_utils.h"

/**
 * CONTRACT: the CONTROL stage runs BEFORE execution, in the same tick.
 *
 * This knights PLAN.md §5.10 slice 1a. `ai_update` used to sit third inside stage 2, *after*
 * `execute_actions` had already run, so an intent the AI declared on frame N was not consumed until
 * frame N+1 — a one-frame input buffer that was an accident of call placement, never a design
 * (ARCHITECTURE_VISION.md §8.8 law 1). Slice 1a moved it into a CONTROL stage at the frame top beside
 * `action_invocations`, so both producers declare into the same settled end-of-previous-tick world and
 * both are consumed by the same frame's execution.
 *
 * The property is invisible to the determinism hash — 1a re-baselines that hash by design — and
 * invisible to the scenario tier, whose reduced pipeline (`simulate_frames`) omits the whole CONTROL
 * stage. So it needs its own assertion, and this test IS that assertion: it would have failed before
 * slice 1a and it goes red again the moment anyone moves the call back down the pipeline.
 *
 * The probe is the §30 tactical drop, because it is the one AI intent that is a pure consume-on-read
 * command reachable in a constructed state: `cTAF.drop_ball` is declared by `update_catching_ai` and
 * fully actualized inside `execute_actions` in the same breath. ONE production `update_game_frame` is
 * therefore enough to separate "control before execution" from "control after execution".
 *
 * The state is built inline rather than shared with test_ai_tactical_drop.c on purpose: that test asks
 * whether the AI *decides* to drop and whether execution *can* actualize it (it calls the two stages by
 * hand, in the order it wants). This one asks whether the production pipeline puts them in that order.
 * Same construction, two genuinely different questions — see the §3.3 anti-rule.
 */
int test_control_stage_precedes_execution(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* match = ctx->state->match;

    // ---- the §30 tactical-drop state: runners settled on 3rd and 2nd, ball in the home catcher's hand
    place_runner_at_base(ctx, 0, BASE_THIRD, 0.0f);
    place_runner_at_base(ctx, 1, BASE_SECOND, 0.0f);
    give_ball_to_pitcher(ctx);

    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx); // sets baseAtPitchStart from current positions

    ASSERT_EQ(12, match->pII.hasBallIndex, "home catcher (idx 12) should hold the ball before the frame");

    int battingTeamIndex = get_batting_team_index(&ctx->state->rules->scoreboard);
    int catchingTeamIndex = (battingTeamIndex + 1) % 2;
    ASSERT_EQ(
        CONTROL_AI, (int)ctx->state->rules->scoreboard.teams[catchingTeamIndex].control,
        "the catching team must be AI-controlled, or no controller declares anything this frame"
    );

    // The caught-fly evaluation window the tactical drop lives in.
    ctx->state->rules->referee.woundingEvaluationActive = 1;

    // Nothing is declared yet — so the frame below cannot pass on a leftover intent.
    ASSERT_EQ(ACTION_IDLE, (int)match->aF.cTAF.drop_ball, "no drop may be declared before the frame under test runs");
    ASSERT_EQ(0, ctx->state->match->flowControl.pause, "update_game_frame no-ops while paused");

    // ---- exactly ONE frame of the real production pipeline. No stage is called by hand.
    update_game_frame(ctx->state, &ctx->menu);

    // If CONTROL still ran after execution, the declaration would be sitting here unconsumed and the
    // ball would still be in the catcher's hand — the whole point of the assertion.
    ASSERT_EQ(
        12, match->pII.lastHadBallIndex,
        "the drop must have been ACTUALIZED this frame: lastHadBallIndex is the dropper's fingerprint, "
        "set only by drop_ball(). Still -1 means the CONTROL stage's intent was not consumed until the "
        "next frame — i.e. ai_update is running after execute_actions again"
    );
    ASSERT_EQ(-1, match->pII.hasBallIndex, "the dropped ball must be loose (hasBallIndex → -1)");
    ASSERT_EQ(1, match->ballInfo.moving, "the dropped ball must be in motion");
    ASSERT_EQ(
        ACTION_IDLE, (int)match->aF.cTAF.drop_ball,
        "the drop command must be consumed within the frame that declared it"
    );

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
