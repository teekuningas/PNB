#include "test_helpers.h"
#include "player_utils.h"
#include "globals.h"
#include <string.h>

int test_get_active_batter_index()
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

int test_get_batting_team_index()
{
    Scoreboard sb;
    memset(&sb, 0, sizeof(Scoreboard));

    // Period 0, inning 0, playsFirst 0 → (0+0+0) % 2 = 0
    sb.inning = 0; sb.period = 0; sb.playsFirst = 0;
    ASSERT_EQ(0, get_batting_team_index(&sb), "Period 0, inning 0, playsFirst 0 → team 0 bats");

    // Period 0, inning 1 → (1+0+0) % 2 = 1
    sb.inning = 1;
    ASSERT_EQ(1, get_batting_team_index(&sb), "Period 0, inning 1 → team 1 bats");

    // Period 0, inning 2 → (2+0+0) % 2 = 0
    sb.inning = 2;
    ASSERT_EQ(0, get_batting_team_index(&sb), "Period 0, inning 2 → team 0 bats again");

    // playsFirst flips the order: inning 0, playsFirst 1 → (0+1+0) % 2 = 1
    sb.inning = 0; sb.playsFirst = 1;
    ASSERT_EQ(1, get_batting_team_index(&sb), "playsFirst=1 flips team 0→1");

    // Period 1 flips again: inning 0, playsFirst 0, period 1 → (0+0+1) % 2 = 1
    sb.playsFirst = 0; sb.period = 1;
    ASSERT_EQ(1, get_batting_team_index(&sb), "Period 1 flips batting team");

    // HR contest: period 4, inning 0 → (0+0+4) % 2 = 0
    sb.period = 4; sb.inning = 0;
    ASSERT_EQ(0, get_batting_team_index(&sb), "HR contest period 4, inning 0 → team 0");

    // HR contest: period 4, inning 1 → (1+0+4) % 2 = 1
    sb.inning = 1;
    ASSERT_EQ(1, get_batting_team_index(&sb), "HR contest period 4, inning 1 → team 1");

    // HR contest round 2: period 6, inning 0 → (0+0+6) % 2 = 0
    sb.period = 6; sb.inning = 0;
    ASSERT_EQ(0, get_batting_team_index(&sb), "HR contest period 6 → team 0");

    return TEST_PASSED;
}