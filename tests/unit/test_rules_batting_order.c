#include "test_helpers.h"
#include "rules_batting_order.h"
#include "batting_ai_strategy.h" // choose_batter — the controller's preference over the legal set

// §27 Lyöntijärjestys, §12 Vuoronvaihto, §7 Joukkue — who may take the bat.
//
// The contract tier proves the INGEST gate refuses the illegal request and the scripted tier proves a
// human is never offered one. Both drive the rule through a running engine. This proves the rule
// itself, in isolation and instantly — which matters more here than usual, because the same function
// answers for two callers with opposite jobs: the client asks it what to OFFER and the gate asks it
// what to REFUSE. A predicate that is subtly wrong is wrong in both directions at once.

int test_batter_seat_verdict_regular_in_turn(void)
{
    // §27: "Pöytäkirjaan merkittyä lyöntijärjestystä on noudatettava koko ottelun ajan." The
    // *lyöntivuoroinen* player may bat, and no other regular may.
    ASSERT_EQ(
        SEAT_ALLOWED, batter_seat_verdict(4, 4, 0, JOKER_STATUS_NOT_A_JOKER), "the in-turn regular may take the bat"
    );
    ASSERT_EQ(
        SEAT_NOT_IN_BATTING_TURN, batter_seat_verdict(5, 4, 0, JOKER_STATUS_NOT_A_JOKER),
        "a regular whose turn it is not may not, however available he looks"
    );
    // The order is a CYCLE, so "not in turn" is never "has had his turn" — it is somebody else's, and
    // the player one place BEHIND is refused for exactly the same reason as one ahead.
    ASSERT_EQ(
        SEAT_NOT_IN_BATTING_TURN, batter_seat_verdict(3, 4, 0, JOKER_STATUS_NOT_A_JOKER),
        "the player who has just batted is not in turn either"
    );
    return TEST_PASSED;
}

int test_batter_seat_verdict_spent_order(void)
{
    // §12(2)/(3): once the order has come round to the designated last batter, no further REGULAR may
    // be seated — the rulebook's own worked example calls seating one an error that voids the actions
    // and whistles the side change. The in-turn player is refused even though it IS his turn: the
    // batting-order clause and the turn are two different questions.
    ASSERT_EQ(
        SEAT_REGULAR_ORDER_SPENT, batter_seat_verdict(4, 4, 1, JOKER_STATUS_NOT_A_JOKER),
        "with the order spent, not even the in-turn regular may bat"
    );
    // …and a joker still may. That is the whole of what "only a joker can extend the turn" means.
    ASSERT_EQ(
        SEAT_ALLOWED, batter_seat_verdict(9, 4, 1, JOKER_STATUS_UNUSED),
        "an unused joker extends the turn after the order is spent"
    );
    return TEST_PASSED;
}

int test_batter_seat_verdict_jokers(void)
{
    // §7: "Joukkue voi käyttää jokaisessa sisävuorossa kolmea eri jokeripelaajaa, kerran kutakin."
    ASSERT_EQ(SEAT_ALLOWED, batter_seat_verdict(10, 4, 0, JOKER_STATUS_UNUSED), "an unused joker may bat");
    ASSERT_EQ(
        SEAT_JOKER_ALREADY_USED, batter_seat_verdict(10, 4, 0, JOKER_STATUS_SPENT),
        "a joker may bat once per half-inning, and this one has"
    );
    // A joker is never IN the batting order, so he is never out of turn — the in-turn index is
    // irrelevant to him, and that is why §27 lets a team put one in at any at-bat.
    ASSERT_EQ(
        SEAT_ALLOWED, batter_seat_verdict(11, 0, 0, JOKER_STATUS_UNUSED),
        "a joker is not in the order, so being 'out of turn' cannot apply to him"
    );
    return TEST_PASSED;
}

int test_choose_batter_preference(void)
{
    // The batting controller's preference, stated rather than walked. should_change_batter says
    // "change away from this one" for a slow player on an empty field (fieldStatus 0) and for a
    // powerless one with a runner on first (fieldStatus 2).
    BatterCandidate fastFirst[2] = {{4, 0, 3}, {9, 0, 0}};
    ASSERT_EQ(4, choose_batter(fastFirst, 2, 0), "an acceptable first candidate is taken, and nothing is walked");

    BatterCandidate slowThenFast[2] = {{4, 0, 1}, {9, 0, 3}};
    ASSERT_EQ(9, choose_batter(slowThenFast, 2, 0), "an unwanted first candidate is passed over for the next");

    // The fallback the old walk had: when it had seen them all and liked none, it selected whatever
    // it had started on. Preserved exactly, because a controller that refuses to choose is the
    // batter-selection deadlock (bug #5) by another name.
    BatterCandidate allSlow[3] = {{4, 0, 1}, {9, 0, 0}, {10, 0, 2}};
    ASSERT_EQ(4, choose_batter(allSlow, 3, 0), "liking none of them still chooses the first, never nobody");

    // An empty list is the one case where there is no answer, and it must say so rather than name
    // player 0 — §12's "nobody may bat" is how a half-inning ends.
    ASSERT_EQ(-1, choose_batter(allSlow, 0, 0), "no candidates means no choice, not player zero");
    return TEST_PASSED;
}
