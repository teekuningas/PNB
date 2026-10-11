#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "execute_actions.h"
#include <math.h>

/**
 * CONTRACT: a pitch is in progress for its whole windup, on EVERY pitch of a half-inning.
 *
 * Two rules hold while the pitcher is winding up: the ball cannot be dropped (the pitch has a claim on
 * it), and the controlled fielder — the pitcher, whose hand the ball is in — stays where the windup put
 * him. Both used to ask `pRAI.pitch_state`, which then carried a windup state that consolidation zeroed
 * on every frame the previous pitch's result still stood — the whole of every windup after a
 * half-inning's first. On those windups both rules were off. The windup state has since left
 * `pitch_state` altogether; the test still builds the later-windup world, because that is the claim.
 *
 * Nothing the game's own producers do reaches it — the human's drop key is for a holder who is not the
 * pitcher, and neither controller steers during a windup — but the gate is the one place a declared
 * intent is told no, and it does not get to trust its producers. A scripted test, a replay, or one day a
 * network peer can send a drop in the middle of a windup, and the engine then releases a pitch out of an
 * empty hand. So the state is BUILT here, exactly as consolidation leaves it, and the messages are sent.
 */

#define DECLARED_POWER 0.5f

static void tick(ScenarioContext* ctx)
{
    execute_actions(
        ctx->state->match, ctx->state->rules, ctx->state->fieldPositions, &ctx->state->channels,
        &ctx->state->playSoundEffect
    );
}

// The pitcher, controlled and holding the ball, is in the middle of a windup that is NOT the
// half-inning's first: the previous pitch's result still stands, so consolidation has zeroed
// pitch_state, and the engine's own record that a pitch is being delivered is the catching action.
static ScenarioContext* later_windup(void)
{
    ScenarioContext* ctx = create_scenario();
    setup_pitcher_ready(ctx);
    MatchSession* m = ctx->state->match;
    m->pII.controlIndex = m->pII.hasBallIndex;

    intent_push(
        &ctx->state->channels.catching,
        (IntentMessage){.kind = INTENT_PITCH_POWER, .as.pitch_power = {.power = DECLARED_POWER}}
    );
    intent_push(
        &ctx->state->channels.catching, (IntentMessage){.kind = INTENT_PITCH_AIM, .as.pitch_aim = {.direction = 0.0f}}
    );
    tick(ctx); // the windup begins

    ctx->state->rules->betweenPitchState.pitchResult = PITCH_RESULT_BALL; // the previous pitch, still standing
    m->pRAI.pitch_state = PITCH_STAGE_NONE; // ...and what consolidation does to pitch_state because of it
    return ctx;
}

// 1. A drop declared mid-windup is refused, and the pitch it would have emptied the hand of still flies.
int test_drop_refused_during_a_later_windup(void)
{
    ScenarioContext* ctx = later_windup();
    MatchSession* m = ctx->state->match;
    const int pitcher = m->pII.hasBallIndex;
    ASSERT_EQ(
        CATCHING_ACTION_PITCHING, (int)m->pendingActionState.catchingAction.kind, "precondition: a windup is running"
    );

    intent_push(&ctx->state->channels.catching, (IntentMessage){.kind = INTENT_DROP_BALL});
    tick(ctx);
    ASSERT_EQ(pitcher, m->pII.hasBallIndex, "the pitcher must still hold the ball: a pitch has a claim on it");

    int released = 0;
    for (int i = 0; i < 200 && !released; i++) {
        tick(ctx);
        released = (m->pRAI.pitch_state == PITCH_STAGE_AIRBORNE);
    }
    ASSERT(released, "the windup must still end in the pitch that was declared");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 2. The pitcher stays put through the windup whoever tells him to walk, as he does on the first one.
int test_controlled_fielder_holds_during_a_later_windup(void)
{
    ScenarioContext* ctx = later_windup();
    MatchSession* m = ctx->state->match;
    const int pitcher = m->pII.hasBallIndex;
    const Vector3D at = m->playerInfo[pitcher].tPI.location;

    // Only the engine stage runs here, so nothing integrates a velocity into a position: the walk shows
    // up as the velocity the mover sets, which is what is read.
    IntentMessage move = {.kind = INTENT_MOVE_TARGET};
    move.as.move_target.point.x = at.x + 10.0f;
    move.as.move_target.point.z = at.z + 10.0f;
    intent_push(&ctx->state->channels.catching, move);
    tick(ctx);

    const Vector3D v = m->playerInfo[pitcher].tPI.velocity;
    ASSERT_EQ(
        CATCHING_ACTION_PITCHING, (int)m->pendingActionState.catchingAction.kind, "precondition: still winding up"
    );
    ASSERT_FLOAT_EQ(0.0f, v.x, 1e-6f, "the pitcher must not be set walking during the windup");
    ASSERT_FLOAT_EQ(0.0f, v.z, 1e-6f, "the pitcher must not be set walking during the windup");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
