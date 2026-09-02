#include "test_helpers.h"
#include "scripted_harness.h"
#include "all_scripted.h"
#include "actions/batting_system.h"
#include "actions_pure/swing_geometry.h"
#include <math.h>

#define HUMAN_PAD CONTROL_PLAYER_1 /* the batting team (team 0) is the human */

/*
 * The human SWING, headless — the whole gesture driven through a real KeyStates and the real
 * action_invocations, with no screen and no AI on the batting side.
 *
 * The contract tier knights what the engine does with a declared power and elevation once they
 * arrive. What only this tier can say is whether a person pressing a key produces them at all: the
 * meter that arms itself off the pitcher's windup, the press that samples it, the second meter that
 * sizes itself to the flight the batter actually has left, and the withdrawal.
 */

static int reach_the_batting_phase(ScriptedGame* g, int budget)
{
    if (!scripted_tick_until_batter_decision(g, budget)) return 0;
    scripted_tap(g, HUMAN_PAD, KEY_2); // accept the highlighted candidate → INTENT_SELECT_BATTER
    for (int i = 0; i < budget; i++) {
        scripted_tick(g);
        if (scripted_match(g)->pRAI.batting_going_on == 1) return 1;
    }
    return 0;
}

// Tick until the AI pitcher has begun its windup, which is when the batter's power meter arms.
static int tick_until_windup(ScriptedGame* g, int budget)
{
    for (int i = 0; i < budget; i++) {
        if (scripted_match(g)->pRAI.pitch_state == PITCH_STAGE_WINDUP) return 1;
        if (!scripted_tick(g)) return 0;
    }
    return 0;
}

static int tick_until_airborne(ScriptedGame* g, int budget)
{
    for (int i = 0; i < budget; i++) {
        if (scripted_match(g)->pRAI.pitch_state == PITCH_STAGE_AIRBORNE) return 1;
        if (!scripted_tick(g)) return 0;
    }
    return 0;
}

/**
 * The power meter arms itself off the WINDUP, and a press declares a power.
 *
 * Arming without a key is the point: the batter has no way to say "I am ready" and needs none — the
 * pitcher beginning to crouch is the cue, and the crouch is the toss height made physical. It is the
 * only channel the fourth law leaves open for the dance, and it is what makes the four beats
 * possible (pitcher power, batter power, pitcher aim, batter elevation) with two people at one
 * screen.
 */
int test_scripted_swing_power_meter_arms_on_the_windup(void)
{
    ScriptedGame* g = scripted_create(0, 1, HUMAN_PAD, CONTROL_AI, 0x5A1F00Du);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(reach_the_batting_phase(g, 900), "the human batting side never reached the batting phase");
    ASSERT(tick_until_windup(g, 900), "the AI pitcher never began a windup");

    ClientInputState* ci = scripted_state(g)->clientInput;
    scripted_tick(g);
    ASSERT_EQ(
        (int)WIDGET_PING_PONG_LOOP, (int)ci->swingWidget.meter.mode,
        "the power meter must arm itself on the windup, with no key pressed"
    );
    ASSERT(ci->swingWidget.meter.dir != 0, "an armed power meter must be sweeping");

    // Let it sweep somewhere off zero, then press: the level under the marker becomes the power.
    scripted_run(g, 9);
    int level = ci->swingWidget.meter.counter;
    ASSERT(level > 0, "the power meter must have travelled before it is sampled");
    scripted_tap(g, HUMAN_PAD, KEY_2);

    MatchSession* m = scripted_match(g);
    ASSERT_EQ(1, m->pendingActionState.swing.powerActive, "a press during the windup must declare a power");
    ASSERT(
        m->pendingActionState.swing.power > 0.0f && m->pendingActionState.swing.power <= 1.0f,
        "the declared power must be the meter's level as a value in [0,1]"
    );
    // And the client keeps its own copy rather than reading the engine's back.
    ASSERT(
        fabsf(ci->swingWidget.power - m->pendingActionState.swing.power) < 0.0001f,
        "the widget must remember what it declared — it never asks the engine"
    );
    scripted_destroy(g);
    return TEST_PASSED;
}

/**
 * Once the ball is up, the elevation meter arms and sizes itself to the flight that is left, and a
 * press declares the value the human can see.
 *
 * The sizing is what the lead is for: the sweep finishes a margin of frames BEFORE contact, so the
 * declaration is in the world before the frame that consumes it. On a wire that margin is what a
 * late message gets to spend.
 */
int test_scripted_swing_elevation_meter_fits_inside_the_flight(void)
{
    ScriptedGame* g = scripted_create(0, 1, HUMAN_PAD, CONTROL_AI, 0x5A1F00Du);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(reach_the_batting_phase(g, 900), "the human batting side never reached the batting phase");
    ASSERT(tick_until_windup(g, 900), "the AI pitcher never began a windup");

    scripted_run(g, 6);
    scripted_tap(g, HUMAN_PAD, KEY_2); // declare a power during the windup
    ASSERT(tick_until_airborne(g, 900), "the pitch never left the pitcher's hand");
    scripted_tick(g);

    ClientInputState* ci = scripted_state(g)->clientInput;
    MatchSession* m = scripted_match(g);
    ASSERT_EQ(
        (int)WIDGET_DESCENT, (int)ci->swingWidget.meter.mode, "the elevation meter must arm when the ball is in the air"
    );
    // What this tier can honestly claim: the sweep the client armed fits inside the flight the engine
    // is timing, so the two agree about the same ball without the client ever reading the engine's
    // contact frame. The LEAD itself is not asserted here — it only binds on the lowest tosses, and
    // the AI pitcher never throws one, so an assertion about it could not be shown a state it would
    // reject. It is pinned in the unit tier instead, across every pitch height, where it does bind.
    ASSERT(
        ci->swingWidget.meter.counter_max <= m->pendingActionState.swing.contactFrame,
        "the client's elevation sweep must fit inside the flight the engine solved for the same ball"
    );

    // Walk the marker down to the sweet spot and press there.
    float top = swing_marker_top(ci->swingWidget.power);
    int guard = 0;
    while (ci->swingWidget.meter.dir != 0 && guard++ < 400) {
        float here = top * (float)ci->swingWidget.meter.counter / (float)ci->swingWidget.meter.counter_max;
        if (here <= SWING_VERTICAL_FOCAL) break;
        scripted_tick(g);
    }
    scripted_tap(g, HUMAN_PAD, KEY_2);

    ASSERT_EQ(1, m->pendingActionState.swing.verticalActive, "a press during the descent must declare an elevation");
    ASSERT(
        fabsf(m->pendingActionState.swing.vertical - SWING_VERTICAL_FOCAL) < 0.05f,
        "pressing as the marker crosses the sweet spot must declare a value at the sweet spot"
    );
    scripted_destroy(g);
    return TEST_PASSED;
}

/**
 * The "väärä!" call: a power already committed, then withdrawn with the other key, and the bat does
 * not come. Worth a ball where swinging and missing would be worth a strike — which is why the
 * withdrawal is a message a human can send and not merely something they fail to do.
 */
int test_scripted_swing_can_be_withdrawn_after_the_power_is_committed(void)
{
    ScriptedGame* g = scripted_create(0, 1, HUMAN_PAD, CONTROL_AI, 0x5A1F00Du);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(reach_the_batting_phase(g, 900), "the human batting side never reached the batting phase");
    ASSERT(tick_until_windup(g, 900), "the AI pitcher never began a windup");

    scripted_run(g, 6);
    scripted_tap(g, HUMAN_PAD, KEY_2); // commit a power
    MatchSession* m = scripted_match(g);
    ASSERT_EQ(1, m->pendingActionState.swing.powerActive, "the swing must be committed before it is withdrawn");
    ASSERT(tick_until_airborne(g, 900), "the pitch never left the pitcher's hand");

    scripted_tap(g, HUMAN_PAD, KEY_1); // ...and think better of it
    scripted_tick(g);
    ASSERT_EQ(1, m->pendingActionState.batting_stopped, "the withdrawal key must stop the swing");
    ASSERT_EQ((int)BATTING_MODE_STOP, (int)m->pendingActionState.batting_mode, "a withdrawn swing spreads the hands");
    scripted_destroy(g);
    return TEST_PASSED;
}

/**
 * Saying nothing is a complete answer. A human who never presses declares no power, and a batter who
 * declared no power has not swung — so there is no miss to record, and the pitch is judged on where
 * it landed. The meter loops the whole time and needs no deadline of its own.
 */
int test_scripted_swing_silence_is_not_a_miss(void)
{
    ScriptedGame* g = scripted_create(0, 1, HUMAN_PAD, CONTROL_AI, 0x5A1F00Du);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(reach_the_batting_phase(g, 900), "the human batting side never reached the batting phase");
    ASSERT(tick_until_airborne(g, 900), "the pitch never left the pitcher's hand");

    MatchSession* m = scripted_match(g);
    for (int i = 0; i < 300; i++) {
        if (m->pendingActionState.swing.contactFrame >= 0 &&
            m->pendingActionState.batting_frame_count > m->pendingActionState.swing.contactFrame + 2) {
            break;
        }
        scripted_tick(g); // press nothing at all
    }

    ASSERT_EQ(0, m->pendingActionState.swing.powerActive, "pressing nothing must declare nothing");
    ASSERT(
        scripted_state(g)->rules->betweenPitchState.batOutcome != BAT_OUTCOME_MISSED,
        "a batter who never swung must not be recorded as having swung and missed"
    );
    ASSERT_EQ(1, m->pendingActionState.batting_stopped, "the engine must conclude that no swing happened");
    scripted_destroy(g);
    return TEST_PASSED;
}
