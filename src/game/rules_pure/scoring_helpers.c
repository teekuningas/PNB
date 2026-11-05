#include "scoring_helpers.h"

int should_period_end(
    const Scoreboard* sb, int batting_runs, int catching_runs, int batting_period0_runs, int catching_period0_runs
)
{
    if (sb->period < 4) {
        // Normal game / Super inning: check if this is the last half-inning of a period
        int isLastHalfInning =
            (sb->inning + 1) % sb->halfInningsInPeriod == 0 || sb->inning + 1 == sb->halfInningsInPeriod * 2 + 2;

        if (isLastHalfInning) {
            // Batting team has taken the lead — opponent won't bat again
            if (batting_runs > catching_runs) return 1;

            // Period 1 tiebreaker: overall score tied, but period 0 winner is ahead
            if (sb->inning + 1 == sb->halfInningsInPeriod * 2 && batting_period0_runs > catching_period0_runs &&
                catching_runs == batting_runs) {
                return 1;
            }
        }
    } else {
        // Homerun contest: check after each full round (even half-inning)
        if ((sb->inning + 1) % 2 == 0) {
            if (batting_runs > catching_runs) return 1;
        }
    }
    return 0;
}
