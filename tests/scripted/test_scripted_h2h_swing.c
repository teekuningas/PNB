#include "test_helpers.h"
#include "scripted_harness.h"
#include "all_scripted.h"
#include "actions/batting_system.h"
#include "actions_pure/swing_geometry.h"
#include "actions_pure/pitching_physics.h"
#include <math.h>

/*
 * TWO PADS, ONE PITCH — the four-beat dance driven by two people at one screen.
 *
 * Every other scripted test drives one human against the AI, so each side's beats are proved
 * separately: the pitcher's power-then-aim in test_scripted_pitch, the batter's power-then-elevation
 * in test_scripted_swing. Separately is not the claim the design makes. The claim is that the two
 * gestures INTERLEAVE inside one pitch — pitcher power, batter power, pitcher aim, batter elevation —
 * and nothing had ever run them together.
 *
 * What only this file can say:
 *   1. The four beats fit. The batter's power sweep is armed by the pitcher's windup and is still
 *      running when the pitcher commits an aim, so both humans are mid-gesture at the same time.
 *      The unit tier proves the ARITHMETIC fits (2*SWING_POWER_SWEEP_FRAMES <= the shortest windup);
 *      only a real pipeline with two pads proves the two widgets actually reach that state.
 *   2. Two producers may declare on the SAME FRAME. Order within the CONTROL stage is free by
 *      construction (the two write disjoint per-team channels) — this is that property, executed.
 *   3. One ClientInputState serves two pads without the widgets colliding. The pitch widget and the
 *      swing widget both sample KEY_2 on a press edge; each belongs to one side of play, and a press
 *      on the pitcher's pad must never arm or sample the batter's meter.
 *
 * It is also the closest headless proxy for the network case the whole refactor exists for: two
 * independent producers, each with private widget memory, declaring values into one engine within one
 * tick. What a peer changes is where the messages come from, not what the engine does with them.
 */

#define BAT_PAD CONTROL_PLAYER_1 /* team 0 bats first (GameSetup.playsFirst = 0) */
#define PITCH_PAD CONTROL_PLAYER_2 /* team 1 is in the field */

/* One press edge on one pad, without ticking a second frame: the caller decides when the release
 * lands, which is what lets two pads share a frame. */
static void press(ScriptedGame* g, int pad, int key)
{
    scripted_hold(g, pad, key);
    scripted_tick(g);
    scripted_release(g, pad, key);
}

static int tick_until_batting(ScriptedGame* g, int budget)
{
    if (!scripted_tick_until_batter_decision(g, budget)) return 0;
    scripted_tap(g, BAT_PAD, KEY_2); // accept the highlighted candidate (sampled on the release edge)
    for (int i = 0; i < budget; i++) {
        scripted_tick(g);
        if (scripted_match(g)->pRAI.batting_going_on == 1) return 1;
    }
    return 0;
}

/* The human pitcher holds the ball, controls the pitcher, and no pitch is in progress. */
static int tick_until_pitch_ready(ScriptedGame* g, int budget)
{
    for (int i = 0; i < budget; i++) {
        MatchSession* m = scripted_match(g);
        int p = m->pII.catcherOnBaseIndex[0];
        if (p != -1 && m->pII.hasBallIndex == p && m->pII.controlIndex == p && m->pRAI.batter_ready == 1 &&
            m->pendingActionState.current_catching_action == CATCHING_ACTION_NONE &&
            m->pRAI.pitch_state == PITCH_STAGE_NONE) {
            return 1;
        }
        if (!scripted_tick(g)) return 0;
    }
    return 0;
}

/* Returning the ball to the pitcher between pitches is a CONTROLLER decision, not an engine
 * behaviour: the catching AI declares a throw home, and with a human on that pad a person must do
 * the same. So a two-human rally needs this between pitches, and a test that omitted it would stop
 * after one pitch believing it had found a defect. Hold KEY_2 + KEY_DOWN to gather toward home,
 * release KEY_2 to fire at the charge the hold reached. */
static int throw_the_ball_home(ScriptedGame* g, int budget)
{
    MatchSession* m = scripted_match(g);
    int pitcher = m->pII.catcherOnBaseIndex[0];
    if (pitcher == -1) return 0;
    if (m->pII.hasBallIndex == pitcher) return 1; // already there

    scripted_hold(g, PITCH_PAD, KEY_2);
    scripted_hold(g, PITCH_PAD, KEY_DOWN);
    scripted_tick(g); // gather toward home
    scripted_run(g, 12); // charge
    scripted_release(g, PITCH_PAD, KEY_2);
    scripted_tick(g); // release edge -> COMMITTED; the engine times the release
    scripted_release(g, PITCH_PAD, KEY_DOWN);

    for (int i = 0; i < budget; i++) {
        scripted_tick(g);
        if (scripted_match(g)->pII.hasBallIndex == pitcher) return 1;
    }
    return 0;
}

/**
 * 1. The whole dance, both sides human, on one pitch.
 *
 * The load-bearing assertion is not that a ball was hit — it is WHEN each declaration landed. The
 * batter's power is asserted to arrive while the pitcher's declaration is still PITCH_DECL_POWER,
 * i.e. with the pitcher's own gesture unfinished. That is the interleave; a game where the batter
 * could only act after the pitcher had finished would pass every other test in this tier.
 */
int test_scripted_h2h_four_beats_on_one_pitch(void)
{
    ScriptedGame* g = scripted_create(0, 1, BAT_PAD, PITCH_PAD, 0x2FACE001u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_batting(g, 1200), "the human batting side never took the bat");
    ASSERT(tick_until_pitch_ready(g, 1200), "the human pitching side never got the ball at the plate");

    ClientInputState* ci = scripted_state(g)->clientInput;
    MatchSession* m = scripted_match(g);

    /* BEAT 1 — the pitcher starts a power ping-pong, then commits a power. The commit is what starts
     * the engine windup, and the windup is what arms the batter. */
    press(g, PITCH_PAD, KEY_2); // start the pitcher's power meter (client-local; nothing declared)
    scripted_tick(g);
    ASSERT(ci->pitchWidget.dir != 0, "the pitcher's power meter must be sweeping");
    ASSERT_EQ(
        (int)WIDGET_IDLE, (int)ci->swingWidget.meter.mode,
        "the batter's meter must NOT arm off the pitcher's own client-local sweep"
    );
    scripted_run(g, 8);
    press(g, PITCH_PAD, KEY_2); // commit the toss power -> the engine windup begins
    scripted_tick(g);
    ASSERT_EQ(
        (int)PITCH_DECL_POWER, (int)m->pendingActionState.pitchDeclaration.phase,
        "the pitcher's power click must declare a power and leave the aim outstanding"
    );
    ASSERT_EQ((int)PITCH_STAGE_WINDUP, (int)m->pRAI.pitch_state, "committing a power must start the engine windup");

    /* BEAT 2 — the batter's power meter arms itself off that windup, with no key pressed, and a
     * press samples it. The pitcher's aim is still outstanding while this happens. */
    ASSERT_EQ((int)WIDGET_PING_PONG, (int)ci->swingWidget.meter.mode, "the windup must arm the batter's power meter");
    scripted_run(g, 10);
    press(g, BAT_PAD, KEY_2);
    scripted_tick(g);
    ASSERT_EQ(1, m->pendingActionState.swing.powerActive, "the batter's press during the windup must declare a power");
    ASSERT_EQ(
        (int)PITCH_DECL_POWER, (int)m->pendingActionState.pitchDeclaration.phase,
        "THE INTERLEAVE: the batter commits while the pitcher's gesture is still unfinished"
    );

    /* BEAT 3 — the pitcher's aim meter has been descending throughout. Press it near the sweet spot
     * for a pitch over the plate. */
    int guard = 0;
    while (ci->pitchWidget.dir != 0 && ci->pitchWidget.counter * 100 > ci->pitchWidget.counter_max * 35 &&
           guard++ < 400) {
        scripted_tick(g);
    }
    ASSERT(ci->pitchWidget.dir != 0, "the pitcher's aim meter ran out before it could be aimed");
    press(g, PITCH_PAD, KEY_2);
    scripted_tick(g);
    ASSERT_EQ(
        (int)PITCH_DECL_AIMED, (int)m->pendingActionState.pitchDeclaration.phase,
        "the aim click must complete the pitch"
    );

    /* The release is the engine's to time, not either human's. */
    int airborne = 0;
    for (int i = 0; i < 250 && !airborne; i++) {
        scripted_tick(g);
        airborne = (m->pRAI.pitch_state == PITCH_STAGE_AIRBORNE);
    }
    ASSERT(airborne, "the two-click pitch must release");
    scripted_tick(g);

    /* BEAT 4 — the batter's elevation meter sizes itself to the flight and is walked to the sweet
     * spot. Nothing here reads the engine's contact frame; the widget solved the same ball. */
    ASSERT_EQ(
        (int)WIDGET_DESCENT, (int)ci->swingWidget.meter.mode, "the ball being up must arm the batter's elevation meter"
    );
    guard = 0;
    while (ci->swingWidget.meter.dir != 0 && guard++ < 400) {
        // The marker's level IS the value it will declare — the bar maps straight to [0,1].
        float here = (float)ci->swingWidget.meter.counter / (float)ci->swingWidget.meter.counter_max;
        if (here <= SWING_VERTICAL_FOCAL) break;
        scripted_tick(g);
    }
    press(g, BAT_PAD, KEY_2);
    scripted_tick(g);
    ASSERT_EQ(1, m->pendingActionState.swing.verticalActive, "the press during the descent must declare an elevation");
    ASSERT(
        fabsf(m->pendingActionState.swing.vertical - SWING_VERTICAL_FOCAL) < 0.05f,
        "a press at the sweet spot must declare a value at the sweet spot"
    );

    /* And the bat meets the ball. Nobody fields it — the catching human presses nothing from here —
     * so only the bat can give the ball horizontal speed. */
    float max_horiz = 0.0f;
    for (int i = 0; i < 300; i++) {
        scripted_tick(g);
        float vx = m->ballInfo.velocity.x, vz = m->ballInfo.velocity.z;
        float horiz = sqrtf(vx * vx + vz * vz);
        if (horiz > max_horiz) max_horiz = horiz;
    }

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT(max_horiz > 0.15f, "a human pitch met by a human swing at the sweet spot must FLY");
    return TEST_PASSED;
}

/**
 * 2. Both producers declare on the SAME FRAME, and both are ingested.
 *
 * Order within the CONTROL stage is free by construction, and true by inspection — every human check
 * returns early for a team it does not drive, so the two producers write disjoint per-team channels
 * and cannot observe each other. This executes it: one frame, two press edges, two channels, one
 * INGEST drain, both declarations in the world.
 *
 * On a wire this is the ordinary case rather than the exotic one: two peers' messages for tick T
 * arrive together and are drained together, so a game where simultaneity was not expressible would
 * have to invent an ordering the engine is not allowed to have.
 */
int test_scripted_h2h_both_pads_may_declare_on_one_frame(void)
{
    ScriptedGame* g = scripted_create(0, 1, BAT_PAD, PITCH_PAD, 0x2FACE002u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_batting(g, 1200), "the human batting side never took the bat");
    ASSERT(tick_until_pitch_ready(g, 1200), "the human pitching side never got the ball at the plate");

    ClientInputState* ci = scripted_state(g)->clientInput;
    MatchSession* m = scripted_match(g);

    press(g, PITCH_PAD, KEY_2);
    scripted_run(g, 9);
    press(g, PITCH_PAD, KEY_2); // power committed -> windup -> the batter's meter arms
    scripted_tick(g);
    ASSERT_EQ((int)PITCH_STAGE_WINDUP, (int)m->pRAI.pitch_state, "the windup must have begun");
    ASSERT_EQ((int)WIDGET_PING_PONG, (int)ci->swingWidget.meter.mode, "the batter's power meter must be armed");

    // Let the pitcher's aim fall to the sweet spot; the batter's power sweep is still running,
    // which is the whole point — both gestures are live on the same frame.
    int guard = 0;
    while (ci->pitchWidget.dir != 0 && ci->pitchWidget.counter * 100 > ci->pitchWidget.counter_max * 35 &&
           guard++ < 400) {
        scripted_tick(g);
    }
    ASSERT(ci->pitchWidget.dir != 0, "the pitcher's aim meter ran out before the shared frame");
    ASSERT(ci->swingWidget.meter.dir != 0, "the batter's power meter must still be live on the shared frame");

    // ONE FRAME, TWO PRESS EDGES.
    scripted_hold(g, BAT_PAD, KEY_2);
    scripted_hold(g, PITCH_PAD, KEY_2);
    scripted_tick(g);
    scripted_release(g, BAT_PAD, KEY_2);
    scripted_release(g, PITCH_PAD, KEY_2);
    scripted_tick(g);

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    int powerActive = m->pendingActionState.swing.powerActive;
    int phase = (int)m->pendingActionState.pitchDeclaration.phase;
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT_EQ(1, powerActive, "the batting side's declaration must survive a frame it shares with the other team");
    ASSERT_EQ((int)PITCH_DECL_AIMED, phase, "the catching side's declaration must survive it too");
    return TEST_PASSED;
}

/**
 * 3. One ClientInputState, two pads, no crosstalk.
 *
 * The pitch widget and the swing widget both sample KEY_2 on a press edge, and there is exactly one
 * ClientInputState in the process — it covers whichever teams this machine drives. What keeps them
 * apart is not a pad index inside the widgets but the fact that each belongs to one SIDE of play, and
 * action_invocations hands each check its own side's control mode. This is that separation, executed
 * from both directions.
 */
int test_scripted_h2h_one_pads_key_never_moves_the_others_widget(void)
{
    ScriptedGame* g = scripted_create(0, 1, BAT_PAD, PITCH_PAD, 0x2FACE003u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_batting(g, 1200), "the human batting side never took the bat");
    ASSERT(tick_until_pitch_ready(g, 1200), "the human pitching side never got the ball at the plate");

    ClientInputState* ci = scripted_state(g)->clientInput;
    MatchSession* m = scripted_match(g);

    // The BATTER hammers the action key with no pitch in progress. Nothing may be declared and no
    // meter may arm: swing_may_be_declared is false, and the pitcher's widget is not this pad's.
    for (int i = 0; i < 6; i++) {
        press(g, BAT_PAD, KEY_2);
        scripted_tick(g);
    }
    ASSERT_EQ((int)WIDGET_IDLE, (int)ci->swingWidget.meter.mode, "with no pitch in progress the batter has no meter");
    ASSERT_EQ(0, m->pendingActionState.swing.powerActive, "a batter cannot declare a power before a pitch exists");
    ASSERT_EQ(
        (int)WIDGET_IDLE, (int)ci->pitchWidget.mode, "the batting pad's key must never touch the pitching widget"
    );
    ASSERT_EQ(
        (int)PITCH_DECL_IDLE, (int)m->pendingActionState.pitchDeclaration.phase,
        "the batting pad's key must never declare a pitch"
    );

    // And the other way: the PITCHER runs a whole gesture, and the batter's own widget only ever
    // responds to the physical windup, never to the pitcher's key.
    press(g, PITCH_PAD, KEY_2);
    scripted_tick(g);
    ASSERT(ci->pitchWidget.dir != 0, "the pitching pad's key must start the pitching widget");
    ASSERT_EQ(
        (int)WIDGET_IDLE, (int)ci->swingWidget.meter.mode,
        "the pitcher's pre-windup sweep is client-local — it must be invisible to the batter"
    );

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);
    ASSERT(!failed, reason);
    return TEST_PASSED;
}

/**
 * 4. The dance REPEATS — several pitches, both sides human, with the validator watching.
 *
 * The three tests above each drive one pitch, and one pitch cannot see the bug family that has cost
 * this codebase the most: a widget that does not retire. Bug #4 was exactly that — a pitch widget
 * left "active" past release, starving the other side's meter — and it was invisible to AI-vs-AI
 * because the AI arms no widget at all. With two humans there are two widgets to leave behind, and
 * the batter's is armed by a physical event rather than by its own key, so a stale one is armed
 * again on the next windup with the previous pitch's state still in it.
 *
 * The batter deliberately never puts a ball in play here (a withdrawal each time, and once a plain
 * silence), because a batted ball no fielder chases would end the test rather than continue it. What
 * is being proved is the CYCLE: pitch, swing, resolve, and everything on both sides back to a state
 * the next pitch can arm from. The production state validator runs every frame throughout.
 */
int test_scripted_h2h_the_dance_repeats_across_pitches(void)
{
    ScriptedGame* g = scripted_create(0, 1, BAT_PAD, PITCH_PAD, 0x2FACE004u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_batting(g, 1200), "the human batting side never took the bat");

    ClientInputState* ci = scripted_state(g)->clientInput;
    MatchSession* m = scripted_match(g);

    int pitches = 0;
    for (int round = 0; round < 5; round++) {
        if (round > 0 && !throw_the_ball_home(g, 600)) break;
        if (!tick_until_pitch_ready(g, 2000)) break;
        if (scripted_match(g)->pRAI.batting_going_on != 1 && !tick_until_batting(g, 1200)) break;

        // Every pitch must find both widgets idle. A stale one is the whole point of this test.
        ASSERT_EQ((int)WIDGET_IDLE, (int)ci->pitchWidget.mode, "a new pitch must find the pitch widget retired");
        ASSERT_EQ((int)WIDGET_IDLE, (int)ci->swingWidget.meter.mode, "a new pitch must find the swing widget retired");
        ASSERT_EQ((int)SWING_BEAT_POWER, (int)ci->swingWidget.beat, "a new pitch must find the gesture back at beat 1");
        ASSERT_EQ(0, m->pendingActionState.swing.consumed, "a new pitch must find the previous swing's record cleared");

        press(g, PITCH_PAD, KEY_2);
        scripted_run(g, 7);
        press(g, PITCH_PAD, KEY_2); // power -> windup
        scripted_tick(g);
        // Asked of the engine's pitch mutex, not of pitch_state: from the second pitch of a
        // half-inning onward, consolidation legitimately holds pitch_state at NONE through the
        // windup while the previous pitch's result is still standing. Probing the wrong one of
        // those two is the defect this soak found on the batting side.
        if (m->pendingActionState.current_catching_action != CATCHING_ACTION_PITCHING) break;
        pitches++;

        int guard = 0;
        while (ci->pitchWidget.dir != 0 && ci->pitchWidget.counter * 100 > ci->pitchWidget.counter_max * 35 &&
               guard++ < 400) {
            scripted_tick(g);
        }
        press(g, PITCH_PAD, KEY_2); // aim -> AIMED
        scripted_tick(g);

        if (round % 2 == 0) {
            // Load, then think better of it — the "väärä!" call, twice over the run.
            scripted_run(g, 6);
            press(g, BAT_PAD, KEY_2);
            scripted_tick(g);
            ASSERT_EQ((int)SWING_BEAT_LOADED, (int)ci->swingWidget.beat, "the press must load the gesture");
            scripted_run(g, 4);
            press(g, BAT_PAD, KEY_1); // withdraw
            scripted_tick(g);
            ASSERT_EQ(1, m->pendingActionState.batting_stopped, "the withdrawal must stop the swing");
        }
        // ...and on the other rounds the batter says nothing at all, which is a complete answer.

        // Let the pitch resolve and the batting cycle come back round.
        for (int i = 0; i < 400; i++) {
            scripted_tick(g);
            if (m->pRAI.pitch_state == PITCH_STAGE_NONE && m->pRAI.batting_going_on == 0) break;
        }
        scripted_run(g, 5);
    }

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    long frames = scripted_frame(g);
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT(pitches >= 3, "the two-human cycle must survive several pitches, not just the first");
    ASSERT(frames > 500, "the soak must actually have run");
    return TEST_PASSED;
}
