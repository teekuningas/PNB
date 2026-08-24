#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "execute_actions.h"
#include "actions/throwing_system.h"
#include "globals.h"
#include <math.h>

/**
 * CONTRACT: the throw (the phased shape plus the engine↔client fix) — the throw is ONE intent
 * {direction, power} the engine actualizes over its own windup clock (ThrowActualization). Both producers
 * declare `power` as a trusted VALUE; the ENGINE owns the release instant (release when the clock reaches
 * windup(power)), never a client "fire now" edge, never derived from the clock:
 *   - COMMITTED in one frame (AI): direction + power at once; the engine sizes the windup to the power and
 *     auto-releases at its end. Higher declared power → faster launch.
 *   - INITIATED → COMMITTED in two frames (human): INITIATED{direction} starts the windup (so the gather
 *     runs while the human picks power on the meter); COMMITTED{power} hands the engine the value and it
 *     releases once the windup for that power completes — immediately if it already elapsed, else it waits.
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

// 0. The AI windup sizing scales with declared power: throw_windup_total_frames maps [THROW_POWER_MIN,1] →
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

// 1. A COMMITTED throw begins the engine windup, releases at throw_windup_total_frames(power), and a
//    higher declared power launches the ball faster (power drives the outcome, read from the declaration).
int test_throw_committed_releases_sized_to_power(void)
{
    // --- low power: also assert the windup timing + resolution bookkeeping ---
    ScenarioContext* ctx = create_scenario();
    setup_fielder_with_ball(ctx);
    MatchSession* m = ctx->state->match;

    const float lowPower = 0.3f;
    m->pendingActionState.throwDeclaration.phase = THROW_DECL_COMMITTED;
    m->pendingActionState.throwDeclaration.target = BASE_FIRST;
    m->pendingActionState.throwDeclaration.power = lowPower;

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
    ASSERT_EQ(
        THROW_DECL_IDLE, (int)m->pendingActionState.throwDeclaration.phase,
        "the declaration is consumer-cleared to IDLE at release"
    );
    ASSERT_EQ(
        CATCHING_ACTION_NONE, (int)m->pendingActionState.current_catching_action,
        "the catching action clears at release"
    );
    cleanup_scenario(ctx);

    // --- high power: same geometry → the only difference is the declared power ---
    ScenarioContext* ctx2 = create_scenario();
    setup_fielder_with_ball(ctx2);
    ctx2->state->match->pendingActionState.throwDeclaration.phase = THROW_DECL_COMMITTED;
    ctx2->state->match->pendingActionState.throwDeclaration.target = BASE_FIRST;
    ctx2->state->match->pendingActionState.throwDeclaration.power = 1.0f;
    float highSpeed = run_until_release(ctx2, throw_windup_total_frames(1.0f) + 6, NULL);
    ASSERT(highSpeed > 0.0f, "the high-power throw must release");

    ASSERT(
        highSpeed > lowSpeed,
        "a higher DECLARED power must launch the ball faster (power drives the outcome, read from the intent)"
    );
    cleanup_scenario(ctx2);
    return TEST_PASSED;
}

// 2. The human's two-frame intent: INITIATED (direction) winds up but never releases while power is
//    PENDING; COMMITTED (power) hands the engine the value, and the ENGINE owns the release instant — it
//    fires when its clock reaches windup(power), NOT on the frame the client committed. Proven two ways:
//    (a) a long INITIATED then a low power releases immediately (the windup already elapsed); (b) a short
//    INITIATED then a high power makes the engine WAIT out the windup (no client "fire now" edge). Velocity
//    tracks the DECLARED power either way — the value is trusted, not derived from the clock.
int test_throw_initiated_then_committed_engine_times_release(void)
{
    // --- long INITIATED, then LOW power → releases immediately, velocity sized to the low power ---
    ScenarioContext* ctx = create_scenario();
    setup_fielder_with_ball(ctx);
    MatchSession* m = ctx->state->match;

    m->pendingActionState.throwDeclaration.phase = THROW_DECL_INITIATED;
    m->pendingActionState.throwDeclaration.target = BASE_FIRST;

    for (int i = 0; i < THROW_WINDUP_MAX_FRAMES + 20; i++) {
        tick(ctx);
    }
    ASSERT_EQ(15, m->pII.hasBallIndex, "an INITIATED throw never releases while power is pending");
    ASSERT_EQ(
        CATCHING_ACTION_THROWING, (int)m->pendingActionState.current_catching_action, "still winding up while pending"
    );
    ASSERT_EQ(
        THROW_WINDUP_MAX_FRAMES, m->pendingActionState.throwActualization.timer,
        "the windup clock caps at the full-power windup (holding past full does not overspill)"
    );

    // Commit a LOW power. The clock (capped at MAX) is already well past windup(0.3), so the engine releases
    // this very tick — the "second frame arrived late" case.
    m->pendingActionState.throwDeclaration.phase = THROW_DECL_COMMITTED;
    m->pendingActionState.throwDeclaration.power = 0.3f;
    tick(ctx);
    ASSERT_EQ(-1, m->pII.hasBallIndex, "committing after a full windup releases immediately");
    float longInitLowPowerSpeed = horiz_speed(m);
    ASSERT_EQ(
        THROW_DECL_IDLE, (int)m->pendingActionState.throwDeclaration.phase,
        "the declaration is cleared to IDLE at release"
    );
    cleanup_scenario(ctx);

    // --- short INITIATED, then HIGH power → the engine WAITS out the windup (it owns the release instant) ---
    ScenarioContext* ctx2 = create_scenario();
    setup_fielder_with_ball(ctx2);
    MatchSession* m2 = ctx2->state->match;
    m2->pendingActionState.throwDeclaration.phase = THROW_DECL_INITIATED;
    m2->pendingActionState.throwDeclaration.target = BASE_FIRST;
    tick(ctx2); // begin windup
    tick(ctx2); // barely wound (clock ~1, well short of windup(1.0) = MAX)

    m2->pendingActionState.throwDeclaration.power = 1.0f;
    m2->pendingActionState.throwDeclaration.phase = THROW_DECL_COMMITTED;
    tick(ctx2); // committing does NOT fire this frame — the windup is not done
    ASSERT_EQ(15, m2->pII.hasBallIndex, "the engine owns the release: a committed throw waits out its windup");
    ASSERT_EQ(
        CATCHING_ACTION_THROWING, (int)m2->pendingActionState.current_catching_action, "still winding after commit"
    );

    int budget = THROW_WINDUP_MAX_FRAMES + 10, frames = 0;
    float shortInitHighPowerSpeed = run_until_release(ctx2, budget, &frames);
    ASSERT(shortInitHighPowerSpeed > 0.0f, "the high-power throw eventually releases (once the windup completes)");
    ASSERT(
        frames >= THROW_WINDUP_MAX_FRAMES - THROW_WINDUP_MIN_FRAMES,
        "release waited for the full-power windup, not the client's commit frame"
    );

    // The DECLARED power decides the launch speed (trusted value), independent of how long INITIATED ran.
    ASSERT(
        shortInitHighPowerSpeed > longInitLowPowerSpeed,
        "launch velocity tracks the DECLARED power value, not the hold/clock (the engine↔client contract)"
    );
    cleanup_scenario(ctx2);
    return TEST_PASSED;
}
