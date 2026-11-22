#include "test_helpers.h"
#include "rules_side_change.h"

// §12 Vuoronvaihto — the official pesäpallo rulebook.

int test_designate_last_batter(void)
{
    // "…se lyöntivuoroinen sisäpelaaja, jolla oli viimeksi etenemisoikeus kotipesästä juoksun
    // syntyessä" — with a regular batter at the plate, that is simply the batter.
    ASSERT_EQ(5, designate_last_batter(5, 2), "the regular batter of the even run becomes the last batter");

    // "Jokeripelaajan lyödessä juoksun on viimeinen lyöjä lyöntijärjestyksessä edellinen varsinainen
    // pelaaja." A joker takes nobody's turn (§7), so the most recent regular batter IS that player —
    // the same lookup answers both sentences, which is why there is only one argument for it.
    ASSERT_EQ(7, designate_last_batter(7, 3), "a joker's run designates the previous regular player");

    // A half-inning opened by a joker has no regular to name yet; the standing designation holds.
    ASSERT_EQ(2, designate_last_batter(-1, 2), "with no regular batter yet, the designation is unchanged");
    ASSERT_EQ(-1, designate_last_batter(-1, -1), "…including when there is no designation at all");

    return TEST_PASSED;
}

int test_is_last_batter_turn_reached(void)
{
    // Before the half-inning's first batter there is no designation, so no clause can fire.
    ASSERT_EQ(0, is_last_batter_turn_reached(-1, 0, 0, 4), "no designation ⇒ the turn cannot be spent");
    ASSERT_EQ(0, is_last_batter_turn_reached(-1, 1, 5, 4), "…whatever the runs and the order say");

    // §12(2), fewer than two runs: the subject is the opening batter and the moment is their coming
    // to bat again — i.e. the order now offers them.
    ASSERT_EQ(1, is_last_batter_turn_reached(4, 0, 0, 4), "scoreless: the order coming round spends the turn");
    ASSERT_EQ(1, is_last_batter_turn_reached(4, 0, 1, 4), "one run is still 'fewer than two'");
    ASSERT_EQ(0, is_last_batter_turn_reached(4, 0, 1, 5), "…but not until the order actually reaches them");
    // Under (2) the designated player's own past at-bats are irrelevant — the clause fires BEFORE
    // they bat, not after. This is the one at-bat by which (2) and (3) differ.
    ASSERT_EQ(0, is_last_batter_turn_reached(4, 1, 1, 5), "having batted before does not spend the turn under (2)");

    // §12(3), two runs or more: the subject is the designated last batter and the moment is the end
    // of their next at-bat — so what matters is that they have taken the bat since being designated.
    ASSERT_EQ(1, is_last_batter_turn_reached(4, 1, 2, 7), "after two runs: the last batter has batted again");
    ASSERT_EQ(0, is_last_batter_turn_reached(4, 0, 2, 4), "being merely due to bat is not enough under (3)");
    ASSERT_EQ(1, is_last_batter_turn_reached(4, 1, 9, 0), "each further pair re-designates; the shape is the same");

    return TEST_PASSED;
}
