#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "execute_actions.h"
#include "actions/throwing_system.h"
#include "globals.h"
#include <math.h>

/**
 * CONTRACT: the throw — two declared values, a target and a power, actualized by the engine-owned windup
 * clock (CatchingAction.timer). Both producers declare `power` as a trusted VALUE; the ENGINE owns the
 * release instant (release when the clock reaches windup(power)), never a client "fire now" edge, never
 * derived from the clock:
 *   - both in one tick (AI): the engine sizes the windup to the power and auto-releases at its end. Higher
 *     declared power → faster launch.
 *   - target, then power (human): the target starts the windup (so the gather runs while the human picks
 *     power on the meter); the power hands the engine the value and it releases once the windup for that
 *     power completes — immediately if it already elapsed, else it waits.
 * We construct a ball-holder away from a base and drive the REAL execute_actions through the catching
 * channel (no AI, no meter). Calling execute_actions directly (not the full frame) keeps the thrown ball
 * un-fielded so the launch velocity can be read at the release frame.
 */

static void declare_target(ScenarioContext* ctx, BaseID base)
{
    intent_push(
        &ctx->state->channels.catching, (IntentMessage){.kind = INTENT_THROW_TARGET, .as.throw_target = {.base = base}}
    );
}

static void declare_power(ScenarioContext* ctx, float power)
{
    intent_push(
        &ctx->state->channels.catching, (IntentMessage){.kind = INTENT_THROW_POWER, .as.throw_power = {.power = power}}
    );
}

// A controlled fielder holds the ball out in the field, far enough from any base to throw.
static void setup_fielder_with_ball(ScenarioContext* ctx)
{
    MatchSession* m = ctx->state->match;
    int fielderIdx = 15;
    m->pII.hasBallIndex = fielderIdx;
    m->pII.controlIndex = fielderIdx;
    m->playerInfo[fielderIdx].tPI.location.x = 5.0f;
    m->playerInfo[fielderIdx].tPI.location.z = 5.0f;
}

static void tick(ScenarioContext* ctx)
{
    execute_actions(
        ctx->state->match, ctx->state->rules, ctx->state->fieldPositions, &ctx->state->channels,
        &ctx->state->playSoundEffect
    );
}

static float horiz_speed(const MatchSession* m)
{
    float vx = m->ballInfo.velocity.x, vz = m->ballInfo.velocity.z;
    return sqrtf(vx * vx + vz * vz);
}

// Run execute_actions until the ball leaves the hand; return the launch horizontal speed (or -1). Also
// reports the number of frames ticked via *frames.
static float run_until_release(ScenarioContext* ctx, int budget, int* frames)
{
    MatchSession* m = ctx->state->match;
    int i;
    for (i = 0; i < budget; i++) {
        tick(ctx);
        if (m->pII.hasBallIndex == -1) {
            if (frames) *frames = i + 1;
            return horiz_speed(m);
        }
    }
    if (frames) *frames = i;
    return -1.0f;
}

// 0. The windup sizing scales with declared power: throw_windup_total_frames maps [THROW_POWER_MIN,1] →
//    [MIN,MAX] frames, monotonically. This is the ONLY throw↔clock relationship that survives — the
//    human's power is a value declared from the client charge widget, never read back off the clock,
//    so the old power<->windup round-trip (and throw_power_from_windup) is gone.
int test_throw_windup_frames_scale_with_power(void)
{
    // Endpoints exact by construction.
    ASSERT_EQ(THROW_WINDUP_MIN_FRAMES, throw_windup_total_frames(THROW_POWER_MIN), "floor power → MIN windup");
    ASSERT_EQ(THROW_WINDUP_MAX_FRAMES, throw_windup_total_frames(1.0f), "max power → MAX windup");

    // Out-of-band power clamps into the band.
    ASSERT_EQ(THROW_WINDUP_MIN_FRAMES, throw_windup_total_frames(0.0f), "power below floor clamps to MIN windup");
    ASSERT_EQ(THROW_WINDUP_MAX_FRAMES, throw_windup_total_frames(2.0f), "power above 1 clamps to MAX windup");

    // Non-decreasing and bounded across the band.
    int prev = -1;
    for (float p = 0.0f; p <= 1.0f; p += 0.05f) {
        int f = throw_windup_total_frames(p);
        ASSERT(f >= prev, "windup length is non-decreasing in declared power");
        ASSERT(f >= THROW_WINDUP_MIN_FRAMES && f <= THROW_WINDUP_MAX_FRAMES, "windup length stays within [MIN,MAX]");
        prev = f;
    }
    return TEST_PASSED;
}

// 1. A throw declared whole begins the engine windup, releases at throw_windup_total_frames(power), and a
//    higher declared power launches the ball faster (power drives the outcome).
int test_throw_declared_whole_releases_sized_to_power(void)
{
    // --- low power: also assert the windup timing + resolution bookkeeping ---
    ScenarioContext* ctx = create_scenario();
    setup_fielder_with_ball(ctx);
    MatchSession* m = ctx->state->match;

    const float lowPower = 0.3f;
    declare_target(ctx, BASE_FIRST);
    declare_power(ctx, lowPower);

    // First frame begins the windup (engine mutex), without releasing.
    tick(ctx);
    ASSERT_EQ(
        CATCHING_ACTION_THROWING, (int)m->pendingActionState.catchingAction.kind,
        "declaring a throw must begin the engine-owned windup"
    );
    ASSERT_EQ(15, m->pII.hasBallIndex, "ball is still in hand during the windup");

    int windup = throw_windup_total_frames(lowPower);
    int frames = 0;
    float lowSpeed = run_until_release(ctx, windup + 6, &frames);

    ASSERT_EQ(-1, m->pII.hasBallIndex, "the ball must leave the hand at the windup end");
    ASSERT(lowSpeed > 0.0f, "a released throw has a horizontal launch velocity");
    // frames counts from the SECOND frame onward (the first began the windup); release near the windup end.
    ASSERT(frames + 1 >= windup - 1, "release must wait for the engine windup, not fire instantly");
    ASSERT_EQ(
        CATCHING_ACTION_NONE, (int)m->pendingActionState.catchingAction.kind, "the catching action clears at release"
    );
    cleanup_scenario(ctx);

    // --- high power: same geometry → the only difference is the declared power ---
    ScenarioContext* ctx2 = create_scenario();
    setup_fielder_with_ball(ctx2);
    declare_target(ctx2, BASE_FIRST);
    declare_power(ctx2, 1.0f);
    float highSpeed = run_until_release(ctx2, throw_windup_total_frames(1.0f) + 6, NULL);
    ASSERT(highSpeed > 0.0f, "the high-power throw must release");

    ASSERT(highSpeed > lowSpeed, "a higher DECLARED power must launch the ball faster (power drives the outcome)");
    cleanup_scenario(ctx2);
    return TEST_PASSED;
}

// 2. The human's two values a moment apart: a target winds up but never releases while the power is owed;
//    the power hands the engine the value, and the ENGINE owns the release instant — it fires when its
//    clock reaches windup(power), NOT on the frame the power arrived. Proven two ways: (a) a long gather
//    then a low power releases immediately (the windup already elapsed); (b) a short gather then a high
//    power makes the engine WAIT out the windup (no client "fire now" edge). Velocity tracks the DECLARED
//    power either way — the value is trusted, not derived from the clock.
int test_throw_target_then_power_engine_times_release(void)
{
    // --- a long gather, then LOW power → releases immediately, velocity sized to the low power ---
    ScenarioContext* ctx = create_scenario();
    setup_fielder_with_ball(ctx);
    MatchSession* m = ctx->state->match;

    declare_target(ctx, BASE_FIRST);

    for (int i = 0; i < THROW_WINDUP_MAX_FRAMES + 20; i++) {
        tick(ctx);
    }
    ASSERT_EQ(15, m->pII.hasBallIndex, "a throw never releases while its power is owed");
    ASSERT_EQ(
        CATCHING_ACTION_THROWING, (int)m->pendingActionState.catchingAction.kind, "still winding up while pending"
    );
    ASSERT_EQ(
        THROW_WINDUP_MAX_FRAMES, m->pendingActionState.catchingAction.timer,
        "the windup clock caps at the full-power windup (holding past full does not overspill)"
    );

    // Declare a LOW power. The clock (capped at MAX) is already well past windup(0.3), so the engine
    // releases this very tick — the "power arrived late" case.
    declare_power(ctx, 0.3f);
    tick(ctx);
    ASSERT_EQ(-1, m->pII.hasBallIndex, "a power declared after a full windup releases immediately");
    float longInitLowPowerSpeed = horiz_speed(m);
    ASSERT_EQ(CATCHING_ACTION_NONE, (int)m->pendingActionState.catchingAction.kind, "the throw is done at release");
    cleanup_scenario(ctx);

    // --- a short gather, then HIGH power → the engine WAITS out the windup (it owns the release instant) ---
    ScenarioContext* ctx2 = create_scenario();
    setup_fielder_with_ball(ctx2);
    MatchSession* m2 = ctx2->state->match;
    declare_target(ctx2, BASE_FIRST);
    tick(ctx2); // begin windup
    tick(ctx2); // barely wound (clock ~1, well short of windup(1.0) = MAX)

    declare_power(ctx2, 1.0f);
    tick(ctx2); // the power does NOT fire this frame — the windup is not done
    ASSERT_EQ(15, m2->pII.hasBallIndex, "the engine owns the release: a powered throw waits out its windup");
    ASSERT_EQ(
        CATCHING_ACTION_THROWING, (int)m2->pendingActionState.catchingAction.kind, "still winding after the power"
    );

    int budget = THROW_WINDUP_MAX_FRAMES + 10, frames = 0;
    float shortInitHighPowerSpeed = run_until_release(ctx2, budget, &frames);
    ASSERT(shortInitHighPowerSpeed > 0.0f, "the high-power throw eventually releases (once the windup completes)");
    ASSERT(
        frames >= THROW_WINDUP_MAX_FRAMES - THROW_WINDUP_MIN_FRAMES,
        "release waited for the full-power windup, not the frame the power arrived"
    );

    // The DECLARED power decides the launch speed (trusted value), independent of how long the gather ran.
    ASSERT(
        shortInitHighPowerSpeed > longInitLowPowerSpeed,
        "launch velocity tracks the DECLARED power value, not the hold/clock (the engine↔client contract)"
    );
    cleanup_scenario(ctx2);
    return TEST_PASSED;
}
