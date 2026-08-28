#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include "rules_pure/player_utils.h"
#include "execute_actions.h" // intent_push — these tests declare the seating as a producer does

/*
 * §12 Vuoronvaihto — the half-inning ends on the designated last batter, not on a pool of nine.
 * Rule and worked example: the official pesäpallo rulebook, §12 (Vuoronvaihto).
 *
 * These are scenario-tier tests, so everything below goes through a real mechanism: batters take the
 * bat by declaring the INTENT_SELECT_BATTER a human key or the AI controller declares, and they
 * leave it by running through the engine's own base-running machinery. Nothing sets `bTPI.state` or
 * emits a `gameEvents` flag by hand — a test that manufactures the state it is checking knights
 * nothing. The one thing these tests do set directly is referee-owned LEGAL state
 * (a burn), which is this tier's stated power and what `test_burnt_player_bats_again` already does.
 */

// Offer the bat through the real selection path: the intent, the flow gate, and batting_system's
// seating — which is what actually emits batterEntered.
//
// Then WAIT for the player to physically arrive. `seat_batter` only sets a walk target, so the
// legal seating is instant while the body is still crossing the field — and a test that starts the
// at-bat before the body lands is measuring a batter who is standing somewhere else entirely. The
// engine's own "in place with the bat" signal is `batter_ready` (§27's *asettuu lyömään*), so that is
// what to wait on rather than a frame count guessed from nothing.
static int take_the_bat(ScenarioContext* ctx, int playerIndex)
{
    ctx->state->match->flowControl.waitingForBatterDecision = 1;

    for (int i = 0; i < 600; i++) {
        // The producer's half of §27, restated every frame exactly as a real one does: the seat may
        // not be free yet, and saying the same true thing again is how a producer waits without
        // knowing that it is waiting.
        intent_push(
            &ctx->state->channels.batting,
            (IntentMessage){.kind = INTENT_SELECT_BATTER, .as.select_batter = {.index = playerIndex}}
        );
        simulate_frames(ctx, 1);
        if (ctx->state->match->pRAI.batter_ready == 1 && get_active_batter_index(ctx->state->match) == playerIndex) {
            return 1;
        }
    }
    return 0;
}

// Bring the batting order round so `playerIndex` is the lyöntivuoroinen player.
//
// §27 is a cycle followed for the whole match, so a regular may only be seated when the order
// reaches them — and the INGEST gate now refuses anything else. Walking eight intervening at-bats to
// prove a §12(3) claim would make the test about the lap rather than about the last batter, so the
// order index is set directly instead. That is scoreboard LEGAL state, the same class this file
// already constructs a burn in, and the setup declares "the order has come round" rather than
// pretending to have played it.
//
// Worth recording that this helper only exists because the gate found the old version illegal: the
// test used to hand the bat to whoever it liked, because nothing could refuse it.
static void bring_the_order_round_to(ScenarioContext* ctx, int playerIndex)
{
    Scoreboard* sb = &ctx->state->rules->scoreboard;
    TeamInfo* team = &sb->teams[get_batting_team_index(sb)];
    for (int i = 0; i < PLAYERS_IN_TEAM; i++) {
        if (team->batterOrder[i] == playerIndex) {
            team->batterOrderIndex = i;
            return;
        }
    }
}

// End the at-bat the way §27 means it: the batter must become *lopullisesti* a runner, which means
// actually REACHING first — a batter who merely set off and got nowhere is sent back to the plate
// when the ball comes home and is still in batting turn. A pitch has to come first, because the
// batter is only released to run once the ball has left the lukkari's hand (§32, `batter_can_advance`);
// running on the first try rather than the third is the batter's own choice and equally legal.
static int run_to_first(ScenarioContext* ctx, int playerIndex)
{
    trigger_player_run_to_next_base(ctx, playerIndex, BASE_HOME);

    for (int i = 0; i < 400; i++) {
        simulate_frames(ctx, 1);
        if (ctx->state->rules->referee.battingPlayers[playerIndex].currentSafetyBase == BASE_FIRST) return 1;
    }
    return 0;
}

static int reach_first_base(ScenarioContext* ctx, int playerIndex)
{
    perform_pitch(ctx, 0.0f);
    simulate_frames(ctx, 3);
    return run_to_first(ctx, playerIndex);
}

// The fielders return the ball to the lukkari at home — §12's other conjunct, "ja pallo tulee
// kotipesässä olevan ulkopelaajan haltuun".
static void return_the_ball_home(ScenarioContext* ctx)
{
    give_ball_to_pitcher(ctx);
    simulate_frames(ctx, 3);
}

// Clear a runner off the field as WOUNDED, not burnt. That distinction is the whole reason this
// scenario is reachable at all: *haavoittunut* (§36) removes a runner without costing an out, so a
// scoreless half-inning really can run a full lap of the order with fewer than three burns. Referee-
// owned legal state, actualized by consolidation — the same construction test_burnt_player_bats_again
// uses, and it deliberately leaves `outs` alone so these tests can never end by §12(1) instead.
static void wound_the_runner(ScenarioContext* ctx, int playerIndex)
{
    ctx->state->rules->referee.battingPlayers[playerIndex].status = PLAYER_STATUS_WOUNDED;
    simulate_frames(ctx, 3);
}

// One run, the plainest way there is: a runner already on third breaks for home while the ball lands
// untouched in the outfield. Repeatable — each call opens a fresh pitch first, so the referee
// re-snapshots the baseline and the previous play's sticky verdicts are cleared.
static void score_a_run_from_third(ScenarioContext* ctx, int runnerIndex)
{
    place_runner_at_base(ctx, runnerIndex, BASE_THIRD, 0.0f);
    snapshot_pitch_start_state(ctx);
    ctx->state->match->playerRuntime[runnerIndex].passedPathPoint = 1;

    Vector3D outfield = {30.0f, 0.0f, 40.0f};
    place_ball_over_location(ctx, outfield);
    trigger_player_run_to_next_base(ctx, runnerIndex, BASE_THIRD);

    simulate_frames(ctx, 450);
}

/**
 * §12(3) — after two runs, the side change waits for the designated last batter.
 *
 * Written RED on purpose: before the last-batter slice the engine refilled a phantom
 * pool of nine batters on every second run and ended the half-inning only when that pool AND the
 * jokers were spent, so the state built below produced no side change at all. The assertions are all
 * in the old vocabulary — a designation the engine makes, and a side change it does or does not
 * pronounce — which is what let the test be red before the fields it describes existed.
 */
int test_last_batter_ends_half_inning(void)
{
    ScenarioContext* ctx = create_scenario();
    StateInfo* st = ctx->state;
    GameRulesState* rules = st->rules;

    int battingTeamIndex = get_batting_team_index(&rules->scoreboard);
    int orderIndex = rules->scoreboard.teams[battingTeamIndex].batterOrderIndex;
    int designated = rules->scoreboard.teams[battingTeamIndex].batterOrder[orderIndex];

    initialize_referee_from_physical_state(ctx);
    ASSERT(take_the_bat(ctx, designated), "setup: the first batter should take the plate");

    // Two runners score while they are at the plate. The SECOND run is the even-numbered one, and the
    // designation names whoever held the right to advance from home when it happened — them.
    score_a_run_from_third(ctx, 1);
    ASSERT_EQ(1, rules->halfInningState.runsInTheInning, "setup: the first runner should have scored");
    score_a_run_from_third(ctx, 2);
    ASSERT_EQ(2, rules->halfInningState.runsInTheInning, "setup: the second runner should have scored");
    ASSERT_EQ(
        designated, rules->halfInningState.lastBatter.designatedIndex,
        "the batter of the even-numbered run becomes the viimeinen lyöjä"
    );

    // That at-bat now ends. This is NOT the moment §12(3) speaks about: the rule waits for the last
    // batter to come to bat *again* ("uudestaan lyöntivuoroon tultuaan"), and the at-bat they were
    // designated during does not count. Ball home, jokers gone, and still no side change.
    exhaust_jokers(ctx);
    ASSERT(reach_first_base(ctx, designated), "setup: the designated batter should reach first base");
    return_the_ball_home(ctx);
    ASSERT_EQ(
        END_INNING_STATE_NONE, rules->referee.endOfInningState,
        "the at-bat the last batter was designated during is not the at-bat that ends the half-inning"
    );

    // The order comes round and they take the bat again. While they are AT THE PLATE the turn is being
    // taken, not spent — ball home and no jokers must not end the half-inning under a batter's feet.
    wound_the_runner(ctx, designated);
    bring_the_order_round_to(ctx, designated);
    ASSERT(take_the_bat(ctx, designated), "setup: the last batter should be back at the plate");
    return_the_ball_home(ctx);
    ASSERT_EQ(
        END_INNING_STATE_NONE, rules->referee.endOfInningState,
        "a turn being taken is not a turn that is spent — no side change while the last batter bats"
    );

    // Now it concludes, and properly: they reach first, so they are *lopullisesti* a runner.
    ASSERT(reach_first_base(ctx, designated), "setup: the last batter should reach first base again");
    return_the_ball_home(ctx);

    EndOfInningTransitionState endState = rules->referee.endOfInningState;
    int outs = rules->halfInningState.outs;
    cleanup_scenario(ctx);

    ASSERT(outs < 3, "the side change under test must be §12(3)'s, not three burns");
    ASSERT_NE(
        END_INNING_STATE_NONE, endState,
        "the half-inning must end when the designated last batter's turn has come round again"
    );
    return TEST_PASSED;
}

/**
 * §12(2) — with fewer than two runs, the half-inning ends when the player who OPENED it is due to bat
 * a second time: one full lap of the order, not a pool that empties.
 *
 * This is the clause the phantom pool happened to imitate — `nonJokerPlayersLeft` also ran out after
 * nine at-bats — so the end state alone would not have distinguished them. What distinguishes them is
 * the per-at-bat check inside the loop: a pool spent early by a run refill ends the turn at the wrong
 * at-bat, and the order coming round is the only thing that may end it here.
 */
int test_scoreless_lap_of_the_order_ends_half_inning(void)
{
    ScenarioContext* ctx = create_scenario();
    StateInfo* st = ctx->state;
    GameRulesState* rules = st->rules;
    int battingTeamIndex = get_batting_team_index(&rules->scoreboard);

    initialize_referee_from_physical_state(ctx);
    exhaust_jokers(ctx);

    int opener = rules->scoreboard.teams[battingTeamIndex]
                     .batterOrder[rules->scoreboard.teams[battingTeamIndex].batterOrderIndex];

    for (int atBat = 0; atBat < PLAYERS_IN_TEAM; atBat++) {
        int next = rules->scoreboard.teams[battingTeamIndex]
                       .batterOrder[rules->scoreboard.teams[battingTeamIndex].batterOrderIndex];
        ASSERT(take_the_bat(ctx, next), "each player in the order should take the plate in turn");
        ASSERT_EQ(next, get_active_batter_index(st->match), "the order should offer each player in turn");

        ASSERT(reach_first_base(ctx, next), "each batter should reach first base");
        return_the_ball_home(ctx);

        if (atBat < PLAYERS_IN_TEAM - 1) {
            ASSERT_EQ(
                END_INNING_STATE_NONE, rules->referee.endOfInningState,
                "the order has not come round yet — the half-inning must continue"
            );
            wound_the_runner(ctx, next);
        }
    }

    int nextUp = rules->scoreboard.teams[battingTeamIndex]
                     .batterOrder[rules->scoreboard.teams[battingTeamIndex].batterOrderIndex];
    EndOfInningTransitionState endState = rules->referee.endOfInningState;
    int runs = rules->halfInningState.runsInTheInning;
    int outs = rules->halfInningState.outs;
    cleanup_scenario(ctx);

    ASSERT_EQ(0, runs, "setup: this clause is about a half-inning with fewer than two runs");
    ASSERT(outs < 3, "the side change under test must be §12(2)'s, not three burns");
    ASSERT_EQ(opener, nextUp, "setup: nine at-bats should bring the order back to the opening batter");
    ASSERT_NE(
        END_INNING_STATE_NONE, endState,
        "the half-inning must end when the opening batter is due a second time, ball home, no jokers"
    );
    return TEST_PASSED;
}

/**
 * §12(2) again, with a joker opening the half-inning — and §7 deciding what that means.
 *
 * §27 lets a team put a joker in before the in-turn player has taken the bat ("Jokeripelaajan voi
 * asettaa lyömään, ellei lyöntivuoroinen pelaaja ole asettunut lyömään"), so a half-inning really
 * can open with one. §7 then says what it does NOT do: "Jokeripelaaja ei vie kenenkään
 * lyöntivuoroa." The joker takes nobody's turn, so §12(2)'s *vuoron aloittanut pelaaja* is still
 * the regular whose turn it was — the joker batted beside the order, not inside it.
 *
 * Why this is a defect and not a nicety: §12(2) is decided by comparing the designation against
 * `batterOrderIndex`, which only ever names one of the nine regular slots. A designation sitting on
 * a joker can therefore never be equal to it, so the clause is not merely wrong for one at-bat —
 * it is unfirable for the whole half-inning, which can then only end on three burns. The sim tier
 * reaches this state in a third of its half-innings and cannot see the consequence, because no
 * half-inning there ends by §12 at all.
 *
 * The sibling test above deliberately spends the jokers before the first at-bat, which is exactly
 * why it never covered this: with no joker available, the half-inning cannot open with one.
 */
int test_joker_opening_does_not_take_the_turn(void)
{
    ScenarioContext* ctx = create_scenario();
    StateInfo* st = ctx->state;
    GameRulesState* rules = st->rules;
    int battingTeamIndex = get_batting_team_index(&rules->scoreboard);

    initialize_referee_from_physical_state(ctx);

    // The regular whose turn it is. Nothing below may take this away from them.
    int opener = rules->scoreboard.teams[battingTeamIndex]
                     .batterOrder[rules->scoreboard.teams[battingTeamIndex].batterOrderIndex];

    // A joker opens the half-inning, which is legal and which the batting AI does routinely.
    int joker = st->match->pII.jokerIndices[0];
    ASSERT(take_the_bat(ctx, joker), "setup: a joker should be able to open the half-inning");
    ASSERT_EQ(
        opener,
        rules->scoreboard.teams[battingTeamIndex]
            .batterOrder[rules->scoreboard.teams[battingTeamIndex].batterOrderIndex],
        "§7: a joker taking the bat must not advance the batting order"
    );
    ASSERT_NE(
        joker, rules->halfInningState.lastBatter.designatedIndex,
        "§7/§12(2): a joker takes nobody's batting turn, so it cannot be the vuoron aloittanut pelaaja"
    );

    ASSERT(reach_first_base(ctx, joker), "setup: the joker should reach first base");
    return_the_ball_home(ctx);
    wound_the_runner(ctx, joker);

    // Now the order runs its lap. The remaining jokers go before the last at-bat concludes, since
    // §12(2)'s third conjunct is "eikä jokeripelaajia ole enää käytettävissä".
    exhaust_jokers(ctx);

    for (int atBat = 0; atBat < PLAYERS_IN_TEAM; atBat++) {
        int next = rules->scoreboard.teams[battingTeamIndex]
                       .batterOrder[rules->scoreboard.teams[battingTeamIndex].batterOrderIndex];
        ASSERT(take_the_bat(ctx, next), "each player in the order should take the plate in turn");
        ASSERT(reach_first_base(ctx, next), "each batter should reach first base");
        return_the_ball_home(ctx);

        if (atBat < PLAYERS_IN_TEAM - 1) {
            ASSERT_EQ(
                END_INNING_STATE_NONE, rules->referee.endOfInningState,
                "the order has not come round yet — the half-inning must continue"
            );
            wound_the_runner(ctx, next);
        }
    }

    int designated = rules->halfInningState.lastBatter.designatedIndex;
    EndOfInningTransitionState endState = rules->referee.endOfInningState;
    int runs = rules->halfInningState.runsInTheInning;
    int outs = rules->halfInningState.outs;
    cleanup_scenario(ctx);

    ASSERT_EQ(0, runs, "setup: this clause is about a half-inning with fewer than two runs");
    ASSERT(outs < 3, "the side change under test must be §12(2)'s, not three burns");
    ASSERT_EQ(opener, designated, "the opening designation belongs to the regular whose turn it was");
    ASSERT_NE(
        END_INNING_STATE_NONE, endState,
        "a half-inning opened by a joker must still end when the order comes round to its opener"
    );
    return TEST_PASSED;
}
