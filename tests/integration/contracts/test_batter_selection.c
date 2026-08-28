#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "execute_actions.h"
#include "rules_pure/player_utils.h"
#include "rules_pure/base_control.h"
#include "globals.h"

/**
 * CONTRACT: §27/§12/§7 decide who may take the bat, and the INGEST gate is where they are asked.
 *
 * The batter-selection slice made WHO a value a producer names rather than a cursor the engine kept.
 * That moves the rule from "the engine only ever offers legal players" to "the engine only ever
 * SEATS legal players", which is the stronger claim and the one that survives a client being wrong —
 * a lagging widget, a scripted test, a peer on a wire. These tests build each illegal request and
 * require it to be refused, and build the legal one and require it to be seated.
 *
 * Why here rather than in the state validator: seating a regular ADVANCES the batting order, which
 * clears `regularOrderSpent` in the same frame. An end-of-frame assertion would be looking at a state
 * that erases itself at the moment of the violation — which is exactly the trap bug #9's invariant
 * fell into. A refusal at the gate has no such window: it either happened or it did not.
 */

// Put the world in the one window §12 is about: nobody at the plate, a decision open, the batting
// order come round to the designated last batter, and jokers still available. The previous batter
// still OWNS home, which is the window bug #9 lived in — the prompt fires when the plate is empty,
// but home safety outlives the plate by some frames.
static void setup_order_spent_with_jokers(ScenarioContext* ctx, int* inTurnRegular)
{
    MatchSession* m = ctx->state->match;
    GameRulesState* r = ctx->state->rules;
    Scoreboard* sb = &r->scoreboard;
    HalfInningState* his = &r->halfInningState;

    initialize_referee_from_physical_state(ctx);

    // Empty the plate but leave home owned: a batter who has set off and not arrived anywhere.
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        m->playerInfo[i].bTPI.state = PLAYER_STATE_IDLE;
        m->playerInfo[i].bTPI.baseId = BASE_NONE;
        r->referee.battingPlayers[i].currentSafetyBase = BASE_NONE;
    }
    const int battingTeamIndex = get_batting_team_index(sb);
    const int runner = sb->teams[battingTeamIndex].batterOrder[0];
    m->playerInfo[runner].bTPI.baseId = BASE_HOME;
    m->playerInfo[runner].bTPI.state = PLAYER_STATE_RUNNING;
    r->referee.battingPlayers[runner].currentSafetyBase = BASE_HOME;
    r->referee.battingPlayers[runner].baseAtPitchStart = BASE_HOME;
    r->referee.battingPlayers[runner].status = PLAYER_STATUS_ACTIVE;

    *inTurnRegular = sb->teams[battingTeamIndex].batterOrder[sb->teams[battingTeamIndex].batterOrderIndex];

    his->lastBatter.regularOrderSpent = 1;
    his->jokersLeft = JOKER_COUNT;
    for (int i = 0; i < JOKER_COUNT; i++) {
        m->playerInfo[m->pII.jokerIndices[i]].bTPI.joker = JOKER_AVAILABLE;
    }
    m->flowControl.waitingForBatterDecision = 1;
}

static void declare_seat(ScenarioContext* ctx, int index)
{
    intent_push(
        &ctx->state->channels.batting,
        (IntentMessage){.kind = INTENT_SELECT_BATTER, .as.select_batter = {.index = index}}
    );
}

static void tick_ingest(ScenarioContext* ctx)
{
    execute_actions(
        ctx->state->match, ctx->state->rules, ctx->state->fieldPositions, &ctx->state->channels,
        &ctx->state->playSoundEffect
    );
}

/**
 * 1. §12 — once the regular order is spent, naming a regular is refused, and the candidate list a
 *    client would show contains only jokers.
 *
 * This is bug #7/#9's shape stated as a refusal instead of as an offer. The rulebook does not treat
 * it as a nuance: its own worked example under §12 voids the actions and whistles the side change,
 * because coming to bat is what spends the team's right to a joker.
 */
int test_seat_refuses_regular_when_order_spent(void)
{
    ScenarioContext* ctx = create_scenario();
    int inTurnRegular;
    setup_order_spent_with_jokers(ctx, &inTurnRegular);
    MatchSession* m = ctx->state->match;

    // The client's view of the same rule: only jokers on offer.
    int candidates[BATTER_CANDIDATE_MAX];
    int count =
        list_batter_candidates(m, &ctx->state->rules->scoreboard, &ctx->state->rules->halfInningState, candidates);
    ASSERT(count > 0, "jokers remain, so somebody may still bat");
    for (int i = 0; i < count; i++) {
        ASSERT_NE(
            (int)JOKER_REGULAR, (int)m->playerInfo[candidates[i]].bTPI.joker,
            "with the order spent, no regular may appear in the candidate list"
        );
    }

    // The engine's view: the request is refused however it was arrived at.
    declare_seat(ctx, inTurnRegular);
    tick_ingest(ctx);

    ASSERT_EQ(-1, get_active_batter_index(m), "a regular must not be seated once §12 has spent the order");
    ASSERT_EQ(1, m->flowControl.waitingForBatterDecision, "and the decision must still be open, not silently consumed");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

/**
 * 2. §7 — a joker already used this half-inning is refused.
 *
 * "Joukkue voi käyttää jokaisessa sisävuorossa kolmea eri jokeripelaajaa, kerran kutakin." Seating a
 * spent joker would also desynchronise the two places that hold this resource: the per-player status
 * has nothing left to change, but the referee decrements jokersLeft on every entry that is not a
 * regular. The state validator holds that pair; this holds the door it comes through.
 */
int test_seat_refuses_a_spent_joker(void)
{
    ScenarioContext* ctx = create_scenario();
    int inTurnRegular;
    setup_order_spent_with_jokers(ctx, &inTurnRegular);
    MatchSession* m = ctx->state->match;

    const int spentJoker = m->pII.jokerIndices[0];
    m->playerInfo[spentJoker].bTPI.joker = JOKER_USED;
    ctx->state->rules->halfInningState.jokersLeft = JOKER_COUNT - 1;

    declare_seat(ctx, spentJoker);
    tick_ingest(ctx);

    ASSERT_EQ(-1, get_active_batter_index(m), "a joker may bat once per half-inning, and this one has");
    ASSERT_EQ(
        (int)JOKER_USED, (int)m->playerInfo[spentJoker].bTPI.joker, "and his status must be left exactly as it was"
    );

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

/**
 * 3. §27 — a regular whose turn it is not is refused, even with the order still live.
 *
 * "Pöytäkirjaan merkittyä lyöntijärjestystä on noudatettava koko ottelun ajan." The order is a cycle
 * followed for the whole match, so "not in turn" is never "has had their turn" — it is somebody
 * else's. Nothing in the old engine could express this request at all: the cursor only ever visited
 * the in-turn regular and the jokers, so the rule was held by the shape of a walk rather than stated.
 * Naming a player makes it expressible, which is why it now has to be refused.
 */
int test_seat_refuses_a_regular_out_of_turn(void)
{
    ScenarioContext* ctx = create_scenario();
    int inTurnRegular;
    setup_order_spent_with_jokers(ctx, &inTurnRegular);
    MatchSession* m = ctx->state->match;
    Scoreboard* sb = &ctx->state->rules->scoreboard;

    ctx->state->rules->halfInningState.lastBatter.regularOrderSpent = 0; // the order is live

    const int battingTeamIndex = get_batting_team_index(sb);
    const int nextIndex = (sb->teams[battingTeamIndex].batterOrderIndex + 1) % PLAYERS_IN_TEAM;
    const int outOfTurn = sb->teams[battingTeamIndex].batterOrder[nextIndex];
    ASSERT_NE(inTurnRegular, outOfTurn, "setup: the two players must actually differ");

    declare_seat(ctx, outOfTurn);
    tick_ingest(ctx);

    ASSERT_EQ(-1, get_active_batter_index(m), "§27: the batting order is not a pool to pick from");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

/**
 * 4. The legal request IS seated — and seating waits for the seat rather than refusing.
 *
 * Both halves matter. The first is the positive control every refusal above needs: without it, a gate
 * that denied everything would pass three tests. The second is the split this slice chose — the gate
 * asks the rules question, and the physical claim (the previous batter still owns home) stays with
 * the seating, where it holds whether or not anyone declared anything. The producer restates its
 * choice, so it is seated on the first frame the seat is free without ever knowing why it waited.
 */
int test_seat_waits_for_home_then_seats_the_named_joker(void)
{
    ScenarioContext* ctx = create_scenario();
    int inTurnRegular;
    setup_order_spent_with_jokers(ctx, &inTurnRegular);
    MatchSession* m = ctx->state->match;
    GameRulesState* r = ctx->state->rules;

    const int joker = m->pII.jokerIndices[0];

    // Home is still owned by the departing batter: a legal request, an unavailable seat.
    ASSERT_NE(-1, get_base_controller(m, &r->referee, BASE_HOME), "setup: home must still be owned");
    declare_seat(ctx, joker);
    tick_ingest(ctx);
    ASSERT_EQ(-1, get_active_batter_index(m), "nobody may be seated while the previous batter still holds home");
    ASSERT_EQ(1, m->flowControl.waitingForBatterDecision, "and the decision stays open");

    // The previous batter is finally established elsewhere; the seat is free.
    const int battingTeamIndex = get_batting_team_index(&r->scoreboard);
    const int runner = r->scoreboard.teams[battingTeamIndex].batterOrder[0];
    r->referee.battingPlayers[runner].currentSafetyBase = BASE_FIRST;
    m->playerInfo[runner].bTPI.baseId = BASE_FIRST;

    declare_seat(ctx, joker); // the producer is still saying the same thing
    tick_ingest(ctx);

    ASSERT_EQ(joker, get_active_batter_index(m), "the named joker takes the bat on the first frame the seat is free");
    ASSERT_EQ(0, m->flowControl.waitingForBatterDecision, "and the decision is closed");
    ASSERT_EQ((int)JOKER_USED, (int)m->playerInfo[joker].bTPI.joker, "§7: the joker is spent by taking the bat");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

/**
 * 5. Repeat-safety: the same request delivered three times in one frame seats exactly one batter.
 *
 * ARCHITECTURE's rule for every message — "a complete current value with hold-until-replaced
 * semantics… never anything that means something different when applied twice" — is what makes
 * rollback (which re-delivers the inputs it has) correct by construction, and what lets a producer
 * restate its choice every frame without consequence. A seating is the sharpest possible test of it:
 * it fires a game event, spends a joker and advances the batting order, so a second application
 * would be loudly wrong.
 */
int test_repeated_seat_requests_seat_once(void)
{
    ScenarioContext* ctx = create_scenario();
    int inTurnRegular;
    setup_order_spent_with_jokers(ctx, &inTurnRegular);
    MatchSession* m = ctx->state->match;
    GameRulesState* r = ctx->state->rules;

    // Free the seat first: this test is about duplicate messages, not about waiting.
    const int battingTeamIndex = get_batting_team_index(&r->scoreboard);
    const int runner = r->scoreboard.teams[battingTeamIndex].batterOrder[0];
    r->referee.battingPlayers[runner].currentSafetyBase = BASE_FIRST;
    m->playerInfo[runner].bTPI.baseId = BASE_FIRST;

    const int joker = m->pII.jokerIndices[0];
    declare_seat(ctx, joker);
    declare_seat(ctx, joker);
    declare_seat(ctx, joker);
    tick_ingest(ctx);

    int atBat = 0;
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        if (m->playerInfo[i].bTPI.state == PLAYER_STATE_AT_BAT) atBat++;
    }
    ASSERT_EQ(1, atBat, "three copies of one request must seat exactly one batter");
    ASSERT_EQ(joker, get_active_batter_index(m), "and it must be the one named");
    ASSERT_EQ(0, ctx->state->channels.batting.count, "the channel is drained within the tick");
    ASSERT_EQ(0, ctx->state->channels.batting.overflowed, "and nothing was dropped for want of room");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

/**
 * 6. A request with no decision on the table is refused.
 *
 * §27 puts the choice at a moment — the previous batter has become permanently a runner — and not
 * before. Without this, a producer could seat a second batter beside the one already at the plate,
 * which the state validator would then catch as two active batters: a refusal at the door is better
 * than a fatal at the end of the frame.
 */
int test_seat_refuses_when_no_decision_is_open(void)
{
    ScenarioContext* ctx = create_scenario();
    int inTurnRegular;
    setup_order_spent_with_jokers(ctx, &inTurnRegular);
    MatchSession* m = ctx->state->match;

    m->flowControl.waitingForBatterDecision = 0;

    declare_seat(ctx, m->pII.jokerIndices[0]);
    tick_ingest(ctx);

    ASSERT_EQ(-1, get_active_batter_index(m), "nobody was asked to take the bat, so nobody may");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
