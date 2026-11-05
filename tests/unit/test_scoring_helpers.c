#include "test_helpers.h"
#include "scoring_helpers.h"
#include <string.h>

/*
 * Test should_period_end() — the pure function that determines
 * if a pesäpallo period should end after a run is scored.
 *
 * halfInningsInPeriod = 4 means 2 full innings (4 half-innings) per period.
 * Inning numbering is 0-based half-innings:
 *   Period 0: innings 0,1,2,3
 *   Period 1: innings 4,5,6,7
 *   Super inning: innings 8,9
 *   HR contest: innings 0,1 (period >= 4)
 */

int test_should_period_end_last_half_inning_lead(void)
{
    Scoreboard sb;
    memset(&sb, 0, sizeof(Scoreboard));
    sb.halfInningsInPeriod = 4;

    // Last half-inning of period 0 (inning 3), batting leads 5-4
    sb.inning = 3;
    sb.period = 0;
    ASSERT_EQ(1, should_period_end(&sb, 5, 4, 0, 0), "Period ends when batting team leads in last half-inning");

    return TEST_PASSED;
}

int test_should_period_end_last_half_inning_no_lead(void)
{
    Scoreboard sb;
    memset(&sb, 0, sizeof(Scoreboard));
    sb.halfInningsInPeriod = 4;

    // Last half-inning of period 0 (inning 3), tied 4-4
    sb.inning = 3;
    sb.period = 0;
    ASSERT_EQ(0, should_period_end(&sb, 4, 4, 0, 0), "Period does NOT end when tied in last half-inning");

    // Last half-inning, catching team ahead 3-5
    ASSERT_EQ(0, should_period_end(&sb, 3, 5, 0, 0), "Period does NOT end when batting team is behind");

    return TEST_PASSED;
}

int test_should_period_end_mid_period(void)
{
    Scoreboard sb;
    memset(&sb, 0, sizeof(Scoreboard));
    sb.halfInningsInPeriod = 4;

    // Mid-period (inning 1), batting leads — should NOT trigger
    sb.inning = 1;
    sb.period = 0;
    ASSERT_EQ(0, should_period_end(&sb, 5, 2, 0, 0), "Period does NOT end mid-period even with big lead");

    // Inning 2 (3rd half-inning of period 0)
    sb.inning = 2;
    ASSERT_EQ(0, should_period_end(&sb, 10, 0, 0, 0), "Period does NOT end in non-last half-inning");

    return TEST_PASSED;
}

int test_should_period_end_period1_last(void)
{
    Scoreboard sb;
    memset(&sb, 0, sizeof(Scoreboard));
    sb.halfInningsInPeriod = 4;

    // Last half-inning of period 1 (inning 7), batting leads
    sb.inning = 7;
    sb.period = 1;
    ASSERT_EQ(1, should_period_end(&sb, 6, 3, 0, 0), "Period 1 ends when batting leads in last half-inning");

    return TEST_PASSED;
}

int test_should_period_end_period0_tiebreaker(void)
{
    Scoreboard sb;
    memset(&sb, 0, sizeof(Scoreboard));
    sb.halfInningsInPeriod = 4;

    // End of period 1 (inning 7), overall tied 4-4,
    // but batting team won period 0 (period0Runs: 3 > 2)
    sb.inning = 7;
    sb.period = 1;
    ASSERT_EQ(1, should_period_end(&sb, 4, 4, 3, 2), "Period ends when tied but batting team won period 0");

    // Same but catching team won period 0 — should NOT end
    ASSERT_EQ(0, should_period_end(&sb, 4, 4, 2, 3), "Period does NOT end when tied and catching team won period 0");

    // Period0 tiebreaker only applies at inning 7 (halfInningsInPeriod * 2 - 1)
    // NOT at inning 3 (end of period 0 itself)
    sb.inning = 3;
    sb.period = 0;
    ASSERT_EQ(0, should_period_end(&sb, 4, 4, 3, 2), "Period0 tiebreaker does NOT apply at end of period 0 itself");

    return TEST_PASSED;
}

int test_should_period_end_super_inning(void)
{
    Scoreboard sb;
    memset(&sb, 0, sizeof(Scoreboard));
    sb.halfInningsInPeriod = 4;

    // Super inning: period 2, inning = halfInningsInPeriod * 2 = 8 (1st half)
    // Last half-inning of super inning: inning 9
    // Check: inning+1 == halfInningsInPeriod*2+2 → 10 == 10
    sb.inning = 9;
    sb.period = 2;
    ASSERT_EQ(1, should_period_end(&sb, 2, 1, 0, 0), "Super inning ends when batting leads in final half-inning");

    // First half-inning of super inning (inning 8) — should NOT end
    sb.inning = 8;
    ASSERT_EQ(0, should_period_end(&sb, 5, 0, 0, 0), "Super inning does NOT end in first half-inning");

    return TEST_PASSED;
}

int test_should_period_end_homerun_contest(void)
{
    Scoreboard sb;
    memset(&sb, 0, sizeof(Scoreboard));
    sb.halfInningsInPeriod = 4;

    // HR contest: period 4, check after even half-inning (inning 1)
    sb.period = 4;
    sb.inning = 1;
    ASSERT_EQ(1, should_period_end(&sb, 3, 1, 0, 0), "HR contest ends when batting leads after full round");

    // Odd half-inning (inning 0) — should NOT end
    sb.inning = 0;
    ASSERT_EQ(0, should_period_end(&sb, 10, 0, 0, 0), "HR contest does NOT end mid-round (odd half-inning count)");

    // Even half-inning, tied — should NOT end
    sb.inning = 1;
    ASSERT_EQ(0, should_period_end(&sb, 3, 3, 0, 0), "HR contest does NOT end when tied");

    return TEST_PASSED;
}
