#include "test_helpers.h"
#include "scripted_harness.h"
#include "globals.h"
#include <math.h>

/*
 * Scripted-human PITCH test — the human input→intent path for the pitch slice (PLAN.md §5), driven
 * entirely through the real action_invocations via scripted KeyStates (no AI, no direct state pokes).
 *
 * It proves the new 3-click dance: click → start the windup; click → declare power; click → declare aim
 * (and the engine releases a real pitch). Timing the third click at the sampling-widget centre aims the
 * pitch at the plate (a strike-zone pitch). Dropping the third click → valesyöttö (a fake; the pitcher
 * keeps the ball, no pitch is thrown). The catching/pitching team is the human here.
 *
 * INTERIM MECHANIC (PLAN.md §5.7): the 3-click sawtooth is scheduled to be replaced next session by a
 * 2-phase, 2-click meter (power-click starts the pitch; the `WINDUP` phase is dropped). These two tests
 * are deliberately not "knighted as stable" — they prove the human input→intent→actualization PATH works
 * today (a real pitch on full input; valesyöttö on a dropped aim), and will be rewritten with the meter.
 */

#define HUMAN_PAD CONTROL_PLAYER_1 /* the catching (pitching) team is the human */

// One click = a key down frame then a release frame (the declaration fires on the release edge).
static void click(ScriptedGame* g, int pad, int key)
{
    scripted_hold(g, pad, key);
    scripted_tick(g);
    scripted_release(g, pad, key);
    scripted_tick(g);
}

// Tick until the human controls the pitcher, holding the ball at home, with a ready batter and no
// catching action in progress — i.e. a windup may legally begin.
static int tick_until_pitch_ready(ScriptedGame* g, int budget)
{
    for (int i = 0; i < budget; i++) {
        scripted_tick(g);
        MatchSession* m = scripted_match(g);
        int p = m->pII.catcherOnBaseIndex[0];
        if (p != -1 && m->pII.hasBallIndex == p && m->pII.controlIndex == p && m->pRAI.batter_ready == 1 &&
            m->pendingActionState.current_catching_action == CATCHING_ACTION_NONE &&
            m->pRAI.pitch_state == PITCH_STAGE_NONE) {
            return 1;
        }
    }
    return 0;
}

// 1. Three clicks (the third at the meter centre) → a real strike-zone pitch is released.
int test_scripted_pitch_three_clicks_strike(void)
{
    ScriptedGame* g = scripted_create(0, 1, CONTROL_AI, HUMAN_PAD, 0x9117C401u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_pitch_ready(g, 8000), "never reached a human pitch-ready state");

    click(g, HUMAN_PAD, KEY_2); // click 1: begin the windup
    click(g, HUMAN_PAD, KEY_2); // click 2: declare power

    // Tick (KEY_2 released) until the sampling widget is at its centre, then click 3 → direction ~ on-plate.
    ClientInputState* ci = scripted_state(g)->clientInput;
    int guard = 0;
    while (ci->pitchWidget.counter_max > 0 && ci->pitchWidget.counter < ci->pitchWidget.counter_max / 2 &&
           guard++ < 200) {
        scripted_tick(g);
    }
    click(g, HUMAN_PAD, KEY_2); // click 3: declare aim (centre = strike zone)

    // Tick to the windup end → release; capture the launch the frame the ball goes airborne.
    MatchSession* m = scripted_match(g);
    float launch_vx = 999.0f;
    int released = 0;
    for (int i = 0; i < 80 && !released; i++) {
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
    ASSERT(released, "the 3-click dance must release a real pitch");
    // Centre-timed aim → a strike-zone pitch (direction near 0 → small horizontal velocity).
    ASSERT(fabs(launch_vx) < 0.1f, "centre-timed third click must aim a strike-zone pitch (small vx)");
    return TEST_PASSED;
}

// 2. Two clicks then no aim → valesyöttö: the pitcher keeps the ball, no pitch is released.
int test_scripted_pitch_dropped_aim_is_valesyotto(void)
{
    ScriptedGame* g = scripted_create(0, 1, CONTROL_AI, HUMAN_PAD, 0x9117C402u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_pitch_ready(g, 8000), "never reached a human pitch-ready state");

    MatchSession* m = scripted_match(g);
    int pitcher = m->pII.catcherOnBaseIndex[0];

    click(g, HUMAN_PAD, KEY_2); // click 1: begin the windup
    click(g, HUMAN_PAD, KEY_2); // click 2: declare power
    // Deliberately never click the aim. Tick well past the windup deadline.
    int wentAirborne = 0;
    for (int i = 0; i < 80; i++) {
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
