#include "test_rules_strikes.h"
#include "test_helpers.h"
#include "../src/game/rules_pure/rules_strikes.h"

// Test: Batter should change when strikes == 3 and first base occupied
static int test_three_strikes_with_runner_on_first() {
	GameState gs = {0};
	gs.strikes = 3;
	int result = should_change_batter_on_strikes(&gs, 0);
	ASSERT_EQ(1, result, "Batter should change with 3 strikes and runner on first base");
	return TEST_PASSED;
}

// Test: Batter should not change when strikes < 3, even with runner on first
static int test_fewer_than_three_strikes_with_runner() {
	GameState gs = {0};
	gs.strikes = 0;
	ASSERT_EQ(0, should_change_batter_on_strikes(&gs, 0), "Should not change with 0 strikes");
	gs.strikes = 1;
	ASSERT_EQ(0, should_change_batter_on_strikes(&gs, 0), "Should not change with 1 strike");
	gs.strikes = 2;
	ASSERT_EQ(0, should_change_batter_on_strikes(&gs, 0), "Should not change with 2 strikes");
	return TEST_PASSED;
}

// Test: Batter should not change when strikes == 3 but first base is empty
static int test_three_strikes_without_runner_on_first() {
	GameState gs = {0};
	gs.strikes = 3;
	int result = should_change_batter_on_strikes(&gs, -1);
	ASSERT_EQ(0, result, "Batter should not change with 3 strikes but no runner on first base");
	return TEST_PASSED;
}

// Test: Batter should not change when strikes < 3 and first base is empty
static int test_fewer_than_three_strikes_without_runner() {
	GameState gs = {0};
	gs.strikes = 0;
	ASSERT_EQ(0, should_change_batter_on_strikes(&gs, -1), "Should not change with 0 strikes and no runner");
	gs.strikes = 1;
	ASSERT_EQ(0, should_change_batter_on_strikes(&gs, -1), "Should not change with 1 strike and no runner");
	gs.strikes = 2;
	ASSERT_EQ(0, should_change_batter_on_strikes(&gs, -1), "Should not change with 2 strikes and no runner");
	return TEST_PASSED;
}

// Test: Edge case with different player indices on first base
static int test_three_strikes_with_different_player_indices() {
	GameState gs = {0};
	gs.strikes = 3;
	ASSERT_EQ(1, should_change_batter_on_strikes(&gs, 0), "Should change with player index 0");
	ASSERT_EQ(1, should_change_batter_on_strikes(&gs, 1), "Should change with player index 1");
	ASSERT_EQ(1, should_change_batter_on_strikes(&gs, 5), "Should change with player index 5");
	return TEST_PASSED;
}

void run_rules_strikes_tests() {
	RUN_TEST(test_three_strikes_with_runner_on_first);
	RUN_TEST(test_fewer_than_three_strikes_with_runner);
	RUN_TEST(test_three_strikes_without_runner_on_first);
	RUN_TEST(test_fewer_than_three_strikes_without_runner);
	RUN_TEST(test_three_strikes_with_different_player_indices);
}
