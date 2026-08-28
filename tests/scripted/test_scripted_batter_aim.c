#include "test_helpers.h"
#include "scripted_harness.h"
#include "all_scripted.h"
#include "actions/batting_system.h"
#include "rules_pure/player_utils.h"
#include <math.h>

#define HUMAN_PAD CONTROL_PLAYER_1 /* the batting team (team 0) is the human */

/*
 * The human batting path, headless: a cursor over the legal batters, and held keys that become an
 * aim. This tier is the only one that runs a real KeyStates through the real action_invocations, so
 * it is the only place the client half of §27 and INTENT_SWING_ANGLE can be proved without a screen.
 *
 * The contract tier already knights what the ENGINE does with an angle once it arrives. What is
 * missing there is whether a human pressing keys produces the right angle at all — the half that
 * used to be key edges the engine had to interpret, and is now a value the client computes.
 */

// Get a human-controlled batting side through the batter decision and into the batting phase, where
// aim is live. Returns 1 on success.
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

/**
 * A held aim key walks the batter around the arc at the arc's own rate, and releasing it stops him
 * exactly where he is.
 *
 * The rate is the load-bearing half. The client cursor and the engine's walk move at the same
 * constant, so the body tracks the keys as directly as it did when the key edges themselves were the
 * message — a client that drifted from the engine's rate would feel like input lag and nothing in
 * the suite but this would notice.
 *
 * The stop is the other half, and it is what the value shape buys: an angle is complete, so a
 * release simply stops changing it. The old path had to deliver a STOP edge to the engine, and a
 * dropped one left the batter walking.
 */
int test_scripted_aim_key_walks_the_batter_and_release_stops_him(void)
{
    ScriptedGame* g = scripted_create(0, 1, HUMAN_PAD, CONTROL_AI, 0x5A1F00Du);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(reach_the_batting_phase(g, 900), "the human batting side never reached the batting phase");

    MatchSession* m = scripted_match(g);
    const float start = m->pendingActionState.batter_angle;

    const int HELD = 8;
    scripted_hold(g, HUMAN_PAD, KEY_PLUS);
    scripted_run(g, HELD);
    const float walked = m->pendingActionState.batter_angle;

    scripted_release(g, HUMAN_PAD, KEY_PLUS);
    scripted_run(g, 20);
    const float rested = m->pendingActionState.batter_angle;

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT(walked > start, "a held aim key must move the batter around the arc");
    // The body may lag the cursor by at most one step, since the cursor moves first and the walk
    // follows within the same frame's actualization.
    const float expected = start + (float)HELD * BATTER_ANGLE_SPEED_CONSTANT;
    ASSERT(
        fabsf(walked - expected) <= BATTER_ANGLE_SPEED_CONSTANT + 0.0001f,
        "the batter must track the held key at the arc's own rate, within a step"
    );
    ASSERT(fabsf(rested - walked) < 0.0001f, "releasing the key must leave the batter exactly where he stood");
    return TEST_PASSED;
}

/**
 * The batter-selection cursor offers only players the rules allow, and accepting one seats him.
 *
 * The client half of the rule: a human is never shown an option the gate would refuse. Pressing the
 * change key here cannot walk off the end of the list or land on a spent joker, because the list is
 * rebuilt from the world each frame by the same function the gate judges the answer with — so this
 * also pins that the two callers really are one rule.
 */
int test_scripted_batter_cursor_offers_only_legal_players(void)
{
    ScriptedGame* g = scripted_create(0, 1, HUMAN_PAD, CONTROL_AI, 0x0FFE1234u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(scripted_tick_until_batter_decision(g, 900), "engine never asked the human for a batter");

    StateInfo* st = scripted_state(g);
    MatchSession* m = st->match;

    // Walk the cursor right round the list and back, checking every stop is a player the rules allow.
    for (int step = 0; step < BATTER_CANDIDATE_MAX + 2; step++) {
        int candidates[BATTER_CANDIDATE_MAX];
        int count = list_batter_candidates(m, &st->rules->scoreboard, &st->rules->halfInningState, candidates);
        ASSERT(count > 0, "a decision is open, so somebody must be offerable");
        int cursor = st->clientInput->batterWidget.highlight;
        ASSERT(cursor >= 0 && cursor < count, "the cursor must always sit inside the candidate list");

        int shown = candidates[cursor];
        ASSERT_NE(
            (int)JOKER_USED, (int)m->playerInfo[shown].bTPI.joker, "§7: a spent joker must never be on the cursor"
        );
        scripted_tap(g, HUMAN_PAD, KEY_1);
    }

    scripted_tap(g, HUMAN_PAD, KEY_2);
    for (int i = 0; i < 300 && m->flowControl.waitingForBatterDecision == 1; i++) {
        scripted_tick(g);
    }

    int seated = get_active_batter_index(m);
    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT_NE(-1, seated, "accepting a candidate must seat a batter");
    return TEST_PASSED;
}
