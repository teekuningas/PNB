#include "test_helpers.h"
#include "scripted_harness.h"
#include "globals.h"
#include "actions/throwing_system.h" // THROW_WINDUP_MAX_FRAMES (release-window budget)
#include <math.h>

/*
 * Scripted-human THROW test — the human input→intent path for the throw (PLAN.md §5.8 phased shape, §5.9
 * engine↔client fix), driven entirely through the real action_invocations via scripted KeyStates (no AI, no
 * direct state pokes).
 *
 * It proves the two-frame gesture: holding KEY_2 + a direction declares INITIATED (direction) and STARTS a
 * client-local CHARGE widget; the widget builds while the key is held; releasing KEY_2 SAMPLES the widget and
 * completes the intent as COMMITTED with power as a VALUE. The power comes from the client widget, never the
 * engine clock (§8.7); the ENGINE then releases the ball once its windup for that power completes. The human
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
        int p = m->pII.catcherOnBaseIndex[0];
        if (p != -1 && m->pII.hasBallIndex == p && m->pII.controlIndex == p &&
            m->pendingActionState.current_catching_action == CATCHING_ACTION_NONE &&
            m->pRAI.pitch_state == PITCH_STAGE_NONE) {
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
    int thrower = m->pII.hasBallIndex;
    ASSERT(thrower >= 0, "the human should hold the ball at home");

    // The client-local throw charge widget — the human's power comes from THIS, never the engine clock.
    InputWidget* tw = &scripted_state(g)->clientInput->throwWidget;

    // Begin the gesture: hold KEY_2 + KEY_LEFT (a pickoff throw to first). One tick to register the hold.
    scripted_hold(g, HUMAN_PAD, KEY_2);
    scripted_hold(g, HUMAN_PAD, KEY_LEFT);
    scripted_tick(g);

    // The windup is now running (GATHERING → the engine's THROWING), ball still in hand, and the client
    // charge widget is armed (CHARGE mode) — the input mechanism that will carry the declared power.
    ASSERT_EQ(
        CATCHING_ACTION_THROWING, (int)m->pendingActionState.current_catching_action,
        "holding KEY_2 + a direction must begin the throw windup"
    );
    ASSERT_EQ(thrower, m->pII.hasBallIndex, "the ball is still in hand while gathering");
    ASSERT_EQ((int)WIDGET_CHARGE, (int)tw->mode, "the client charge widget must arm when the gather begins");

    // Keep holding to build power (the charge widget rises while held) — the throw must NOT auto-release.
    scripted_run(g, 20);
    ASSERT_EQ(thrower, m->pII.hasBallIndex, "a held throw does not auto-release before the human lets go");
    ASSERT_EQ(
        CATCHING_ACTION_THROWING, (int)m->pendingActionState.current_catching_action, "still gathering while held"
    );
    ASSERT(tw->counter > 0, "the client charge widget builds power while KEY_2 is held (the input signal)");

    // Release KEY_2 → completes the intent as COMMITTED (power sampled from the widget); the ENGINE then
    // releases the ball once its windup for that power finishes (the gather ran while held, so it fires
    // within a few frames of key-up — but never on the raw release edge).
    scripted_release(g, HUMAN_PAD, KEY_2);
    int released = 0;
    for (int i = 0; i < THROW_WINDUP_MAX_FRAMES && !released; i++) {
        scripted_tick(g);
        if (m->pII.hasBallIndex == -1) released = 1;
    }
    float vx = m->ballInfo.velocity.x, vz = m->ballInfo.velocity.z;
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
