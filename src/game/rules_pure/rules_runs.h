#ifndef RULES_RUNS_H
#define RULES_RUNS_H

#include "globals.h"

/**
 * Calculates whether a run is scored.
 *
 * References:
 * - §41 Juoksu (Run): A run is scored when a runner safely advances from the 3rd base to the home base.
 * - §42 Kunniajuoksu (Run of Honor): A run of honor is scored when a batter advances to the 3rd base
 *   during their own turn at bat (on the same pitch/hit), and the ball is still in play.
 *
 * @param player_base Current base ID of the player (BASE_HOME_SCORED for home, BASE_THIRD for 3rd base).
 * @param player_original_base The base the player started from before this play.
 * @param player_is_wounded Whether the player has been wounded (put out) during the play.
 * @param gameModeState Current game mode state containing can_make_run_of_honor.
 * @param has_made_run_on_third_base Whether the runner already scored a run of honor.
 * @return 1 if a run should be scored, 0 otherwise.
 */
int calculate_runs(BaseID player_base, BaseID player_original_base, int player_is_wounded, const GameModeState* gameModeState, int has_made_run_on_third_base);

#endif // RULES_RUNS_H