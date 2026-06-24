#include "test_helpers.h"
#include "scripted_harness.h"
#include "globals.h"
#include "rules_pure/player_utils.h"

/*
 * Scripted-human BASE-RUN behavioral test (the harness's first real customer — the base-running slice
 * is now stable, so it is knighted, per "testing is knighting").
 *
 * It proves the human input→intent mapping the slice introduced (action_invocations.c +
 * execute_actions.c::base_run): a SINGLE base-run tap = RUN_FORWARD ("arm / lead"), a DOUBLE tap = a
 * RUN_COMMIT ("run now"). The sharp behavioral difference is on the batter: with the ball live (a pitch
 * in the air, batter_can_advance == 1), a lone tap only ARMS the batter — they keep standing — while a
 * double tap makes them break for first (the steal the user reported as broken before this mapping
 * existed). Driven entirely through the real action_invocations via scripted KeyStates.
 */

#define HUMAN_PAD CONTROL_PLAYER_1 /* batting team (team 0) is the human */

static void press_release(ScriptedGame* g, int pad, int key)
{
    scripted_hold(g, pad, key);
    scripted_tick(g); // key down (the run intent is set on the release edge, not the press)
    scripted_release(g, pad, key);
    scripted_tick(g); // release edge -> action_invocations sets the intent, execute_actions consumes it
}

static int tick_until_batter_decision(ScriptedGame* g, int budget)
{
    for (int i = 0; i < budget; i++) {
        scripted_tick(g);
        if (scripted_match(g)->flowControl.waitingForBatterDecision == 1) return 1;
    }
    return 0;
}

// Tick until the AI fielding side has put a pitch in the air (the ball is live for the batter).
static int tick_until_ball_live(ScriptedGame* g, int budget)
{
    for (int i = 0; i < budget; i++) {
        scripted_tick(g);
        if (scripted_match(g)->pRAI.batter_can_advance == 1) return 1;
    }
    return 0;
}

// Boot a human-batting vs AI-fielding game and get a chosen batter standing in with the ball live.
// Returns the active batter index, or -1 on setup failure.
static int setup_live_batter(ScriptedGame* g)
{
    if (!tick_until_batter_decision(g, 1500)) return -1;
    press_release(g, HUMAN_PAD, KEY_1); // cycle to a concrete batter
    press_release(g, HUMAN_PAD, KEY_2); // accept it
    if (!tick_until_ball_live(g, 4000)) return -1;
    return get_active_batter_index(scripted_match(g));
}

// 1. A single tap only ARMS the batter — with the ball live they do NOT start running.
int test_scripted_single_tap_does_not_run_batter(void)
{
    ScriptedGame* g = scripted_create(0, 1, HUMAN_PAD, CONTROL_AI, 0xB00B5E01u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");

    int bi = setup_live_batter(g);
    ASSERT(bi >= 0, "could not get a live batter (selection / AI pitch never happened)");
    ASSERT_EQ(PLAYER_STATE_AT_BAT, (int)scripted_match(g)->playerInfo[bi].bTPI.state, "batter should be AT_BAT");

    press_release(g, HUMAN_PAD, KEY_DOWN); // single tap on the home/first key
    scripted_run(g, 5);

    // Armed, not committed: the batter is still at bat and not going forward.
    int going = scripted_match(g)->playerRuntime[bi].goingForward;
    int state = (int)scripted_match(g)->playerInfo[bi].bTPI.state;
    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT_EQ(0, going, "a single tap must only ARM the batter, never start them running");
    ASSERT_EQ(PLAYER_STATE_AT_BAT, state, "single-tapped batter should remain AT_BAT");
    return TEST_PASSED;
}

// 2. A double tap COMMITS the batter — with the ball live they break for first (the steal).
int test_scripted_double_tap_runs_batter(void)
{
    ScriptedGame* g = scripted_create(0, 1, HUMAN_PAD, CONTROL_AI, 0xB00B5E02u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");

    int bi = setup_live_batter(g);
    ASSERT(bi >= 0, "could not get a live batter (selection / AI pitch never happened)");

    // Double tap within the press window: first release arms, second release commits.
    press_release(g, HUMAN_PAD, KEY_DOWN);
    press_release(g, HUMAN_PAD, KEY_DOWN);
    scripted_run(g, 5);

    // Committed: run_to_next_base(BASE_HOME) set the batter going forward and stopped the at-bat.
    int going = scripted_match(g)->playerRuntime[bi].goingForward;
    int batting_going_on = scripted_match(g)->pRAI.batting_going_on;
    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT_EQ(1, going, "a double tap must COMMIT the batter to run toward first (the steal)");
    ASSERT_EQ(0, batting_going_on, "committing the batter to run must end the at-bat");
    return TEST_PASSED;
}
