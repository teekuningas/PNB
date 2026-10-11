#include "test_helpers.h"
#include "scripted_harness.h"
#include "globals.h"
#include "action_invocations.h" // catching_widget_view — what the pitcher actually sees on the bar
#include <math.h>

/*
 * Scripted-human PITCH test — the human input→intent path for the pitch slice, driven
 * entirely through the real action_invocations via scripted KeyStates (no AI, no direct state pokes).
 *
 * It proves the two-ping-pong meter dance: press → start the power meter sweeping; click during the sweep
 * → declare power AND start the windup (power-as-start); click during the aim sweep → declare aim (and the
 * engine releases a real pitch). Timing the aim click on the early/low part of the sweep keeps the
 * direction near the plate (a strike). Dropping the aim click → the aim meter runs out and the engine
 * windup elapses → valesyöttö (a fake; the pitcher keeps the ball). The pitching team is the human here.
 */

#define HUMAN_PAD CONTROL_PLAYER_1 /* the catching (pitching) team is the human */

// Tick until the human controls the pitcher, holding the ball at home, with a ready batter and no
// catching action in progress — i.e. a windup may legally begin.
static int tick_until_pitch_ready(ScriptedGame* g, int budget)
{
    for (int i = 0; i < budget; i++) {
        scripted_tick(g);
        MatchSession* m = scripted_match(g);
        int p = m->pII.catcherOnBaseIndex[0];
        if (p != -1 && m->pII.hasBallIndex == p && m->pII.controlIndex == p && m->pRAI.batter_ready == 1 &&
            m->pendingActionState.catchingAction.kind == CATCHING_ACTION_NONE &&
            m->pRAI.pitch_state == PITCH_STAGE_NONE) {
            return 1;
        }
    }
    return 0;
}

// Tick until the human's pitch sampling widget is actively sweeping (one ping-pong is running), so the
// next click lands a value rather than no-op'ing. Returns 0 if the meter never starts.
static int tick_until_widget_sweeping(ScriptedGame* g, int budget)
{
    ClientInputState* ci = scripted_state(g)->clientInput;
    for (int i = 0; i < budget; i++) {
        if (ci->pitchWidget.meter.dir != 0) return 1;
        scripted_tick(g);
    }
    return ci->pitchWidget.meter.dir != 0;
}

// 1. Press → power click → aim click (timed at the sweet-spot) → a real strike-zone pitch is released.
int test_scripted_pitch_two_pingpongs_strike(void)
{
    ScriptedGame* g = scripted_create(0, 1, CONTROL_AI, HUMAN_PAD, 0x9117C401u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_pitch_ready(g, 8000), "never reached a human pitch-ready state");

    scripted_tap(g, HUMAN_PAD, KEY_2); // press: start the power meter sweeping (nothing declared yet)
    ASSERT(tick_until_widget_sweeping(g, 8), "power meter did not start sweeping after the start press");
    scripted_tap(
        g, HUMAN_PAD, KEY_2
    ); // click during the power sweep: lock power → POWER, windup begins, aim meter starts

    // The aim meter descends from the right; tick until the cursor falls near the strike sweet-spot (≈FOCAL
    // from the left), then click → a strike-zone aim.
    ClientInputState* ci = scripted_state(g)->clientInput;
    int guard = 0;
    while (ci->pitchWidget.meter.dir != 0 &&
           ci->pitchWidget.meter.counter * 100 > ci->pitchWidget.meter.counter_max * 35 && guard++ < 400) {
        scripted_tick(g);
    }
    scripted_tap(g, HUMAN_PAD, KEY_2); // click during the aim descent: lock aim near the plate (strike)

    // Tick to the windup end → release; capture the launch the frame the ball goes airborne.
    MatchSession* m = scripted_match(g);
    float launch_vx = 999.0f;
    int released = 0;
    for (int i = 0; i < 200 && !released; i++) {
        scripted_tick(g);
        if (m->pRAI.pitch_state == PITCH_STAGE_AIRBORNE) {
            launch_vx = m->ballInfo.velocity.x;
            released = 1;
        }
    }

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT(released, "the power+aim dance must release a real pitch");
    // Sweet-spot-timed aim → a strike-zone pitch (direction near 0 → small horizontal velocity).
    ASSERT(fabs(launch_vx) < 0.1f, "sweet-spot-timed aim click must aim a strike-zone pitch (small vx)");
    return TEST_PASSED;
}

// 2. Press → power click → no aim click → valesyöttö: the pitcher keeps the ball, no pitch is released.
int test_scripted_pitch_dropped_aim_is_valesyotto(void)
{
    ScriptedGame* g = scripted_create(0, 1, CONTROL_AI, HUMAN_PAD, 0x9117C402u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_pitch_ready(g, 8000), "never reached a human pitch-ready state");

    MatchSession* m = scripted_match(g);
    int pitcher = m->pII.catcherOnBaseIndex[0];

    scripted_tap(g, HUMAN_PAD, KEY_2); // press: start the power meter sweeping
    ASSERT(tick_until_widget_sweeping(g, 8), "power meter did not start sweeping after the start press");
    scripted_tap(g, HUMAN_PAD, KEY_2); // click during the power sweep: lock power → POWER, windup begins
    // Deliberately never click the aim. Tick well past the (power-dependent) windup deadline.
    int wentAirborne = 0;
    for (int i = 0; i < 250; i++) {
        scripted_tick(g);
        if (m->pRAI.pitch_state == PITCH_STAGE_AIRBORNE) wentAirborne = 1;
    }

    int keptBall = (m->pII.hasBallIndex == pitcher);
    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT(!wentAirborne, "dropping the aim must NOT release a pitch");
    ASSERT(keptBall, "valesyöttö: the pitcher keeps the ball");
    return TEST_PASSED;
}

// 3. After a HUMAN pitch, the AI batter's hit must actually FLY — regression guard for the bug where the
// pitch widget stayed "active" past release (checkPitch goes quiet once the ball leaves the hand), so
// the shared meter kept showing it and never advanced the batting meter → the swing built ~0 power → the bat
// "hit" the ball with near-zero velocity and it floated at the plate. AI-vs-AI never hit this (no widget),
// so only a human-pitch → AI-bat path catches it.
int test_scripted_human_pitch_ai_hit_flies(void)
{
    ScriptedGame* g = scripted_create(0, 1, CONTROL_AI, HUMAN_PAD, 0x9117C403u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_pitch_ready(g, 8000), "never reached a human pitch-ready state");
    ClientInputState* ci = scripted_state(g)->clientInput;
    MatchSession* m = scripted_match(g);

    // Pitch a medium-power, centred strike (the band the AI hits well): start the power meter, click near
    // mid-sweep for ~0.5 power, then click the aim at the sweet-spot.
    scripted_tap(g, HUMAN_PAD, KEY_2);
    ASSERT(tick_until_widget_sweeping(g, 8), "power meter did not start sweeping");
    int guard = 0;
    while (ci->pitchWidget.meter.dir == 1 &&
           ci->pitchWidget.meter.counter * 100 < ci->pitchWidget.meter.counter_max * 50 && guard++ < 200) {
        scripted_tick(g);
    }
    scripted_tap(g, HUMAN_PAD, KEY_2); // lock ~mid power → windup begins
    guard = 0;
    while (ci->pitchWidget.meter.dir != 0 &&
           ci->pitchWidget.meter.counter * 100 > ci->pitchWidget.meter.counter_max * 35 && guard++ < 400) {
        scripted_tick(g);
    }
    scripted_tap(g, HUMAN_PAD, KEY_2); // lock aim near the plate (strike)

    // Wait for release, then watch the ball: a real hit sends it flying (large horizontal speed); the bug
    // leaves it floating (horizontal speed ≈ the pitch's ≈0). No one fields it (the catching team is the
    // idle human), so only the bat can give it horizontal speed.
    int airborne = 0;
    float max_horiz_speed = 0.0f;
    for (int i = 0; i < 400; i++) {
        scripted_tick(g);
        if (m->pRAI.pitch_state == PITCH_STAGE_AIRBORNE) airborne = 1;
        if (airborne) {
            float vx = m->ballInfo.velocity.x, vz = m->ballInfo.velocity.z;
            float horiz = sqrtf(vx * vx + vz * vz);
            if (horiz > max_horiz_speed) max_horiz_speed = horiz;
        }
    }

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT(airborne, "the human pitch must release");
    // A floated hit has horizontal speed ≈ the pitch's (~0); a real batted ball is many times faster.
    ASSERT(max_horiz_speed > 0.15f, "the AI's hit off a human pitch must FLY, not float at the plate");
    return TEST_PASSED;
}

// 4. The catching side's bar, as the human pitcher sees it, across one whole gesture. It is answered by
// the input layer from the gesture itself (catching_widget_view) — there is no engine copy of it any
// more — so this holds what that copy used to promise, every frame: the bar rests at its left end until
// the human starts, the cursor IS the sweep's level while it rises, the aim cursor descends from the far
// end once power is locked, and the bar is back at rest once the engine has resolved the pitch.
static float widget_level(const InputWidget* w)
{
    return (float)w->counter / (float)w->counter_max;
}

int test_scripted_catching_bar_follows_the_pitch_gesture(void)
{
    ScriptedGame* g = scripted_create(0, 1, CONTROL_AI, HUMAN_PAD, 0x9117C404u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_pitch_ready(g, 8000), "never reached a human pitch-ready state");
    const ClientInputState* ci = scripted_state(g)->clientInput;

    ASSERT_FLOAT_EQ(0.0f, catching_widget_view(ci), 1e-6f, "before the gesture the bar rests at its left end");

    scripted_tap(g, HUMAN_PAD, KEY_2); // start the power sweep
    ASSERT(tick_until_widget_sweeping(g, 8), "power meter did not start sweeping after the start press");
    float previous = catching_widget_view(ci);
    int rose = 0;
    for (int i = 0; i < 6; i++) {
        scripted_tick(g);
        const float now = catching_widget_view(ci);
        ASSERT_FLOAT_EQ(widget_level(&ci->pitchWidget.meter), now, 1e-6f, "the bar shows the power sweep's own level");
        if (now > previous) rose = 1;
        previous = now;
    }
    ASSERT(rose, "the power cursor moves while it sweeps");

    scripted_tap(g, HUMAN_PAD, KEY_2); // lock power: the windup begins and the aim descent arms
    ASSERT_EQ(WIDGET_DESCENT, (int)ci->pitchWidget.meter.mode, "locking power arms the aim descent");
    previous = catching_widget_view(ci);
    int fell = 0;
    for (int i = 0; i < 6; i++) {
        scripted_tick(g);
        const float now = catching_widget_view(ci);
        ASSERT_FLOAT_EQ(widget_level(&ci->pitchWidget.meter), now, 1e-6f, "the bar shows the aim descent's own level");
        if (now < previous) fell = 1;
        previous = now;
    }
    ASSERT(fell, "the aim cursor descends");

    // Never click the aim: the descent runs out, the engine fakes the pitch and clears it, and the
    // gesture is over. The bar must come back to rest rather than hang on the last level it showed.
    int rested = 0;
    for (int i = 0; i < 300 && !rested; i++) {
        scripted_tick(g);
        rested = (ci->pitchWidget.meter.mode == WIDGET_IDLE && catching_widget_view(ci) == 0.0f);
    }

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT(rested, "once the pitch is resolved the bar is back at its left end");
    return TEST_PASSED;
}
