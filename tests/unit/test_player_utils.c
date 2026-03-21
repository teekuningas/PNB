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