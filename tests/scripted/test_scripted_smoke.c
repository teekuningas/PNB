#include "test_helpers.h"
#include "scripted_harness.h"
#include "globals.h"

/*
 * Scripted-human harness — PLUMBING smoke tests only (Phase 0b).
 *
 * These knight the *harness itself*, not any game mechanic (testing is knighting; we do not knight
 * mechanics about to change). They prove the three things the harness must do:
 *   1. boot a human-controlled team and run the real pipeline headless without corrupting state,
 *   2. translate a held-key script into the production KeyStates down/release edges correctly,
 *   3. carry a scripted key press through the *real* action_invocations into a game effect.
 *
 * Behavioral, per-action scripted tests (base-run, then pitch, then swing) arrive with each action
 * slice as it stabilizes — base-running first, this harness's first real customer.
 */

#define HUMAN_PAD CONTROL_PLAYER_1 /* pad slot 0 */

/* ---- 1. Liveness / no-corruption ------------------------------------------------------------ *
 * A human-controlled team that does nothing, ticked through the real update_game_frame for a budget.
 * The production state validator guards every frame (a violation sets flowControl.pause, which the
 * harness reports as failure). The game legitimately *stalls* (a human who never acts never takes
 * their turn) — that is fine; we assert only that no frame corrupts state. */
int test_scripted_human_runs_headless(void)
{
    ScriptedGame* g = scripted_create(0, 1, HUMAN_PAD, CONTROL_AI, 0x5C217E01u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");

    scripted_release_all(g); // human does nothing
    long ticked = scripted_run(g, 1500);

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT(ticked > 0, "harness ticked zero frames");
    return TEST_PASSED;
}

/* ---- 2. Key-derivation contract ------------------------------------------------------------- *
 * The subtle, load-bearing bit every future scripted test relies on: held → down, and the one-frame
 * release edge. Inspect the production KeyStates after each tick (nothing in the pipeline clears it —
 * input.c does that on a real machine; here the harness owns it). */
int test_scripted_key_edges(void)
{
    ScriptedGame* g = scripted_create(0, 1, HUMAN_PAD, CONTROL_AI, 0x5C217E02u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");

    KeyStates* ks = scripted_state(g)->keyStates;

    // Hold: down set, no release edge.
    scripted_hold(g, HUMAN_PAD, KEY_2);
    scripted_tick(g);
    ASSERT_EQ(1, ks->down[HUMAN_PAD][KEY_2], "held key should be down");
    ASSERT_EQ(0, ks->released[HUMAN_PAD][KEY_2], "held key should not show a release edge");

    // Release: down clear, release edge for exactly one frame.
    scripted_release(g, HUMAN_PAD, KEY_2);
    scripted_tick(g);
    ASSERT_EQ(0, ks->down[HUMAN_PAD][KEY_2], "released key should be up");
    ASSERT_EQ(1, ks->released[HUMAN_PAD][KEY_2], "release edge should fire the frame after release");

    // Next frame: edge is gone.
    scripted_tick(g);
    ASSERT_EQ(0, ks->released[HUMAN_PAD][KEY_2], "release edge must last exactly one frame");

    // An untouched pad/key stays zero throughout.
    ASSERT_EQ(0, ks->down[CONTROL_PLAYER_2][KEY_1], "untouched pad/key must stay zero");

    scripted_destroy(g);
    return TEST_PASSED;
}

/* ---- 3. Input reaches the real action_invocations ------------------------------------------- *
 * Prove a scripted key press flows through the production action_invocations into a durable game
 * effect. Uses the start-of-game batter selection (the first decision a batting human faces): once
 * the engine asks for a batter (waitingForBatterDecision == 1), a change (KEY_1) then select (KEY_2)
 * — each a release edge — must clear the request. This exercises checkBatterSelection +
 * change_batter + select_batter via the real pipeline. */
int test_scripted_input_reaches_pipeline(void)
{
    ScriptedGame* g = scripted_create(0, 1, HUMAN_PAD, CONTROL_AI, 0x5C217E03u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");

    ASSERT(scripted_tick_until_batter_decision(g, 600), "engine never asked the human for a batter");

    scripted_tap(g, HUMAN_PAD, KEY_1); // cycle to a concrete batter (CHOOSE_BATTER_NEXT)
    scripted_tap(g, HUMAN_PAD, KEY_2); // accept it (CHOOSE_BATTER_SELECT)

    int still_waiting = scripted_match(g)->flowControl.waitingForBatterDecision;
    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT_EQ(0, still_waiting, "scripted select should have cleared waitingForBatterDecision");
    return TEST_PASSED;
}
