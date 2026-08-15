#ifndef RULES_RUNS_H
#define RULES_RUNS_H

#include "globals.h"

/**
 * Checks if a regular run is scored (player reaches home safely).
 *
 * §41 does not depend on where the runner started — reaching home unwounded is a run whether they began
 * the play as the batter or on second. (`is_run_of_honor` below DOES take the original base, because §42
 * is defined as the batter's own hit reaching third; the asymmetry between the two signatures is the rule.)
 *
 * @param player_base Current base ID of the player.
 * @param player_is_wounded Whether the player has been wounded (put out) during the play.
 * @return 1 if a regular run is scored, 0 otherwise.
 */
int is_regular_run(BaseID player_base, int player_is_wounded);

/**
 * Checks if a Run of Honor (Kunniajuoksu) is scored (batter reaches 3rd base safely).
 *
 * §42: Batter arrives at 3rd base with their own hit = 1 run + stays at 3rd.
 * Conditions: Arriving at 3rd, started at home, not wounded, not already scored.
 *
 * @param player_base Current base ID of the player.
 * @param player_original_base The base the player started from before this play.
 * @param player_is_wounded Whether the player has been wounded (put out) during the play.
 * @param has_made_run_on_third_base Whether the runner already scored a run of honor this inning.
 * @return 1 if a run of honor is scored, 0 otherwise.
 */
int is_run_of_honor(
    BaseID player_base, BaseID player_original_base, int player_is_wounded, int has_made_run_on_third_base
);

#endif // RULES_RUNS_H