#include "test_rules_runs.h"
#include "test_helpers.h"
#include "../src/game/rules_pure/rules_runs.h"

// Parameters for calculate_runs:
// int calculate_runs(int player_base, int player_original_base, int player_is_wounded, int can_make_run_of_honor, int has_made_run_on_third_base);

static int test_normal_run_home_base() {
    // Basic run: Player arrives at home base (4), not wounded
    ASSERT_EQ(1, calculate_runs(4, 1, 0, 0, 0), "Should score run when arriving at home base");
    ASSERT_EQ(1, calculate_runs(4, 3, 0, 1, 0), "Should score run when arriving at home base (even if run of honor possible)");
    return TEST_PASSED;
}

static int test_run_of_honor() {
    // Run of Honor: Batter (originalBase 0) arrives at 3rd base (3), allowed to make run of honor
    ASSERT_EQ(1, calculate_runs(3, 0, 0, 1, 0), "Should score run of honor");
    return TEST_PASSED;
}

static int test_no_run_wounded() {
    // Wounded players cannot score
    ASSERT_EQ(0, calculate_runs(4, 1, 1, 0, 0), "Wounded player at home base should not score");
    ASSERT_EQ(0, calculate_runs(3, 0, 1, 1, 0), "Wounded player at 3rd base should not score run of honor");
    return TEST_PASSED;
}

static int test_no_run_of_honor_conditions() {
    // Not original batter (originalBase != 0)
    ASSERT_EQ(0, calculate_runs(3, 1, 0, 1, 0), "Player starting from 1st base cannot score run of honor");
    ASSERT_EQ(0, calculate_runs(3, 2, 0, 1, 0), "Player starting from 2nd base cannot score run of honor");

    // Run of Honor flag not set
    ASSERT_EQ(0, calculate_runs(3, 0, 0, 0, 0), "Cannot score run of honor if can_make_run_of_honor is false");

    // Already made run of honor
    ASSERT_EQ(0, calculate_runs(3, 0, 0, 1, 1), "Cannot score run of honor if already made on this base visit");
    
    return TEST_PASSED;
}

static int test_other_bases() {
    // Arriving at 1st or 2nd base never scores a run
    ASSERT_EQ(0, calculate_runs(1, 0, 0, 1, 0), "Arriving at 1st base does not score");
    ASSERT_EQ(0, calculate_runs(2, 0, 0, 1, 0), "Arriving at 2nd base does not score");
    // Arriving at 3rd base but not run of honor conditions
    ASSERT_EQ(0, calculate_runs(3, 2, 0, 0, 0), "Arriving at 3rd base normally does not score");
    return TEST_PASSED;
}

void run_rules_runs_tests() {
    RUN_TEST(test_normal_run_home_base);
    RUN_TEST(test_run_of_honor);
    RUN_TEST(test_no_run_wounded);
    RUN_TEST(test_no_run_of_honor_conditions);
    RUN_TEST(test_other_bases);
}