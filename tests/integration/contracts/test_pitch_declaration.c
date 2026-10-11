#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "execute_actions.h"
#include "actions/pitching_system.h"
#include "actions_pure/pitching_physics.h"
#include "globals.h"
#include <math.h>

/**
 * CONTRACT: the pitch — two declared values actualized by the engine-owned windup, and the released
 * ball's velocity a pure function of them (never a live meter).
 *
 * The producer says a power, which begins the windup, and an aim, which is held on it; the engine runs
 * its own deterministic clock and resolves — aimed when the windup ends → a real pitch whose velocity ==
 * pitch_velocity_from_aim(power, direction); never aimed in time → valesyöttö (a fake, the pitcher keeps
 * the ball). We construct the ready state and drive the REAL execute_actions through the catching
 * channel, no AI and no meter.
 */

static void declare_power(ScenarioContext* ctx, float power)
{
    intent_push(
        &ctx->state->channels.catching, (IntentMessage){.kind = INTENT_PITCH_POWER, .as.pitch_power = {.power = power}}
    );
}

static void declare_aim(ScenarioContext* ctx, float direction)
{
    intent_push(
        &ctx->state->channels.catching,
        (IntentMessage){.kind = INTENT_PITCH_AIM, .as.pitch_aim = {.direction = direction}}
    );
}

static void tick(ScenarioContext* ctx)
{
    execute_actions(
        ctx->state->match, ctx->state->rules, ctx->state->fieldPositions, &ctx->state->channels,
        &ctx->state->playSoundEffect
    );
}

// Run execute_actions until the pitcher no longer holds the ball (release) or a frame budget elapses.
// Returns the number of frames ticked.
static int run_actualizer(ScenarioContext* ctx, int budget)
{
    int i;
    for (i = 0; i < budget; i++) {
        tick(ctx);
        if (ctx->state->match->pII.hasBallIndex == -1) break; // ball left the hand
    }
    return i;
}

// 1. A pitch declared whole — power and aim in one tick, as the AI does — releases with velocity ==
//    pitch_velocity_from_aim(power, direction).
int test_pitch_aimed_releases_with_declared_velocity(void)
{
    ScenarioContext* ctx = create_scenario();
    setup_pitcher_ready(ctx);
    MatchSession* match = ctx->state->match;

    const float power = 0.5f;
    const float direction = 0.0f; // straight up over the plate (a strike)

    declare_power(ctx, power);
    declare_aim(ctx, direction);

    // First frame begins the windup, without releasing.
    tick(ctx);
    ASSERT_EQ(
        CATCHING_ACTION_PITCHING, (int)match->pendingActionState.catchingAction.kind,
        "declaring a pitch must begin the engine-owned windup"
    );
    ASSERT_EQ(1, match->pendingActionState.catchingAction.as.pitch.aimed, "an aim declared with the power is held");
    ASSERT_EQ(12, match->pII.hasBallIndex, "ball is still in hand during the windup");

    // Run the windup clock to its end → release.
    int windup = pitch_windup_total_frames(power);
    int frames = run_actualizer(ctx, windup + 4);

    ASSERT_EQ(-1, match->pII.hasBallIndex, "the ball must leave the hand at the windup end");
    ASSERT_EQ(1, match->gameEvents.pitchReleased, "release must fire the pitchReleased event");
    ASSERT_EQ(PITCH_STAGE_AIRBORNE, (int)match->pRAI.pitch_state, "pitch_state must be AIRBORNE after release");
    ASSERT(frames >= windup - 1, "release must wait for the engine windup, not fire instantly");

    // The decisive assertion: the launch velocity is a pure function of the DECLARED values.
    Vector3D expected = pitch_velocity_from_aim(power, direction);
    ASSERT(fabs(expected.x - match->ballInfo.velocity.x) < 0.0001f, "ball vx == pitch_velocity_from_aim.x");
    ASSERT(fabs(expected.y - match->ballInfo.velocity.y) < 0.0001f, "ball vy == pitch_velocity_from_aim.y");
    ASSERT(fabs(expected.z - match->ballInfo.velocity.z) < 0.0001f, "ball vz == pitch_velocity_from_aim.z");

    ASSERT_EQ(CATCHING_ACTION_NONE, (int)match->pendingActionState.catchingAction.kind, "the pitch is done at release");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 2. A pitch never aimed in time resolves to a valesyöttö: the pitcher keeps the ball, no pitch is thrown.
int test_pitch_unaimed_is_valesyotto(void)
{
    ScenarioContext* ctx = create_scenario();
    setup_pitcher_ready(ctx);
    MatchSession* match = ctx->state->match;

    // Power only — the aim never comes (a human whose aim meter ran out).
    declare_power(ctx, 0.5f);

    // Run well past the windup deadline.
    run_actualizer(ctx, pitch_windup_total_frames(0.5f) + 4);

    ASSERT_EQ(12, match->pII.hasBallIndex, "valesyöttö: the pitcher keeps the ball (no pitch thrown)");
    ASSERT_EQ(0, match->gameEvents.pitchReleased, "valesyöttö must NOT fire pitchReleased");
    ASSERT_EQ(PITCH_STAGE_NONE, (int)match->pRAI.pitch_state, "valesyöttö ends with no pitch in flight");
    ASSERT_EQ(
        CATCHING_ACTION_NONE, (int)match->pendingActionState.catchingAction.kind, "valesyöttö ends the catching action"
    );

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 3. An aim declared later, while the windup runs — as a human does — is the aim the ball leaves with.
int test_pitch_aim_declared_during_the_windup_is_the_aim_released(void)
{
    ScenarioContext* ctx = create_scenario();
    setup_pitcher_ready(ctx);
    MatchSession* match = ctx->state->match;

    const float power = 0.3f;
    const float direction = 0.4f;
    declare_power(ctx, power);
    tick(ctx);
    for (int i = 0; i < 20; i++) {
        tick(ctx);
    }
    ASSERT_EQ(0, match->pendingActionState.catchingAction.as.pitch.aimed, "no aim has been declared yet");
    declare_aim(ctx, direction);
    run_actualizer(ctx, pitch_windup_total_frames(power) + 4);

    ASSERT_EQ(-1, match->pII.hasBallIndex, "an aimed windup releases a real pitch");
    Vector3D expected = pitch_velocity_from_aim(power, direction);
    ASSERT(fabs(expected.x - match->ballInfo.velocity.x) < 0.0001f, "the late aim is the aim released");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 4. §18(1) as a permission: no pitch may begin to a batter who has already received his three correct
//    pitches. "Lyöjä muuttuu lopullisesti etenijäksi, kun hän saa kolme oikeaa syöttöä" — he is out of the
//    batting turn, so there is nobody to pitch to.
//
//    This is stated at the permission rather than left to the engine's forced run, which used to be the
//    only thing closing the pitch: it cleared batter_ready as a side effect, and only in normal play.
//    The home-run contest skipped it and got a fourth pitch (bug #8). The conjunct makes the fourth pitch
//    unrepresentable in every mode instead of merely unreached in one.
int test_no_pitch_after_three_correct_pitches(void)
{
    // Control: with tries left, the same declaration DOES begin a windup. Without this half, the test
    // would still pass if the fixture were simply too broken to pitch at all.
    ScenarioContext* allowed = create_scenario();
    setup_pitcher_ready(allowed);
    allowed->state->rules->halfInningState.strikes = 2;
    declare_power(allowed, 0.5f);
    declare_aim(allowed, 0.0f);
    tick(allowed);
    ASSERT_EQ(
        CATCHING_ACTION_PITCHING, (int)allowed->state->match->pendingActionState.catchingAction.kind,
        "with a correct pitch still owed, the windup must begin"
    );
    cleanup_scenario(allowed);

    // The rule: the third correct pitch has been spent, so the windup must be refused.
    ScenarioContext* ctx = create_scenario();
    setup_pitcher_ready(ctx);
    MatchSession* match = ctx->state->match;
    ctx->state->rules->halfInningState.strikes = 3;

    declare_power(ctx, 0.5f);
    declare_aim(ctx, 0.0f);
    tick(ctx);

    ASSERT_EQ(
        CATCHING_ACTION_NONE, (int)match->pendingActionState.catchingAction.kind,
        "a batter who has spent his three correct pitches may not be pitched to"
    );
    ASSERT_EQ(12, match->pII.hasBallIndex, "the refused pitch leaves the ball in the pitcher's hand");

    // Nothing of the refused pitch is left pending to fire later: once the count would allow a pitch
    // again, a tick with nothing declared still begins nothing.
    ctx->state->rules->halfInningState.strikes = 2;
    tick(ctx);
    ASSERT_EQ(
        CATCHING_ACTION_NONE, (int)match->pendingActionState.catchingAction.kind,
        "a refused pitch is dropped, not left pending to fire later"
    );

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
