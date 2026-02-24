#include "test_helpers.h"
#include "base_logic.h"

int test_base_sequence()
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

int test_base_properties()
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

int test_base_comparisons()
{
    ASSERT_TRUE(base_is_at_least(BASE_SECOND, BASE_FIRST), "2nd >= 1st");
    ASSERT_TRUE(base_is_at_least(BASE_SECOND, BASE_SECOND), "2nd >= 2nd");
    ASSERT_TRUE(!base_is_at_least(BASE_FIRST, BASE_SECOND), "1st !>= 2nd");

    ASSERT_TRUE(base_cmp(BASE_THIRD, BASE_SECOND) > 0, "3rd > 2nd");
    ASSERT_TRUE(base_cmp(BASE_FIRST, BASE_SECOND) < 0, "1st < 2nd");
    ASSERT_TRUE(base_cmp(BASE_HOME, BASE_HOME) == 0, "Home == Home");

    return TEST_PASSED;
}