#include "test_helpers.h"
#include "scripted_harness.h"
#include "globals.h"
#include <math.h>

/*
 * Scripted-human THROW test — the human input→intent path for the throw rework (PLAN.md §5.8), driven
 * entirely through the real action_invocations via scripted KeyStates (no AI, no direct state pokes).
 *
 * It proves the hold-release gesture: holding KEY_2 + a direction declares a phased ThrowDeclaration
 * (GATHERING) — the engine begins the windup while the key is held — and releasing KEY_2 declares RELEASED,
 * firing the throw with power read off the shared windup clock. There is zero client-local throw state; the
 * clock is the "meter". The human here is the catching team, holding the ball at home; it throws a pickoff
 * to first (home→first is far enough to throw) instead of pitching.
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

    // Begin the gesture: hold KEY_2 + KEY_LEFT (a pickoff throw to first). One tick to register the hold.
    scripted_hold(g, HUMAN_PAD, KEY_2);
    scripted_hold(g, HUMAN_PAD, KEY_LEFT);
    scripted_tick(g);

    // The windup is now running (GATHERING → the engine's THROWING), ball still in hand.
    ASSERT_EQ(
        CATCHING_ACTION_THROWING, (int)m->pendingActionState.current_catching_action,
        "holding KEY_2 + a direction must begin the throw windup"
    );
    ASSERT_EQ(1, m->pendingActionState.throw_going_on, "throw_going_on is set during the gather");
    ASSERT_EQ(thrower, m->pII.hasBallIndex, "the ball is still in hand while gathering");

    // Keep holding to build power (the gather animates while held) — the throw must NOT auto-release.
    scripted_run(g, 20);
    ASSERT_EQ(thrower, m->pII.hasBallIndex, "a held throw does not auto-release before the human lets go");
    ASSERT_EQ(
        CATCHING_ACTION_THROWING, (int)m->pendingActionState.current_catching_action, "still gathering while held"
    );

    // Release KEY_2 → the RELEASED edge fires the throw; power is read off the windup clock.
    scripted_release(g, HUMAN_PAD, KEY_2);
    int released = 0;
    for (int i = 0; i < 10 && !released; i++) {
        scripted_tick(g);
        if (m->pII.hasBallIndex == -1) released = 1;
    }
    float vx = m->ballInfo.velocity.x, vz = m->ballInfo.velocity.z;
    float horiz = sqrtf(vx * vx + vz * vz);

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT(released, "releasing KEY_2 must fire the throw (the ball leaves the hand)");
    ASSERT(horiz > 0.01f, "the thrown ball must have a real horizontal velocity");
    return TEST_PASSED;
}
