#ifndef RULES_SIDE_CHANGE_H
#define RULES_SIDE_CHANGE_H

/*
 * §12 Vuoronvaihto — the batting-order half of the side change.
 *
 * The two predicates here are the whole of what the rulebook says about WHEN a batting turn is
 * spent. They are deliberately written in plain ints and include nothing: a side change is a
 * statement about a batting order and a run count, not about bodies, bases or a ball. Everything
 * physical in §12 — "pallo tulee kotipesässä olevan ulkopelaajan haltuun" — and the joker conjunct
 * are the referee's to conjoin at the call site, where that state actually lives.
 *
 * Rule text: docs PNB/RULES.md §3a, verbatim from reference/RULES_OFFICIAL.md §12.
 */

/**
 * @brief Who becomes the *viimeinen lyöjä* when an even-numbered run is scored.
 *
 * "Kahden saadun juoksun ja kunkin kahden lisäjuoksun jälkeen viimeiseksi lyöjäksi jää se
 * lyöntivuoroinen sisäpelaaja, jolla oli viimeksi etenemisoikeus kotipesästä juoksun syntyessä.
 * Jokeripelaajan lyödessä juoksun on viimeinen lyöjä lyöntijärjestyksessä edellinen varsinainen
 * pelaaja."
 *
 * Both sentences collapse into one lookup. A joker never takes anyone's batting turn (§7), so the
 * most recent REGULAR player to take the bat is simultaneously "the batter, if the batter is
 * regular" and "the previous regular player in the order, if a joker batted the run".
 *
 * @param last_regular_batter_index The most recent regular player to take the bat this half-inning,
 *        or -1 if none has (a half-inning opened by a joker).
 * @param current_designation Today's designation, returned unchanged when there is no regular to name.
 * @return The player index that is now the last batter.
 */
int designate_last_batter(int last_regular_batter_index, int current_designation);

/**
 * @brief Has the designated last batter's turn come round, in the sense §12(2)/(3) means?
 *
 * The caller must already have established that a batting turn just CONCLUDED — "kun edellinen
 * lyöjä on muuttunut lopullisesti etenijäksi" (§27). Given that, the two clauses differ by exactly
 * one at-bat, and that is the only thing this function decides:
 *
 *   §12(2), fewer than two runs: the subject is the player who opened the half-inning, and the
 *   moment is *"kun vuoron aloittanut pelaaja tulee toisen kerran lyöntivuoroon"* — as they come to
 *   bat. So: the turn that just ended was the one before theirs, i.e. they are next in the order.
 *
 *   §12(3), two runs or more: the subject is the designated last batter, and the moment is *"kun
 *   viimeinen lyöjä uudestaan lyöntivuoroon tultuaan muuttuu lopullisesti etenijäksi"* — as their
 *   own at-bat ends. So: they have taken the bat since being designated, and that turn just ended.
 *
 * @param designated_index The current *viimeinen lyöjä*; -1 before the half-inning's first batter.
 * @param has_batted_again 1 once the designated player has taken the bat since being designated.
 * @param runs_in_the_inning Runs scored by the batting team this half-inning (runs of honour count).
 * @param next_in_order_index The player the batting order offers next.
 * @return 1 if the batting order has run its course, 0 otherwise.
 */
int is_last_batter_turn_reached(
    int designated_index, int has_batted_again, int runs_in_the_inning, int next_in_order_index
);

#endif /* RULES_SIDE_CHANGE_H */
