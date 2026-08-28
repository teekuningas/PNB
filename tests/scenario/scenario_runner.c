#include "test_helpers.h"
#include "fixtures.h"
#include "all_scenarios.h"

#include <string.h>

int tests_run = 0;
int tests_failed = 0;

static void run_scenario_tests(void)
{
    printf("Running Scenario Tests (Full-Game Simulations)...\n");
    RUN_TEST(test_runner_scores_from_third);
    RUN_TEST(test_batter_forced_out_at_first);
    RUN_TEST(test_fly_ball_runner_wounded);
    RUN_TEST(test_runner_chain_reaction_no_catch);
    RUN_TEST(test_fly_ball_double_wound);
    RUN_TEST(test_fly_ball_double_wound_late_arrival);
    RUN_TEST(test_out_of_bounds_reset);
    RUN_TEST(test_pitching_strike);
    RUN_TEST(test_pitching_ball);
    RUN_TEST(test_free_walk_resolution);
    RUN_TEST(test_free_walk_is_not_offered_to_nobody);
    RUN_TEST(test_run_of_honor);
    RUN_TEST(test_run_arrival_before_ball_lands);
    RUN_TEST(test_run_arrival_before_catch);
    RUN_TEST(test_fly_ball_out_and_wound);
    RUN_TEST(test_fly_ball_early_arrival);
    RUN_TEST(test_burnt_player_bats_again);
    RUN_TEST(test_last_batter_ends_half_inning);
    RUN_TEST(test_scoreless_lap_of_the_order_ends_half_inning);
    RUN_TEST(test_joker_opening_does_not_take_the_turn);
}

int main(int argc, char* argv[])
{
    printf("========================================\n");
    printf("PNB Scenario Test Suite\n");
    printf("========================================\n\n");

    run_scenario_tests();

    printf("\n========================================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests failed: %d\n", tests_failed);
    printf("========================================\n");

    return tests_failed;
}
