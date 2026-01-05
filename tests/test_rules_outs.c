#include "test_rules_outs.h"
#include "test_helpers.h"
#include "../src/game/rules_pure/rules_outs.h"

int test_forced_out_at_first_base() {
    // Runner running to 1st base, ball at 1st base -> OUT
    GameState gs = {0};
    int result = is_runner_forced_out(BASE_HOME, 0, BASE_HOME, 0, &gs);
    ASSERT_EQ(1, result, "Runner running to 1st base should be out if ball is there");
    return TEST_PASSED;
}

int test_safe_on_first_base() {
    // Runner safe on 1st base, ball at 1st base -> SAFE
    GameState gs = {0};
    int result = is_runner_forced_out(BASE_HOME, 1, BASE_HOME, 0, &gs);
    ASSERT_EQ(0, result, "Runner safe on 1st base should not be out");
    return TEST_PASSED;
}

int test_runner_at_different_base() {
    // Runner running to 2nd base, ball at 1st base -> SAFE
    GameState gs = {0};
    int result = is_runner_forced_out(BASE_FIRST, 0, BASE_HOME, 0, &gs);
    ASSERT_EQ(0, result, "Runner running to 2nd base should not be out if ball is at 1st");
    return TEST_PASSED;
}

int test_forced_out_at_second_base() {
    // Runner running to 2nd base, ball at 2nd base -> OUT
    GameState gs = {0};
    int result = is_runner_forced_out(BASE_FIRST, 0, BASE_FIRST, 0, &gs);
    ASSERT_EQ(1, result, "Runner running to 2nd base should be out if ball is there");
    return TEST_PASSED;
}

int test_free_walk_protection() {
    // Runner running to 1st base, ball at 1st base, taking free walk -> SAFE
    GameState gs = {0};
    int result = is_runner_forced_out(BASE_HOME, 0, BASE_HOME, 1, &gs);
    ASSERT_EQ(0, result, "Runner taking free walk should be safe");
    return TEST_PASSED;
}

int test_out_of_bounds_protection() {
    // Runner running to 1st base, ball at 1st base, ball out of bounds -> SAFE
    GameState gs = {0};
    gs.outOfBounds = 1;
    int result = is_runner_forced_out(BASE_HOME, 0, BASE_HOME, 0, &gs);
    ASSERT_EQ(0, result, "Runner should be safe if ball is out of bounds");
    return TEST_PASSED;
}

int test_regression_runner_from_base_zero() {
    // Regression test: Runner at base 0, running to base 1, ball at base 1 -> OUT
    // Previously failed due to incorrect parameter passing (used loop index i instead of baseIndex)
    // When ball is at base 1 (i=1), we check with baseIndex=BASE_HOME (0)
    GameState gs = {0};
    int result = is_runner_forced_out(BASE_HOME, 0, BASE_HOME, 0, &gs);
    ASSERT_EQ(1, result, "Runner from base 0 running to base 1 should be out if ball is at base 1");
    return TEST_PASSED;
}

void run_rules_outs_tests() {
    RUN_TEST(test_forced_out_at_first_base);
    RUN_TEST(test_safe_on_first_base);
    RUN_TEST(test_runner_at_different_base);
    RUN_TEST(test_forced_out_at_second_base);
    RUN_TEST(test_free_walk_protection);
    RUN_TEST(test_out_of_bounds_protection);
    RUN_TEST(test_regression_runner_from_base_zero);
}