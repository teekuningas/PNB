#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "execute_actions.h"
#include "actions/throwing_system.h"
#include "globals.h"
#include <math.h>

/**
 * CONTRACT: the throw rework (PLAN.md §5.8) — a phased ThrowDeclaration is actualized by an engine-owned
 * windup clock (ThrowActualization), the same shape as the pitch. Two producer paths ride ONE clock:
 *   - COMMITTED (AI): target + power declared at once; the engine sizes the windup to the power and
 *     auto-releases at its end. Power drives the release velocity — read from the declaration, not a meter.
 *   - GATHERING → RELEASED (human): the windup runs while "held"; the throw waits for the RELEASED edge and
 *     reads power off the clock (a longer hold = more power). The engine never auto-fires a GATHERING throw.
 * The throw analog of test_pitch_declaration: we construct a ball-holder away from a base and drive the
 * REAL execute_actions (no AI, no meter). Calling execute_actions directly (not the full frame) keeps the
 * thrown ball un-fielded so the launch velocity can be read at the release frame.
 */

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
    execute_actions(ctx->state->match, ctx->state->rules, ctx->state->fieldPositions, &ctx->state->playSoundEffect);
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

// 0. The power<->windup pair round-trips: declaring a power (AI) then reading it back off the clock
//    (human) recovers the same power, within integer-frame truncation. This is the invariant that lets one
//    shared clock serve both producers.
int test_throw_windup_power_roundtrip(void)
{
    // Endpoints exact by construction.
    ASSERT(
        fabsf(throw_power_from_windup(throw_windup_total_frames(THROW_POWER_MIN)) - THROW_POWER_MIN) < 0.001f,
        "floor power round-trips"
    );
    ASSERT(fabsf(throw_power_from_windup(throw_windup_total_frames(1.0f)) - 1.0f) < 0.001f, "max power round-trips");

    // Interior points within one frame's worth of power resolution.
    float tol = (1.0f - THROW_POWER_MIN) / (float)(THROW_WINDUP_MAX_FRAMES - THROW_WINDUP_MIN_FRAMES) + 0.001f;
    for (float p = THROW_POWER_MIN; p <= 1.0f; p += 0.1f) {
        float rt = throw_power_from_windup(throw_windup_total_frames(p));
        ASSERT(fabsf(rt - p) <= tol, "declared power is recovered from the windup clock within one frame");
    }

    // The clock reading is monotonic in the hold duration (longer hold ⇒ at least as much power).
    float prev = -1.0f;
    for (int t = 0; t <= THROW_WINDUP_MAX_FRAMES + 5; t++) {
        float pw = throw_power_from_windup(t);
        ASSERT(pw >= prev - 0.001f, "power is non-decreasing in the hold duration");
        ASSERT(pw >= THROW_POWER_MIN - 0.001f && pw <= 1.0f + 0.001f, "clock-read power stays in [floor,1]");
        prev = pw;
    }
    return TEST_PASSED;
}

// 1. A COMMITTED throw begins the engine windup, releases at throw_windup_total_frames(power), and a
//    higher declared power launches the ball faster (power drives the outcome, read from the declaration).
int test_throw_committed_releases_sized_to_power(void)
{
    // --- low power: also assert the windup timing + resolution bookkeeping ---
    ScenarioContext* ctx = create_scenario();
    setup_fielder_with_ball(ctx);
    MatchSession* m = ctx->state->match;

    const float lowPower = 0.3f;
    m->aF.cTAF.throw.phase = THROW_DECL_COMMITTED;
    m->aF.cTAF.throw.target = BASE_FIRST;
    m->aF.cTAF.throw.power = lowPower;

    // First frame begins the windup (engine mutex), without releasing.
    tick(ctx);
    ASSERT_EQ(
        CATCHING_ACTION_THROWING, (int)m->pendingActionState.current_catching_action,
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
    ASSERT_EQ(THROW_DECL_IDLE, (int)m->aF.cTAF.throw.phase, "the declaration is consumer-cleared to IDLE at release");
    ASSERT_EQ(
        CATCHING_ACTION_NONE, (int)m->pendingActionState.current_catching_action,
        "the catching action clears at release"
    );
    cleanup_scenario(ctx);

    // --- high power: same geometry → the only difference is the declared power ---
    ScenarioContext* ctx2 = create_scenario();
    setup_fielder_with_ball(ctx2);
    ctx2->state->match->aF.cTAF.throw.phase = THROW_DECL_COMMITTED;
    ctx2->state->match->aF.cTAF.throw.target = BASE_FIRST;
    ctx2->state->match->aF.cTAF.throw.power = 1.0f;
    float highSpeed = run_until_release(ctx2, throw_windup_total_frames(1.0f) + 6, NULL);
    ASSERT(highSpeed > 0.0f, "the high-power throw must release");

    ASSERT(
        highSpeed > lowSpeed,
        "a higher DECLARED power must launch the ball faster (power drives the outcome, read from the intent)"
    );
    cleanup_scenario(ctx2);
    return TEST_PASSED;
}

// 2. A GATHERING throw waits for the human's RELEASED edge (never auto-fires), the clock caps at a full
//    hold, and the power is read from the clock — a longer hold launches the ball faster.
int test_throw_gathering_reads_windup_clock_power(void)
{
    // --- long hold: run well past the AI windup, prove it does NOT auto-release, then release at full ---
    ScenarioContext* ctx = create_scenario();
    setup_fielder_with_ball(ctx);
    MatchSession* m = ctx->state->match;

    m->aF.cTAF.throw.phase = THROW_DECL_GATHERING;
    m->aF.cTAF.throw.target = BASE_FIRST;

    for (int i = 0; i < THROW_WINDUP_MAX_FRAMES + 20; i++) {
        tick(ctx);
    }
    ASSERT_EQ(15, m->pII.hasBallIndex, "a GATHERING throw must wait for the RELEASED edge — never auto-release");
    ASSERT_EQ(
        CATCHING_ACTION_THROWING, (int)m->pendingActionState.current_catching_action, "still winding up while held"
    );
    ASSERT_EQ(
        THROW_WINDUP_MAX_FRAMES, m->pendingActionState.throwActualization.timer,
        "the windup clock caps at the full-power hold (holding past full does not overspill)"
    );

    // Release now → power reads the (full) clock.
    m->aF.cTAF.throw.phase = THROW_DECL_RELEASED;
    tick(ctx);
    ASSERT_EQ(-1, m->pII.hasBallIndex, "the RELEASED edge fires the throw");
    float fullSpeed = horiz_speed(m);
    ASSERT_EQ(THROW_DECL_IDLE, (int)m->aF.cTAF.throw.phase, "the declaration is cleared to IDLE at release");
    cleanup_scenario(ctx);

    // --- short hold: release after just a couple of frames → floor-ish power → slower launch ---
    ScenarioContext* ctx2 = create_scenario();
    setup_fielder_with_ball(ctx2);
    MatchSession* m2 = ctx2->state->match;
    m2->aF.cTAF.throw.phase = THROW_DECL_GATHERING;
    m2->aF.cTAF.throw.target = BASE_FIRST;
    tick(ctx2); // frame 1: begin windup
    tick(ctx2); // frame 2: winding (timer small)
    m2->aF.cTAF.throw.phase = THROW_DECL_RELEASED;
    tick(ctx2); // release with a small clock reading
    ASSERT_EQ(-1, m2->pII.hasBallIndex, "the short-hold throw must release");
    float shortSpeed = horiz_speed(m2);

    ASSERT(fullSpeed > shortSpeed, "a longer hold reads more power off the shared windup clock (clock-driven power)");
    cleanup_scenario(ctx2);
    return TEST_PASSED;
}
