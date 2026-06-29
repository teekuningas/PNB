#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "execute_actions.h"
#include "actions/pitching_system.h"
#include "actions_pure/pitching_physics.h"
#include "globals.h"
#include <math.h>

/**
 * CONTRACT: the pitch slice — a DECLARED phased pitch is actualized by the engine-owned windup, and the
 * released ball's velocity is a pure function of the declared aim (never a live meter).
 *
 * This knights the pitch migration (PLAN.md §5): the producer declares a PitchDeclaration; the engine
 * runs its own deterministic windup clock (PitchActualization) and resolves — AIMED → a real pitch whose
 * velocity == pitch_velocity_from_aim(power, direction); never aimed in time → valesyöttö (a fake, the
 * pitcher keeps the ball). The pitch analog of test_ai_tactical_drop: we construct the ready state and
 * drive the REAL execute_actions, no AI and no meter.
 */

// Run execute_actions until the pitcher no longer holds the ball (release) or a frame budget elapses.
// Returns the number of frames ticked.
static int run_actualizer(ScenarioContext* ctx, int budget)
{
    int i;
    for (i = 0; i < budget; i++) {
        execute_actions(
            ctx->state->match, ctx->state->clientInput, ctx->state->rules, ctx->state->fieldPositions,
            &ctx->state->playSoundEffect
        );
        if (ctx->state->match->pII.hasBallIndex == -1) break; // ball left the hand
    }
    return i;
}

// 1. A declared AIMED pitch releases with velocity == pitch_velocity_from_aim(power, direction).
int test_pitch_aimed_releases_with_declared_velocity(void)
{
    ScenarioContext* ctx = create_scenario();
    setup_pitcher_ready(ctx);
    MatchSession* match = ctx->state->match;

    const float power = 0.5f;
    const float direction = 0.0f; // straight up over the plate (a strike)

    // Declare a complete aim (as the AI does at once, or a human at the 3rd click).
    match->aF.cTAF.pitch.phase = PITCH_DECL_AIMED;
    match->aF.cTAF.pitch.power = power;
    match->aF.cTAF.pitch.direction = direction;

    // First frame begins the windup (engine mutex), without releasing.
    execute_actions(
        match, ctx->state->clientInput, ctx->state->rules, ctx->state->fieldPositions, &ctx->state->playSoundEffect
    );
    ASSERT_EQ(
        CATCHING_ACTION_PITCHING, (int)match->pendingActionState.current_catching_action,
        "declaring a pitch must begin the engine-owned windup"
    );
    ASSERT_EQ(12, match->pII.hasBallIndex, "ball is still in hand during the windup");

    // Run the windup clock to its end → release.
    int frames = run_actualizer(ctx, PITCH_WINDUP_FRAMES + 4);

    ASSERT_EQ(-1, match->pII.hasBallIndex, "the ball must leave the hand at the windup end");
    ASSERT_EQ(1, match->gameEvents.pitchReleased, "release must fire the pitchReleased event");
    ASSERT_EQ(PITCH_STAGE_AIRBORNE, (int)match->pRAI.pitch_state, "pitch_state must be AIRBORNE after release");
    ASSERT(frames >= PITCH_WINDUP_FRAMES - 1, "release must wait for the engine windup, not fire instantly");

    // The decisive assertion: the launch velocity is a pure function of the DECLARED aim.
    Vector3D expected = pitch_velocity_from_aim(power, direction);
    ASSERT(fabs(expected.x - match->ballInfo.velocity.x) < 0.0001f, "ball vx == pitch_velocity_from_aim.x");
    ASSERT(fabs(expected.y - match->ballInfo.velocity.y) < 0.0001f, "ball vy == pitch_velocity_from_aim.y");
    ASSERT(fabs(expected.z - match->ballInfo.velocity.z) < 0.0001f, "ball vz == pitch_velocity_from_aim.z");

    // The declaration is consumer-cleared at resolution.
    ASSERT_EQ(PITCH_DECL_IDLE, (int)match->aF.cTAF.pitch.phase, "declaration cleared to IDLE at resolution");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 2. A pitch never aimed in time resolves to a valesyöttö: the pitcher keeps the ball, no pitch is thrown.
int test_pitch_unaimed_is_valesyotto(void)
{
    ScenarioContext* ctx = create_scenario();
    setup_pitcher_ready(ctx);
    MatchSession* match = ctx->state->match;

    // Declare power only — the aim window is never filled (a human who ran out of clicks, or an AI that
    // declined). The phase stays at POWER through the windup.
    match->aF.cTAF.pitch.phase = PITCH_DECL_POWER;
    match->aF.cTAF.pitch.power = 0.5f;

    // Run well past the windup deadline.
    run_actualizer(ctx, PITCH_WINDUP_FRAMES + 4);

    ASSERT_EQ(12, match->pII.hasBallIndex, "valesyöttö: the pitcher keeps the ball (no pitch thrown)");
    ASSERT_EQ(0, match->gameEvents.pitchReleased, "valesyöttö must NOT fire pitchReleased");
    ASSERT_EQ(PITCH_STAGE_NONE, (int)match->pRAI.pitch_state, "valesyöttö ends with no pitch in flight");
    ASSERT_EQ(
        CATCHING_ACTION_NONE, (int)match->pendingActionState.current_catching_action,
        "valesyöttö clears the catching action"
    );
    ASSERT_EQ(PITCH_DECL_IDLE, (int)match->aF.cTAF.pitch.phase, "declaration cleared to IDLE at resolution");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
