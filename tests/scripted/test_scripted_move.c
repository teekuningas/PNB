#include "test_helpers.h"
#include "scripted_harness.h"
#include "globals.h"
#include <math.h>

/*
 * Scripted-human MOVE test — the human input→intent path for fielder movement, driven entirely
 * through the real action_invocations via scripted KeyStates.
 *
 * What the human sends is a DESTINATION, never a direction and never an edge: holding an arrow
 * declares the far end of that heading, releasing everything declares the point the fielder is
 * standing on (which is what "stop" means to an engine that only knows destinations), and the engine
 * owns the walk. These tests are the proof that the gesture survived the change of shape — the same
 * keys, the same speed, the same immediate stop — plus one thing the key stream could not do at all:
 * a destination declared while the engine is refusing to move the fielder is HELD, and the fielder
 * sets off by itself when the refusal ends.
 */

#define HUMAN_PAD CONTROL_PLAYER_1 /* the catching team is the human */
#define MOVE_TEST_FRAMES 60

// As above, but also waiting for a batter who is ready to receive — the engine will not begin a
// windup without one, and the pitch is how this file reaches the engine's "not now" state.
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

// Tick until the human controls the pitcher holding the ball at home, with nothing in progress —
// a settled state in which the engine will walk the controlled fielder wherever it is sent.
static int tick_until_steerable(ScriptedGame* g, int budget)
{
    for (int i = 0; i < budget; i++) {
        scripted_tick(g);
        MatchSession* m = scripted_match(g);
        int p = m->pII.catcherOnBaseIndex[0];
        if (p != -1 && m->pII.controlIndex == p && m->pII.hasBallIndex == p &&
            m->pendingActionState.current_catching_action == CATCHING_ACTION_NONE &&
            m->pRAI.pitch_state == PITCH_STAGE_NONE) {
            return 1;
        }
    }
    return 0;
}

// 1. Hold UP for N frames and the fielder has run N frames' worth of RUN_SPEED to the north, with no
//    sideways drift. This is the speed and the heading in one: the destination the widget sends lies
//    along the same vector the four key flags used to produce, so the velocity is identical.
int test_scripted_move_held_key_runs_at_run_speed(void)
{
    ScriptedGame* g = scripted_create(0, 1, CONTROL_AI, HUMAN_PAD, 0x30FFEE01u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_steerable(g, 8000), "never reached a steerable state");

    MatchSession* m = scripted_match(g);
    int fielder = m->pII.controlIndex;
    Vector3D start = m->playerInfo[fielder].tPI.location;

    scripted_hold(g, HUMAN_PAD, KEY_UP);
    scripted_run(g, MOVE_TEST_FRAMES);

    ASSERT_EQ(fielder, m->pII.controlIndex, "the same fielder was steered throughout");
    float dz = m->playerInfo[fielder].tPI.location.z - start.z;
    float dx = m->playerInfo[fielder].tPI.location.x - start.x;

    // North is -z. One frame of slack at each end for the declare/ingest/actualize ordering.
    float expected = MOVE_TEST_FRAMES * RUN_SPEED;
    ASSERT(dz < 0.0f, "holding UP moves the fielder north");
    ASSERT(
        fabsf(-dz - expected) <= 2.0f * RUN_SPEED,
        "and it covers a held key's worth of RUN_SPEED, not a walk and not a stutter"
    );
    ASSERT(fabsf(dx) < 0.01f, "with no sideways drift on a single-axis heading");

    scripted_destroy(g);
    return TEST_PASSED;
}

// 2. Release and the fielder is stopped on the very next frame. There is no "stop" message: the
//    widget declares the point under the fielder's feet, and the engine reads a destination it has
//    already reached as nothing to do.
int test_scripted_move_release_stops_within_a_frame(void)
{
    ScriptedGame* g = scripted_create(0, 1, CONTROL_AI, HUMAN_PAD, 0x30FFEE02u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_steerable(g, 8000), "never reached a steerable state");

    MatchSession* m = scripted_match(g);
    int fielder = m->pII.controlIndex;

    scripted_hold(g, HUMAN_PAD, KEY_LEFT);
    scripted_run(g, 30);
    ASSERT_EQ(1, m->playerInfo[fielder].cPI.moving, "the fielder is under way before the release");

    scripted_release(g, HUMAN_PAD, KEY_LEFT);
    scripted_tick(g);

    ASSERT_EQ(0, m->playerInfo[fielder].cPI.moving, "releasing every direction stops the fielder at once");
    Vector3D settled = m->playerInfo[fielder].tPI.location;
    scripted_run(g, 30);
    float dx = m->playerInfo[fielder].tPI.location.x - settled.x;
    float dz = m->playerInfo[fielder].tPI.location.z - settled.z;
    ASSERT(sqrtf(dx * dx + dz * dz) < 1e-4f, "and it stays stopped — a released key does not coast");

    scripted_destroy(g);
    return TEST_PASSED;
}

// 3. The engine refuses to move a fielder while a pitch is in the air, and the destination declared
//    during the refusal is HELD rather than thrown away: the fielder sets off the moment the pitch is
//    over, with the key never released.
//
//    This is the behaviour the key stream could not have. There, a direction pressed during a pitch
//    was discarded at the actualization site (fielder_move zeroed the flag), so the human had to let
//    go and press again — the same missing-resume that the smooth_out_movement re-trigger hack existed
//    to paper over after a catch.
int test_scripted_move_declared_during_a_pitch_resumes_after_it(void)
{
    ScriptedGame* g = scripted_create(0, 1, CONTROL_AI, HUMAN_PAD, 0x30FFEE03u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_pitch_ready(g, 8000), "never reached a human pitch-ready state");

    MatchSession* m = scripted_match(g);
    ClientInputState* ci = scripted_state(g)->clientInput;

    // Pitch: press to start the power meter, click during the sweep to lock power — which is what
    // begins the engine windup.
    scripted_tap(g, HUMAN_PAD, KEY_2);
    for (int i = 0; i < 8 && ci->pitchWidget.dir == 0; i++) {
        scripted_tick(g);
    }
    ASSERT(ci->pitchWidget.dir != 0, "the power meter did not start sweeping");
    scripted_tap(g, HUMAN_PAD, KEY_2);

    int reached = 0;
    for (int i = 0; i < 400 && !reached; i++) {
        scripted_tick(g);
        reached = (m->pRAI.pitch_state != PITCH_STAGE_NONE);
    }
    ASSERT(reached, "the human pitch never started");

    // Steer while the pitch is live. The fielder must not budge.
    int fielder = m->pII.controlIndex;
    ASSERT(fielder != -1, "somebody is still controlled during the pitch");
    Vector3D held_still = m->playerInfo[fielder].tPI.location;

    scripted_hold(g, HUMAN_PAD, KEY_UP);
    for (int i = 0; i < 20 && m->pRAI.pitch_state != PITCH_STAGE_NONE; i++) {
        scripted_tick(g);
        ASSERT_EQ(fielder, m->pII.controlIndex, "control did not change during the pitch window");
        float dx = m->playerInfo[fielder].tPI.location.x - held_still.x;
        float dz = m->playerInfo[fielder].tPI.location.z - held_still.z;
        ASSERT(sqrtf(dx * dx + dz * dz) < 1e-4f, "a fielder does not move while a pitch is in progress");
    }

    // The destination was stored all along, so the walk begins by itself once the pitch resolves —
    // no second press.
    ASSERT_EQ(
        1, m->catchingState.controlledMoveTargetActive,
        "the destination declared during the pitch was kept, not discarded"
    );

    scripted_destroy(g);
    return TEST_PASSED;
}
