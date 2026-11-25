#include "test_helpers.h"
#include "scripted_harness.h"
#include <stdio.h>
#include "key_states.h"
#include "world.h"
#include "client_input.h"
#include "action_invocations.h" // catching_widget_view — what the thrower sees on the bar
#include "actions/throwing_system.h" // THROW_WINDUP_MAX_FRAMES (release-window budget)
#include <math.h>

/*
 * Scripted-human THROW test — the human input→intent path for the throw (the phased shape and the
 * engine↔client fix), driven entirely through the real action_invocations via scripted KeyStates (no AI, no
 * direct state pokes).
 *
 * It proves the two-frame gesture: holding KEY_2 + a direction declares INITIATED (direction) and STARTS a
 * client-local CHARGE widget; the widget builds while the key is held; releasing KEY_2 SAMPLES the widget and
 * completes the intent as COMMITTED with power as a VALUE. The power comes from the client widget, never the
 * engine clock; the ENGINE then releases the ball once its windup for that power completes. The human
 * here is the catching team, holding the ball at home; it throws a pickoff to first (home→first is far enough
 * to throw) instead of pitching.
 */

#define HUMAN_PAD CONTROL_PLAYER_1 /* the catching team is the human (holds the ball at home to start) */

// Tick until the human controls the pitcher holding the ball at home with no catching action in progress —
// the same idle ready state the pitch test reaches, but here the human will THROW to a base.
static int tick_until_ball_holder_ready(ScriptedGame* g, int budget)
{
    for (int i = 0; i < budget; i++) {
        scripted_tick(g);
        MatchSession* m = scripted_match(g);
        int p = m->fielding_posts.baseman[0];
        if (p != -1 && m->ball.holder == p && m->catching.controlled_fielder == p &&
            m->catching_action.kind == CATCHING_ACTION_NONE && !m->ball.pitch_in_flight) {
            return 1;
        }
    }
    return 0;
}

// Hold KEY_2 + a direction to gather a throw, hold to build power, release KEY_2 to fire → the ball is
// thrown to the chosen base.
int test_scripted_throw_hold_release_to_base(void)
{
    ScriptedGame* g = scripted_create(0, 1, CONTROL_AI, HUMAN_PAD, 0x2C0FFEE1u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_ball_holder_ready(g, 8000), "never reached a human ball-holder ready state");

    MatchSession* m = scripted_match(g);
    int thrower = m->ball.holder;
    ASSERT(thrower >= 0, "the human should hold the ball at home");

    // The client-local throw charge widget — the human's power comes from THIS, never the engine clock.
    InputWidget* tw = &scripted_state(g)->client_input->throw_widget.meter;

    // Begin the gesture: hold KEY_2 + KEY_LEFT (a pickoff throw to first). One tick to register the hold.
    scripted_hold(g, HUMAN_PAD, KEY_2);
    scripted_hold(g, HUMAN_PAD, KEY_LEFT);
    scripted_tick(g);

    // The windup is now running (GATHERING → the engine's THROWING), ball still in hand, and the client
    // charge widget is armed (CHARGE mode) — the input mechanism that will carry the declared power.
    ASSERT_EQ(
        CATCHING_ACTION_THROWING, (int)m->catching_action.kind,
        "holding KEY_2 + a direction must begin the throw windup"
    );
    ASSERT_EQ(thrower, m->ball.holder, "the ball is still in hand while gathering");
    ASSERT_EQ((int)WIDGET_CHARGE, (int)tw->mode, "the client charge widget must arm when the gather begins");

    // Keep holding to build power (the charge widget rises while held) — the throw must NOT auto-release.
    scripted_run(g, 20);
    ASSERT_EQ(thrower, m->ball.holder, "a held throw does not auto-release before the human lets go");
    ASSERT_EQ(CATCHING_ACTION_THROWING, (int)m->catching_action.kind, "still gathering while held");
    ASSERT(tw->counter > 0, "the client charge widget builds power while KEY_2 is held (the input signal)");

    // Release KEY_2 → completes the intent as COMMITTED (power sampled from the widget); the ENGINE then
    // releases the ball once its windup for that power finishes (the gather ran while held, so it fires
    // within a few frames of key-up — but never on the raw release edge).
    scripted_release(g, HUMAN_PAD, KEY_2);
    int released = 0;
    for (int i = 0; i < THROW_WINDUP_MAX_FRAMES && !released; i++) {
        scripted_tick(g);
        if (m->ball.holder == -1) released = 1;
    }
    float vx = m->ball.velocity.x, vz = m->ball.velocity.z;
    float horiz = sqrtf(vx * vx + vz * vz);
    int widget_retired = (tw->mode == WIDGET_IDLE); // the sampled widget is retired at the release edge

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT(released, "releasing KEY_2 must fire the throw (the ball leaves the hand)");
    ASSERT(horiz > 0.01f, "the thrown ball must have a real horizontal velocity");
    ASSERT(widget_retired, "the client charge widget is retired once its value is sampled into the declaration");
    return TEST_PASSED;
}

// Releasing KEY_2 commits the throw power and keeps that level visible on the catching bar while the
// throw finishes winding up and flying cleanly, then returns the bar to rest once caught or grounded.
int test_scripted_throw_committed_power_stays_on_bar(void)
{
    ScriptedGame* g = scripted_create(0, 1, CONTROL_AI, HUMAN_PAD, 0x2C0FFEE2u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_ball_holder_ready(g, 8000), "never reached a human ball-holder ready state");

    MatchSession* m = scripted_match(g);
    const ClientInputState* ci = scripted_state(g)->client_input;

    // Hold KEY_2 + KEY_LEFT to charge a throw to 1st base.
    scripted_hold(g, HUMAN_PAD, KEY_2);
    scripted_hold(g, HUMAN_PAD, KEY_LEFT);
    scripted_run(g, 21); // 1 tick to arm + 20 ticks charging

    float level_before_release = catching_widget_view(ci);
    ASSERT(level_before_release > 0.3f, "bar must show the rising throw charge while held");

    // Release KEY_2: the live charge meter retires, but the committed power stays visible on the bar.
    scripted_release(g, HUMAN_PAD, KEY_2);
    scripted_release(g, HUMAN_PAD, KEY_LEFT);
    scripted_tick(g);

    float committed_level = catching_widget_view(ci);
    ASSERT_EQ(WIDGET_IDLE, (int)ci->throw_widget.meter.mode, "charge meter retires on KEY_2 release");
    ASSERT_EQ(1, ci->throw_widget.power_committed, "committed throw power flag must arm on release");
    ASSERT(committed_level >= level_before_release, "committed bar level must not drop back to 0 on release");

    // Watch every frame through windup and clean airborne flight: the bar must hold the committed mark,
    // and then return to 0 once the throw is caught or touches the ground.
    int airborne_frames_held = 0;
    int bar_dropped_early = 0;
    int returned_to_rest = 0;
    for (int f = 0; f < 300; f++) {
        scripted_tick(g);
        const int in_flight = (m->catching_action.kind == CATCHING_ACTION_THROWING) ||
                              (m->ball.thrown_to != BASE_NONE && !m->ball.flight_has_hit_ground);
        const float view = catching_widget_view(ci);
        if (in_flight) {
            if (m->ball.holder == -1) airborne_frames_held++;
            if (fabsf(view - committed_level) > 1e-6f) bar_dropped_early = 1;
        } else if (!ci->throw_widget.power_committed && fabsf(view) < 1e-6f) {
            returned_to_rest = 1;
            break;
        }
    }

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT(airborne_frames_held > 0, "the thrown ball must fly in the air while the committed mark holds");
    ASSERT(!bar_dropped_early, "committed throw power must stay steady on the bar until the throw finishes");
    ASSERT(returned_to_rest, "once the throw is caught or grounded, the bar must return to rest");
    return TEST_PASSED;
}
