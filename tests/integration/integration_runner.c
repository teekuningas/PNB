#include "test_helpers.h"
#include "fixtures.h"

// NEW FULL-SCENARIO TESTS
#include "scenarios/all_scenarios.h"

#include <string.h>

int tests_run = 0;
int tests_failed = 0;

void run_full_scenario_tests(void)
{
	printf("Running Full-Scenario Integration Tests...\n");
	RUN_TEST(test_full_runner_scores_from_third);
	RUN_TEST(test_full_batter_forced_out_at_first);
	RUN_TEST(test_full_fly_ball_runner_wounded);
	RUN_TEST(test_full_runner_chain_reaction_no_catch);
	RUN_TEST(test_full_tuplahaava_double_wound);
	RUN_TEST(test_full_tuplahaava_late_arrival_between_bases);
	RUN_TEST(test_full_out_of_bounds_reset);
	RUN_TEST(test_full_pitching_strike);
	RUN_TEST(test_full_pitching_ball);
	RUN_TEST(test_full_free_walk_resolution);
}

int main(int argc, char* argv[])
{
	printf("========================================\n");
	printf("PNB Integration Test Suite\n");
	printf("========================================\n\n");

	// NEW Full-scenario tests
	run_full_scenario_tests();

	printf("\n========================================\n");
	printf("Tests run: %d\n", tests_run);
	printf("Tests failed: %d\n", tests_failed);
	printf("========================================\n");

	return tests_failed;
}