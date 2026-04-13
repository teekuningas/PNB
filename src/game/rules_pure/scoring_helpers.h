#ifndef SCORING_HELPERS_H
#define SCORING_HELPERS_H

#include "globals.h"

/**
 * @brief Determines if the current period should end after a run is scored.
 *
 * In pesäpallo, a period ends when the batting team takes the lead in the
 * last half-inning (the catching team won't bat again, so they can't catch up).
 *
 * Three period types are handled:
 *   - Normal periods (0-1): End at last half-inning of period if batting leads.
 *   - Super inning (period 2-3): End at last half-inning (only 2 half-innings).
 *   - Homerun contest (period 4+): End at even-numbered half-inning if batting leads.
 *
 * Special case: At the end of period 1, if period 0 winner is ahead on period0Runs
 * and the overall score is tied, the period ends (opponent can't overtake).
 *
 * @param sb The scoreboard (inning, period, halfInningsInPeriod).
 * @param batting_runs Total runs for the batting team this period.
 * @param catching_runs Total runs for the catching team this period.
 * @param batting_period0_runs Batting team's period 0 runs (for tiebreaker).
 * @param catching_period0_runs Catching team's period 0 runs (for tiebreaker).
 * @return 1 if the period should end, 0 otherwise.
 */
int should_period_end(
    const Scoreboard* sb, int batting_runs, int catching_runs, int batting_period0_runs, int catching_period0_runs
);

#endif /* SCORING_HELPERS_H */
