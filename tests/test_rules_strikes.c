#include "test_rules_strikes.h"
#include "test_helpers.h"
#include "../src/game/rules_pure/rules_strikes.h"

// Test: Batter should change when strikes >= 3
static int test_three_strikes()
{
	GameState gs = {0};
	gs.strikes = 3;
	int result = should_change_batter_on_strikes(&gs);
	ASSERT_EQ(1, result, "Batter should change with 3 strikes");

	gs.strikes = 4;
	result = should_change_batter_on_strikes(&gs);
	ASSERT_EQ(1, result, "Batter should change with >3 strikes");

	return TEST_PASSED;
}

// Test: Batter should not change when strikes < 3
static int test_fewer_than_three_strikes()
{
	GameState gs = {0};
	gs.strikes = 0;
	ASSERT_EQ(0, should_change_batter_on_strikes(&gs), "Should not change with 0 strikes");
	gs.strikes = 1;
	ASSERT_EQ(0, should_change_batter_on_strikes(&gs), "Should not change with 1 strike");
	gs.strikes = 2;
	ASSERT_EQ(0, should_change_batter_on_strikes(&gs), "Should not change with 2 strikes");
	return TEST_PASSED;
}

void run_rules_strikes_tests()
{
	RUN_TEST(test_three_strikes);
	RUN_TEST(test_fewer_than_three_strikes);
}
