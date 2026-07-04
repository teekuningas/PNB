#include "test_helpers.h"
#include "base_logic.h"
#include "globals.h"
#include <string.h>

int test_base_sequence(void)
{
    ASSERT_EQ(BASE_FIRST, base_get_next(BASE_HOME), "Home -> First");
    ASSERT_EQ(BASE_SECOND, base_get_next(BASE_FIRST), "First -> Second");
    ASSERT_EQ(BASE_THIRD, base_get_next(BASE_SECOND), "Second -> Third");
    ASSERT_EQ(BASE_HOME_SCORED, base_get_next(BASE_THIRD), "Third -> Home Scored");
    ASSERT_EQ(BASE_NONE, base_get_next(BASE_HOME_SCORED), "Home Scored -> None");
    ASSERT_EQ(BASE_NONE, base_get_next(BASE_NONE), "None -> None");

    ASSERT_EQ(BASE_HOME, base_get_prev(BASE_FIRST), "First -> Home");
    ASSERT_EQ(BASE_FIRST, base_get_prev(BASE_SECOND), "Second -> First");
    ASSERT_EQ(BASE_SECOND, base_get_prev(BASE_THIRD), "Third -> Second");
    ASSERT_EQ(BASE_THIRD, base_get_prev(BASE_HOME_SCORED), "Home Scored -> Third");

    return TEST_PASSED;
}

int test_base_properties(void)
{
    // Safe Haven (can stay there)
    ASSERT_TRUE(base_is_safe_haven(BASE_HOME), "Home is safe haven");
    ASSERT_TRUE(base_is_safe_haven(BASE_FIRST), "First is safe haven");
    ASSERT_TRUE(base_is_safe_haven(BASE_SECOND), "Second is safe haven");
    ASSERT_TRUE(base_is_safe_haven(BASE_THIRD), "Third is safe haven");
    ASSERT_TRUE(!base_is_safe_haven(BASE_HOME_SCORED), "Home Scored is not safe haven");
    ASSERT_TRUE(!base_is_safe_haven(BASE_NONE), "None is not safe haven");

    // Can Advance (next base is runnable)
    ASSERT_TRUE(base_can_advance(BASE_HOME), "Home can advance"); // -> 1st
    ASSERT_TRUE(base_can_advance(BASE_FIRST), "First can advance"); // -> 2nd
    ASSERT_TRUE(base_can_advance(BASE_SECOND), "Second can advance"); // -> 3rd
    ASSERT_TRUE(!base_can_advance(BASE_THIRD), "Third cannot advance (normal)"); // -> Score (not normal run)

    // Indexing (0-3)
    ASSERT_TRUE(base_is_index(BASE_HOME), "Home is index");
    ASSERT_TRUE(base_is_index(BASE_THIRD), "Third is index");
    ASSERT_TRUE(!base_is_index(BASE_HOME_SCORED), "Home Scored is not index");

    return TEST_PASSED;
}

int test_base_comparisons(void)
{
    ASSERT_TRUE(base_is_at_least(BASE_SECOND, BASE_FIRST), "2nd >= 1st");
    ASSERT_TRUE(base_is_at_least(BASE_SECOND, BASE_SECOND), "2nd >= 2nd");
    ASSERT_TRUE(!base_is_at_least(BASE_FIRST, BASE_SECOND), "1st !>= 2nd");

    ASSERT_TRUE(base_cmp(BASE_THIRD, BASE_SECOND) > 0, "3rd > 2nd");
    ASSERT_TRUE(base_cmp(BASE_FIRST, BASE_SECOND) < 0, "1st < 2nd");
    ASSERT_TRUE(base_cmp(BASE_HOME, BASE_HOME) == 0, "Home == Home");

    return TEST_PASSED;
}

int test_base_to_int_index(void)
{
    ASSERT_EQ(0, base_to_int_index(BASE_HOME), "Home is 0");
    ASSERT_EQ(1, base_to_int_index(BASE_FIRST), "First is 1");
    ASSERT_EQ(2, base_to_int_index(BASE_SECOND), "Second is 2");
    ASSERT_EQ(3, base_to_int_index(BASE_THIRD), "Third is 3");

    ASSERT_EQ(-1, base_to_int_index(BASE_HOME_SCORED), "Home Scored is not index");
    ASSERT_EQ(-1, base_to_int_index(BASE_NONE), "None is not index");
    return TEST_PASSED;
}

int test_player_is_safe_from_fly(void)
{
    ASSERT_TRUE(player_is_safe_from_fly(PLAYER_STATE_ADVANCING_FREELY, BASE_SECOND, BASE_FIRST), "Free walk is safe");
    ASSERT_TRUE(!player_is_safe_from_fly(PLAYER_STATE_RUNNING, BASE_SECOND, BASE_FIRST), "Advanced runner is not safe");
    ASSERT_TRUE(player_is_safe_from_fly(PLAYER_STATE_ON_BASE, BASE_FIRST, BASE_FIRST), "On original base is safe");
    ASSERT_TRUE(player_is_safe_from_fly(PLAYER_STATE_AT_BAT, BASE_HOME, BASE_HOME), "At bat on home is safe");
    ASSERT_TRUE(!player_is_safe_from_fly(PLAYER_STATE_RUNNING, BASE_FIRST, BASE_FIRST), "Running off base is not safe");

    return TEST_PASSED;
}

int test_count_active_batting_players(void)
{
    PlayerInfo players[PLAYERS_IN_TEAM + JOKER_COUNT];
    memset(players, 0, sizeof(players));

    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        players[i].bTPI.baseId = BASE_NONE; // All inactive
    }

    ASSERT_EQ(0, count_active_batting_players(players), "0 active players");

    players[0].bTPI.baseId = BASE_HOME; // At bat
    players[2].bTPI.baseId = BASE_FIRST; // Safe
    players[4].bTPI.baseId = BASE_SECOND; // Running

    ASSERT_EQ(3, count_active_batting_players(players), "3 active players");

    return TEST_PASSED;
}