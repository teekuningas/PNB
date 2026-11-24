#include "test_helpers.h"
#include "actions/batting_system.h"
#include "scripted_harness.h"
#include <stdio.h>
#include "key_states.h"
#include "world.h"
#include "rules_pure/player_utils.h"

/*
 * Scripted-human BASE-RUN behavioral test (the harness's first real customer — the base-running slice
 * is now stable, so it is knighted, per "testing is knighting").
 *
 * It proves the human input→intent mapping the slice introduced (action_invocations.c +
 * actualize.c's base_run): a SINGLE base-run tap = RUN_FORWARD ("arm / lead"), a DOUBLE tap = a
 * RUN_COMMIT ("run now"). The sharp behavioral difference is on the batter: with the ball live (a pitch
 * in the air, half_inning.batter_may_leave_home == 1), a lone tap only ARMS the batter — they keep standing — while a
 * double tap makes them break for first (the steal the user reported as broken before this mapping
 * existed). Driven entirely through the real action_invocations via scripted KeyStates.
 */

#define HUMAN_PAD CONTROL_PLAYER_1 /* batting team (team 0) is the human */

// Tick until the AI fielding side has put a pitch in the air (the ball is live for the batter).
static int tick_until_ball_live(ScriptedGame* g, int budget)
{
    for (int i = 0; i < budget; i++) {
        scripted_tick(g);
        if (scripted_state(g)->rules->half_inning.batter_may_leave_home == 1) return 1;
    }
    return 0;
}

// Boot a human-batting vs AI-fielding game and get a chosen batter standing in with the ball live.
// Returns the active batter index, or -1 on setup failure.
static int setup_live_batter(ScriptedGame* g)
{
    if (!scripted_tick_until_batter_decision(g, 1500)) return -1;
    scripted_tap(g, HUMAN_PAD, KEY_1); // cycle to a concrete batter
    scripted_tap(g, HUMAN_PAD, KEY_2); // accept it
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
    ASSERT_EQ(
        PLAYER_STATE_AT_BAT, (int)scripted_match(g)->players[bi].batting.presence.state, "batter should be AT_BAT"
    );

    scripted_tap(g, HUMAN_PAD, KEY_DOWN); // single tap on the home/first key
    scripted_run(g, 5);

    // Armed, not committed: the batter is still at bat and not going forward.
    int going = scripted_match(g)->players[bi].path.going_forward;
    int state = (int)scripted_match(g)->players[bi].batting.presence.state;
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
    scripted_tap(g, HUMAN_PAD, KEY_DOWN);
    scripted_tap(g, HUMAN_PAD, KEY_DOWN);
    scripted_run(g, 5);

    // Committed: run_to_next_base(BASE_HOME) set the batter going forward and stopped the at-bat.
    int going = scripted_match(g)->players[bi].path.going_forward;
    int cycle_open = batting_cycle_is_open(&scripted_match(g)->at_bat);
    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT_EQ(1, going, "a double tap must COMMIT the batter to run toward first (the steal)");
    ASSERT_EQ(0, cycle_open, "committing the batter to run must end the at-bat");
    return TEST_PASSED;
}

// 3. §32: a double tap BEFORE the first pitch is refused by the INGEST gate (while single tap still arms).
int test_scripted_double_tap_before_first_pitch_does_not_run_batter(void)
{
    // Both teams human so the pitcher never pitches while we test the pre-pitch window.
    ScriptedGame* g = scripted_create(0, 1, CONTROL_PLAYER_1, CONTROL_PLAYER_2, 0xB00B5E03u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");

    ASSERT(scripted_tick_until_batter_decision(g, 1500), "batter decision never opened");
    scripted_tap(g, CONTROL_PLAYER_1, KEY_1);
    scripted_tap(g, CONTROL_PLAYER_1, KEY_2);

    int ready = 0;
    for (int i = 0; i < 600; i++) {
        scripted_tick(g);
        if (batter_stands_ready(&scripted_match(g)->at_bat)) {
            ready = 1;
            break;
        }
    }
    ASSERT(ready, "batter never arrived at the plate");
    int bi = get_active_batter_index(scripted_match(g));
    ASSERT(bi >= 0, "active batter should exist");
    ASSERT_EQ(0, scripted_state(g)->rules->half_inning.batter_may_leave_home, "§32 right must be 0 before first pitch");

    // Double tap KEY_DOWN: first tap arms (RUN_FORWARD, allowed), second tap emits RUN_COMMIT (refused by §32).
    scripted_tap(g, CONTROL_PLAYER_1, KEY_DOWN);
    scripted_tap(g, CONTROL_PLAYER_1, KEY_DOWN);
    scripted_run(g, 10);

    int armed = scripted_match(g)->runner_commitments.armed[BASE_HOME];
    int going = scripted_match(g)->players[bi].path.going_forward;
    int state = (int)scripted_match(g)->players[bi].batting.presence.state;
    int cycle_open = batting_cycle_is_open(&scripted_match(g)->at_bat);
    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT_EQ(1, armed, "first tap of double-tap still arms the batter before first pitch");
    ASSERT_EQ(0, going, "§32 must refuse RUN_COMMIT before first pitch");
    ASSERT_EQ(PLAYER_STATE_AT_BAT, state, "batter must remain AT_BAT before first pitch");
    ASSERT_EQ(1, cycle_open, "batting cycle must stay open when RUN_COMMIT is refused");
    return TEST_PASSED;
}
