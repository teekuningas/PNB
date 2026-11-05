#include "test_helpers.h"
#include "player_utils.h"
#include "rules_state.h"
#include "world.h"
#include <string.h>

int test_get_active_batter_index(void)
{
    MatchSession game;
    memset(&game, 0, sizeof(MatchSession));

    ASSERT_EQ(-1, get_active_batter_index(NULL), "NULL game returns -1");
    ASSERT_EQ(-1, get_active_batter_index(&game), "Empty game returns -1");

    game.playerInfo[5].bTPI.state = PLAYER_STATE_AT_BAT;
    ASSERT_EQ(5, get_active_batter_index(&game), "Returns index of AT_BAT player");

    // Clear and test another
    game.playerInfo[5].bTPI.state = PLAYER_STATE_IDLE;
    game.playerInfo[PLAYERS_IN_TEAM + JOKER_COUNT - 1].bTPI.state = PLAYER_STATE_AT_BAT;
    ASSERT_EQ(PLAYERS_IN_TEAM + JOKER_COUNT - 1, get_active_batter_index(&game), "Returns index for last valid player");

    // Fielders (index >= limit) should not be considered
    game.playerInfo[PLAYERS_IN_TEAM + JOKER_COUNT - 1].bTPI.state = PLAYER_STATE_IDLE;
    game.playerInfo[PLAYERS_IN_TEAM + JOKER_COUNT].bTPI.state = PLAYER_STATE_AT_BAT;
    ASSERT_EQ(-1, get_active_batter_index(&game), "Fielding players are ignored even if incorrectly marked");

    return TEST_PASSED;
}

int test_get_batting_team_index(void)
{
    Scoreboard sb;
    memset(&sb, 0, sizeof(Scoreboard));

    // Period 0, inning 0, playsFirst 0 → (0+0+0) % 2 = 0
    sb.inning = 0;
    sb.period = 0;
    sb.playsFirst = 0;
    ASSERT_EQ(0, get_batting_team_index(&sb), "Period 0, inning 0, playsFirst 0 → team 0 bats");

    // Period 0, inning 1 → (1+0+0) % 2 = 1
    sb.inning = 1;
    ASSERT_EQ(1, get_batting_team_index(&sb), "Period 0, inning 1 → team 1 bats");

    // Period 0, inning 2 → (2+0+0) % 2 = 0
    sb.inning = 2;
    ASSERT_EQ(0, get_batting_team_index(&sb), "Period 0, inning 2 → team 0 bats again");

    // playsFirst flips the order: inning 0, playsFirst 1 → (0+1+0) % 2 = 1
    sb.inning = 0;
    sb.playsFirst = 1;
    ASSERT_EQ(1, get_batting_team_index(&sb), "playsFirst=1 flips team 0→1");

    // Period 1 flips again: inning 0, playsFirst 0, period 1 → (0+0+1) % 2 = 1
    sb.playsFirst = 0;
    sb.period = 1;
    ASSERT_EQ(1, get_batting_team_index(&sb), "Period 1 flips batting team");

    // HR contest: period 4, inning 0 → (0+0+4) % 2 = 0
    sb.period = 4;
    sb.inning = 0;
    ASSERT_EQ(0, get_batting_team_index(&sb), "HR contest period 4, inning 0 → team 0");

    // HR contest: period 4, inning 1 → (1+0+4) % 2 = 1
    sb.inning = 1;
    ASSERT_EQ(1, get_batting_team_index(&sb), "HR contest period 4, inning 1 → team 1");

    // HR contest round 2: period 6, inning 0 → (0+0+6) % 2 = 0
    sb.period = 6;
    sb.inning = 0;
    ASSERT_EQ(0, get_batting_team_index(&sb), "HR contest period 6 → team 0");

    return TEST_PASSED;
}
// The jokers are whoever the batting team placed after its nine regulars, offered in roster order — and
// it is the BATTING team's order that is read, which changes with the half-inning.
int test_batting_team_jokers_are_the_order_after_the_regulars(void)
{
    Scoreboard sb;
    memset(&sb, 0, sizeof(Scoreboard));
    const int order0[12] = {3, 0, 1, 2, 4, 5, 6, 7, 8, 11, 9, 10};
    const int order1[12] = {0, 1, 2, 3, 4, 5, 6, 10, 8, 7, 11, 9};
    memcpy(sb.teams[0].batterOrder, order0, sizeof(order0));
    memcpy(sb.teams[1].batterOrder, order1, sizeof(order1));

    int jokers[JOKER_COUNT];
    sb.inning = 0; // team 0 bats
    batting_team_jokers(&sb, jokers);
    ASSERT_EQ(9, jokers[0], "team 0: places 10-12 hold 11, 9, 10 — offered in roster order");
    ASSERT_EQ(10, jokers[1], "team 0: second joker");
    ASSERT_EQ(11, jokers[2], "team 0: third joker");

    sb.inning = 1; // team 1 bats
    batting_team_jokers(&sb, jokers);
    ASSERT_EQ(7, jokers[0], "team 1: its own order's places 10-12 hold 7, 11, 9");
    ASSERT_EQ(9, jokers[1], "team 1: second joker");
    ASSERT_EQ(11, jokers[2], "team 1: third joker");

    return TEST_PASSED;
}
