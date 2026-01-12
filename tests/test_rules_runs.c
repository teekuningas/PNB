#include "test_rules_runs.h"
#include "test_helpers.h"
#include "../src/game/rules_pure/rules_runs.h"

// Parameters for is_regular_run:
// int is_regular_run(BaseID player_base, BaseID player_original_base, int player_is_wounded);

// Parameters for is_run_of_honor:
// int is_run_of_honor(BaseID player_base, BaseID player_original_base, int player_is_wounded, int has_made_run_on_third_base);

static int test_is_regular_run()
{
	// Basic run: Player arrives at home base (BASE_HOME_SCORED), not wounded
	ASSERT_EQ(1, is_regular_run(BASE_HOME_SCORED, BASE_FIRST, 0), "Should score run when arriving at home base");
	ASSERT_EQ(1, is_regular_run(BASE_HOME_SCORED, BASE_THIRD, 0), "Should score run when arriving at home base from 3rd");

	// Wounded player
	ASSERT_EQ(0, is_regular_run(BASE_HOME_SCORED, BASE_THIRD, 1), "Wounded player should not score regular run");

	// Not at scoring base
	ASSERT_EQ(0, is_regular_run(BASE_THIRD, BASE_SECOND, 0), "Arriving at 3rd base is not a regular run");

	return TEST_PASSED;
}

static int test_is_run_of_honor()
{
	// Run of Honor: Batter (baseAtPitchStart BASE_HOME) arrives at 3rd base (BASE_THIRD)
	ASSERT_EQ(1, is_run_of_honor(BASE_THIRD, BASE_HOME, 0, 0), "Should score run of honor");

	// Wounded
	ASSERT_EQ(0, is_run_of_honor(BASE_THIRD, BASE_HOME, 1, 0), "Wounded player should not score run of honor");

	// Not original batter (started at 1st or 2nd)
	ASSERT_EQ(0, is_run_of_honor(BASE_THIRD, BASE_FIRST, 0, 0), "Player starting from 1st base cannot score run of honor");
	ASSERT_EQ(0, is_run_of_honor(BASE_THIRD, BASE_SECOND, 0, 0), "Player starting from 2nd base cannot score run of honor");

	// Already made run of honor
	ASSERT_EQ(0, is_run_of_honor(BASE_THIRD, BASE_HOME, 0, 1), "Cannot score run of honor if already made on this base visit");

	// Not at 3rd base
	ASSERT_EQ(0, is_run_of_honor(BASE_SECOND, BASE_HOME, 0, 0), "Cannot score run of honor at 2nd base");

	return TEST_PASSED;
}

void run_rules_runs_tests()
{
	RUN_TEST(test_is_regular_run);
	RUN_TEST(test_is_run_of_honor);
}
