#include "test_helpers.h"
#include "scripted_harness.h"
#include "all_scripted.h"
#include "actions/batting_system.h"
#include "rules_pure/player_utils.h" // get_active_batter_index — whose body we are watching
#include "action_invocations.h" // swing_widget_view — what the batter actually sees on the bar
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
        (int)WIDGET_PING_PONG, (int)ci->swingWidget.meter.mode,
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
    int guard = 0;
    while (ci->swingWidget.meter.dir != 0 && guard++ < 400) {
        // The marker's level IS the value it will declare — the bar maps straight to [0,1].
        float here = (float)ci->swingWidget.meter.counter / (float)ci->swingWidget.meter.counter_max;
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

    // This withdrawal lands BEFORE the body has begun any motion, so there is no shape to observe
    // yet — the claim is that when the body does start, it starts spread. There is no stored mode to
    // ask instead: the shape is derived from the declaration wherever it is needed, so the only
    // honest witness is the animation itself.
    int moving = 0;
    for (int i = 0; i < 300 && !moving; i++) {
        scripted_tick(g);
        moving = (m->pendingActionState.batter_moving == 1);
    }
    int batter = get_active_batter_index(m);
    ASSERT(moving && batter != -1, "the batter's body never started its motion");
    ASSERT_EQ(
        PLAYER_ANIM_BAT_SWING_3, m->playerInfo[batter].cPI.model,
        "a swing withdrawn before the motion began must never start as a swing"
    );
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

/**
 * The gesture survives a declaration that has not arrived yet — the netplay property, stated as a
 * test rather than as a comment.
 *
 * Locally a message is ingested inside the tick that produced it, so "have I declared a power?" and
 * "does the engine hold a power?" are the same question and a widget could ask either. Across a wire
 * they are not: a message stamped for tick T+D leaves the engine's copy empty for D ticks after the
 * human pressed. A gesture keyed off the engine's copy would spend those ticks believing nothing had
 * been said — re-arming the one power sweep this pitch offers and inviting a second commitment on
 * the same ball.
 *
 * There is no input delay to point at yet, so the delay is simulated in the only honest way: clear
 * the engine's copy immediately after the press, exactly as an in-flight message leaves it, and watch
 * the widget. A widget that keeps its own account does not notice. Ablate by keying the arm and the
 * sample on match->pendingActionState.swing.powerActive again, and this goes red while every other
 * test in the tier stays green — which is the whole point: local play cannot see this.
 */
int test_scripted_swing_gesture_survives_a_declaration_still_in_flight(void)
{
    ScriptedGame* g = scripted_create(0, 1, HUMAN_PAD, CONTROL_AI, 0x5A1F00Eu);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(reach_the_batting_phase(g, 900), "the human batting side never reached the batting phase");
    ASSERT(tick_until_windup(g, 900), "the AI pitcher never began a windup");

    ClientInputState* ci = scripted_state(g)->clientInput;
    MatchSession* m = scripted_match(g);

    scripted_run(g, 8);
    scripted_tap(g, HUMAN_PAD, KEY_2); // the human commits a power
    ASSERT_EQ(1, m->pendingActionState.swing.powerActive, "the power must have been declared to begin with");
    ASSERT_EQ((int)SWING_BEAT_LOADED, (int)ci->swingWidget.beat, "the gesture must know it has loaded");
    float declared = m->pendingActionState.swing.power;

    // The wire, simulated: the engine has not seen it yet.
    m->pendingActionState.swing.powerActive = 0;
    m->pendingActionState.swing.power = 0.0f;

    for (int i = 0; i < 20; i++) {
        scripted_tick(g);
        ASSERT(
            ci->swingWidget.meter.mode != WIDGET_PING_PONG,
            "a power sweep must not re-arm while the declaration is still in flight"
        );
        ASSERT_EQ(
            (int)SWING_BEAT_LOADED, (int)ci->swingWidget.beat,
            "the gesture's own account of itself must not depend on the engine having received it"
        );
        ASSERT_EQ(
            0, m->pendingActionState.swing.powerActive, "the widget must not have declared a SECOND power on this ball"
        );
    }
    // And its copy of the value is intact, so the descent it scales is still this batter's.
    ASSERT(fabsf(ci->swingWidget.power - declared) < 0.0001f, "the widget must still hold the power it declared");

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);
    ASSERT(!failed, reason);
    return TEST_PASSED;
}

/**
 * The power beat exists on the SECOND pitch too — and on every one after it.
 *
 * Every other test in this file drives the FIRST pitch of a half-inning, and that blind spot hid a
 * live defect for the whole of the swing slice: the batter's window was asked of `pitch_state ==
 * WINDUP`, which consolidation stamps back to NONE on every frame a resolved pitch's `pitchResult`
 * is still set — and that flag is only cleared when the NEXT pitch is released. So from pitch two
 * onward the crouch simply was not a beat: the batter could declare in 0 of the windup's 87 frames,
 * the power meter never armed, and the four-beat dance the slice was designed around collapsed to
 * two beats for all but the opening pitch of each half-inning.
 *
 * It survived because nothing could see it. AI-vs-AI could not: the AI restates its power every
 * frame, so it simply declared once the ball was airborne and its box score barely moved — the
 * workaround was even written into the controller as a comment about declaring "as early as it
 * legally can". And one-pitch tests could not, by construction.
 *
 * So the claim here is deliberately about the SECOND pitch. Ablate by asking `pitch_state ==
 * PITCH_STAGE_WINDUP` again in swing_may_be_declared: this goes red and the first-pitch tests above
 * all stay green.
 */
int test_scripted_swing_power_beat_survives_into_later_pitches(void)
{
    ScriptedGame* g = scripted_create(0, 1, HUMAN_PAD, CONTROL_AI, 0x5A1F00Fu);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(reach_the_batting_phase(g, 900), "the human batting side never reached the batting phase");

    ClientInputState* ci = scripted_state(g)->clientInput;
    MatchSession* m = scripted_match(g);

    // Sit out the first pitch entirely — declare nothing, and let it resolve. That is what leaves a
    // pitchResult standing, which is the state the next windup has to survive.
    ASSERT(tick_until_windup(g, 900), "the AI pitcher never began a first windup");
    for (int i = 0; i < 900; i++) {
        scripted_tick(g);
        if (scripted_state(g)->rules->betweenPitchState.pitchResult != PITCH_RESULT_NONE) break;
    }
    ASSERT(
        scripted_state(g)->rules->betweenPitchState.pitchResult != PITCH_RESULT_NONE,
        "the first pitch must resolve, leaving the stale flag this test is about"
    );

    // The SECOND windup. Nothing but the pitcher's crouch may be needed to arm the batter.
    int armed = 0;
    for (int i = 0; i < 1200 && !armed; i++) {
        scripted_tick(g);
        if (m->pendingActionState.current_catching_action == CATCHING_ACTION_PITCHING) {
            scripted_tick(g);
            ASSERT(
                swing_may_be_declared(m),
                "the batter must be able to declare during a windup that follows a resolved pitch"
            );
            ASSERT_EQ(
                (int)WIDGET_PING_PONG, (int)ci->swingWidget.meter.mode,
                "the power meter must arm on EVERY windup, not only the half-inning's first"
            );
            armed = 1;
        }
    }
    ASSERT(armed, "a second windup never began");

    // And the beat is usable, not merely visible: a press during it declares a power, before release.
    scripted_run(g, 8);
    scripted_tap(g, HUMAN_PAD, KEY_2);
    ASSERT_EQ(1, m->pendingActionState.swing.powerActive, "a press during the second windup must declare a power");
    ASSERT(
        m->pRAI.pitch_state != PITCH_STAGE_AIRBORNE, "...and it must have been declared BEFORE the ball was released"
    );

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);
    ASSERT(!failed, reason);
    return TEST_PASSED;
}

/**
 * The elevation marker starts at the EXTREME, whatever power was declared.
 *
 * It used to start at a height that rose with the declared power, which is the last thing that
 * coupled the batter's two decisions — the physics has not coupled them since the elevation law
 * collapsed and power cancelled out of it. Two costs, and the second is the one that was felt: the
 * second gesture appeared to begin wherever the first had ended, and the sweet spot arrived at a
 * different moment for every power, so there was no rhythm to learn. A bunt's marker began ON the
 * sweet spot; a full swing's began three times above it.
 *
 * Now the marker crosses the whole bar every time, so the sweet spot passes at a fixed fraction of
 * every sweep — the same fraction, in the same direction, as the pitcher's aim descent.
 *
 * Ablate by scaling the descent by the declared power again: this goes red at the first assertion.
 */
int test_scripted_swing_elevation_marker_always_starts_at_the_extreme(void)
{
    // A LOW power, which is where the old geometry was most obviously wrong: a bunt's marker used to
    // start at the sweet spot itself, leaving nothing to time.
    ScriptedGame* g = scripted_create(0, 1, HUMAN_PAD, CONTROL_AI, 0x5A1F010u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(reach_the_batting_phase(g, 900), "the human batting side never reached the batting phase");
    ASSERT(tick_until_windup(g, 900), "the AI pitcher never began a windup");

    ClientInputState* ci = scripted_state(g)->clientInput;
    MatchSession* m = scripted_match(g);

    // Press almost immediately, while the sweep is still near the bottom: a deliberately weak swing.
    scripted_run(g, 2);
    scripted_tap(g, HUMAN_PAD, KEY_2);
    ASSERT_EQ(1, m->pendingActionState.swing.powerActive, "a power must have been declared");
    ASSERT(m->pendingActionState.swing.power < 0.5f, "this test needs a LOW declared power to be meaningful");

    ASSERT(tick_until_airborne(g, 900), "the pitch never left the pitcher's hand");
    scripted_tick(g);
    ASSERT_EQ((int)WIDGET_DESCENT, (int)ci->swingWidget.meter.mode, "the elevation meter must arm");

    // The marker is at the top of the bar — not at the top of some power-dependent sub-range.
    ASSERT_EQ(
        ci->swingWidget.meter.counter_max, ci->swingWidget.meter.counter,
        "the descent must begin at the extreme, whatever power was declared"
    );
    ASSERT(
        fabsf(swing_widget_view(&ci->swingWidget).cursor - 1.0f) < 0.0001f,
        "and it must READ as the extreme: the value shown is the value that will be declared"
    );

    // Walk it to the sweet spot. The fraction of the sweep this takes is 1 - FOCAL for every power,
    // which is the rhythm the change exists to make learnable.
    int steps = 0;
    while (ci->swingWidget.meter.dir != 0 && steps < 400) {
        if (swing_widget_view(&ci->swingWidget).cursor <= SWING_VERTICAL_FOCAL) break;
        scripted_tick(g);
        steps++;
    }
    float fraction = (float)steps / (float)ci->swingWidget.meter.counter_max;
    ASSERT(
        fabsf(fraction - (1.0f - SWING_VERTICAL_FOCAL)) < 0.05f,
        "the sweet spot must arrive at the same fraction of the sweep as the pitcher's aim does"
    );

    scripted_tap(g, HUMAN_PAD, KEY_2);
    ASSERT(
        fabsf(m->pendingActionState.swing.vertical - SWING_VERTICAL_FOCAL) < 0.05f,
        "and pressing there must declare the sweet spot, for a weak swing as for a full one"
    );

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);
    ASSERT(!failed, reason);
    return TEST_PASSED;
}

/**
 * A batter who has declared nothing does not swing at the ball — and the BODY says so, not just the
 * scoreboard.
 *
 * Saying nothing was always a complete answer and always scored as one: the engine's backstop at
 * contact concludes there was no swing, so the pitch is judged on where it landed. But the shape the
 * body made while the answer was still open was a full swing, because that was the default. So the
 * batter wound up and swung at a ball he had declined, and only on the contact frame — with the
 * motion already committed — did his hands spread. What a person saw was a swing; what the rules
 * recorded was a pass.
 *
 * The shape follows the declaration now, and nothing declared is its own shape. Ablate by returning
 * BATTING_MODE_SWING as the default in swing_shape again.
 */
int test_scripted_swing_a_batter_who_declares_nothing_does_not_swing(void)
{
    ScriptedGame* g = scripted_create(0, 1, HUMAN_PAD, CONTROL_AI, 0x5A1F011u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(reach_the_batting_phase(g, 900), "the human batting side never reached the batting phase");
    ASSERT(tick_until_airborne(g, 900), "the pitch never left the pitcher's hand");

    MatchSession* m = scripted_match(g);

    // Press nothing at all, and catch the frame the body commits to a motion.
    int sawTheBodyStart = 0;
    for (int i = 0; i < 300; i++) {
        scripted_tick(g);
        if (m->pendingActionState.batter_moving == 1) {
            sawTheBodyStart = 1;
            break;
        }
        if (m->pendingActionState.swing.consumed) break;
    }
    ASSERT(sawTheBodyStart, "the batter's body never started its motion, so there is nothing to judge");

    // This is the load-bearing frame: the decision is still OPEN (nothing has stopped the swing yet,
    // and a late power would still re-shape him), and the body is nevertheless not swinging.
    ASSERT_EQ(0, m->pendingActionState.swing.powerActive, "the premise: nothing has been declared");
    ASSERT_EQ(0, m->pendingActionState.batting_stopped, "the premise: the backstop has not fired yet either");
    int batter = get_active_batter_index(m);
    ASSERT(batter != -1, "there must be a batter to look at");
    ASSERT_EQ(
        PLAYER_ANIM_BAT_SWING_3, m->playerInfo[batter].cPI.model,
        "a batter who has declared nothing must not be making a swing with his body"
    );

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);
    ASSERT(!failed, reason);
    return TEST_PASSED;
}

/**
 * The committed power stays on the bar through the WAIT — the batter's own beat of the four.
 *
 * This is the one state the pitcher's gesture does not have. His aim meter arms on the same frame his
 * power locks, so for him "a marker is descending" and "a power is committed" are the same condition
 * and a display keyed on either is right. The batter's second meter cannot arm until the ball is in
 * the air, so between his two beats there is a wait of arbitrary length — and a mark keyed on the
 * meter went dark for the whole of it: visible while the sweep ran, gone at the instant of the press,
 * back only when the ball came up. What a person saw was their decision being forgotten.
 *
 * So the claim is specifically about the frames in between, and it is asserted on EVERY one of them
 * rather than at the ends. The other half of the same modelling error is here too: while the wait
 * lasts there is no moving cursor at all, and a bar that could only report a position had to claim
 * one — parking a meaningless marker at the far left, beside the committed mark.
 *
 * Ablate by keying the static mark on `swingWidget.meter.mode == WIDGET_DESCENT` again.
 */
int test_scripted_swing_committed_power_stays_on_the_bar_until_the_ball_is_up(void)
{
    ScriptedGame* g = scripted_create(0, 1, HUMAN_PAD, CONTROL_AI, 0x5A1F012u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(reach_the_batting_phase(g, 900), "the human batting side never reached the batting phase");
    ASSERT(tick_until_windup(g, 900), "the AI pitcher never began a windup");

    ClientInputState* ci = scripted_state(g)->clientInput;
    MatchSession* m = scripted_match(g);

    // While the power sweeps: a live cursor, and nothing committed yet.
    scripted_run(g, 6);
    SwingMeterView sweeping = swing_widget_view(&ci->swingWidget);
    ASSERT(sweeping.cursorLive, "the power sweep must be a live cursor");
    ASSERT(!sweeping.powerCommitted, "nothing is committed before the press");

    scripted_tap(g, HUMAN_PAD, KEY_2);
    float declared = m->pendingActionState.swing.power;
    ASSERT_EQ(1, m->pendingActionState.swing.powerActive, "the press must declare a power");

    // THE WAIT. Every frame of it, until the ball is up: the mark is there, at the value committed,
    // and there is no moving cursor pretending to be somewhere.
    int waited = 0;
    for (int i = 0; i < 900; i++) {
        if (m->pRAI.pitch_state == PITCH_STAGE_AIRBORNE) break;
        SwingMeterView v = swing_widget_view(&ci->swingWidget);
        ASSERT(v.powerCommitted, "the committed power must stay on the bar for the whole wait");
        ASSERT(fabsf(v.power - declared) < 0.0001f, "...and it must stay at the value that was committed, not drift");
        ASSERT(!v.cursorLive, "there is no moving cursor while the batter waits for the ball");
        waited++;
        scripted_tick(g);
    }
    ASSERT(waited > 2, "the wait must actually have lasted some frames, or this proves nothing");

    // And once the ball is up, BOTH marks: the committed one still, and the descent moving.
    scripted_tick(g);
    SwingMeterView falling = swing_widget_view(&ci->swingWidget);
    ASSERT(falling.powerCommitted, "the committed mark must survive into the second beat");
    ASSERT(falling.cursorLive, "the elevation marker must be live once the ball is up");
    ASSERT(fabsf(falling.power - declared) < 0.0001f, "and still at the committed value");

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);
    ASSERT(!failed, reason);
    return TEST_PASSED;
}

/**
 * A withdrawal reaches the BODY, not just the scoreboard.
 *
 * The "väärä!" call is a message precisely so that declining is an act rather than an absence — and
 * an act the batter cannot see is not much of an act. The withdrawal already stopped the swing in
 * every sense the rules care about (no miss recorded, the pitch judged on where it landed), while the
 * bat went on swinging on screen.
 *
 * The claim is therefore about the animation the batter is playing, asserted at the moment that is
 * hardest: after his body has already committed to a motion, which is where a withdrawal has to land
 * on all but the very highest tosses.
 */
int test_scripted_swing_withdrawal_reshapes_the_body_mid_motion(void)
{
    ScriptedGame* g = scripted_create(0, 1, HUMAN_PAD, CONTROL_AI, 0x5A1F013u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(reach_the_batting_phase(g, 900), "the human batting side never reached the batting phase");
    ASSERT(tick_until_windup(g, 900), "the AI pitcher never began a windup");

    MatchSession* m = scripted_match(g);

    scripted_run(g, 8);
    scripted_tap(g, HUMAN_PAD, KEY_2); // load
    ASSERT_EQ(1, m->pendingActionState.swing.powerActive, "the power must be committed before it is withdrawn");
    ASSERT(tick_until_airborne(g, 900), "the pitch never left the pitcher's hand");

    // Tick until the body has committed to its motion — the state a late withdrawal has to survive.
    int moving = 0;
    for (int i = 0; i < 300 && !moving; i++) {
        scripted_tick(g);
        moving = (m->pendingActionState.batter_moving == 1);
    }
    ASSERT(moving, "the batter's body never started its motion");
    int batter = get_active_batter_index(m);
    ASSERT(batter != -1, "there must be a batter to look at");
    ASSERT(
        m->playerInfo[batter].cPI.model != PLAYER_ANIM_BAT_SWING_3, "the premise: he is swinging, not already spread"
    );

    // ...and think better of it, with the bat already on its way.
    scripted_tap(g, HUMAN_PAD, KEY_1);
    scripted_tick(g);

    ASSERT_EQ(1, m->pendingActionState.batting_stopped, "the withdrawal must stop the swing");
    ASSERT_EQ(
        PLAYER_ANIM_BAT_SWING_3, m->playerInfo[batter].cPI.model,
        "and the BODY must follow it — a withdrawal the batter cannot see is not a withdrawal"
    );

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);
    ASSERT(!failed, reason);
    return TEST_PASSED;
}
