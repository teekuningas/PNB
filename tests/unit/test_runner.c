#include "test_helpers.h"
#include "all_unit.h"
#include "cup.h" // Cup API used by the fixture tests below
#include "fixtures.h"
#include "menu_types.h"
#include "globals.h"
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

// Test fixture creation
int test_fixture_super_inning_setup(void)
{
    GameSetup setup;
    fixture_create_super_inning(&setup, 0, 1, 0, 1);

    ASSERT_EQ(GAME_MODE_SUPER_INNING, setup.gameMode, "Should be super inning mode");
    ASSERT_EQ(0, setup.team1, "Team 1 should be 0");
    ASSERT_EQ(1, setup.team2, "Team 2 should be 1");
    ASSERT_EQ(0, setup.team1_control, "Team 1 should be player 1");
    ASSERT_EQ(1, setup.team2_control, "Team 2 should be player 2");
    ASSERT_EQ(4, setup.halfInningsInPeriod, "Should have 4 half-innings per period (2 full innings)");

    return TEST_PASSED;
}

int test_fixture_homerun_contest_setup(void)
{
    GameSetup setup;
    fixture_create_homerun_contest(&setup, 0, 1, 0, 1);

    ASSERT_EQ(GAME_MODE_HOMERUN_CONTEST, setup.gameMode, "Should be homerun contest mode");
    ASSERT_EQ(5, setup.homerun_choice_count, "Should have 5 batter/runner pairs (first round)");

    // Check that batters and runners are set correctly
    // Batters: indices 0-4, Runners: indices 5-9
    ASSERT_EQ(0, setup.homerun_choices1[0][0], "First team batter 0");
    ASSERT_EQ(5, setup.homerun_choices1[1][0], "First team runner 5");
    ASSERT_EQ(4, setup.homerun_choices1[0][4], "First team batter 4 (last)");
    ASSERT_EQ(9, setup.homerun_choices1[1][4], "First team runner 9 (last)");

    return TEST_PASSED;
}

int test_fixture_cup_final_super_inning_setup(void)
{
    GameSetup setup;
    fixture_create_cup_final_super_inning(&setup, 0, 1, 0, 1);

    ASSERT_EQ(GAME_MODE_SUPER_INNING, setup.gameMode, "Should be super inning mode");
    ASSERT_EQ(4, setup.halfInningsInPeriod, "Should have 4 half-innings per period");

    return TEST_PASSED;
}

int test_fixture_arg_parsing(void)
{
    char* argv[] = {"program", "--fixture", "super-inning", "--team1", "2", "--team2", "3"};
    int argc = 7;
    FixtureRequest request;

    int result = fixture_parse_args(argc, argv, &request);

    ASSERT_EQ(1, result, "Should detect fixture flag");
    ASSERT_EQ(1, request.enabled, "Fixture should be enabled");
    ASSERT_STR_EQ("super-inning", request.name, "Fixture name should match");
    ASSERT_EQ(2, request.team1, "Team 1 should be 2");
    ASSERT_EQ(3, request.team2, "Team 2 should be 3");

    return TEST_PASSED;
}

int main(int argc, char* argv[])
{
    printf("========================================\n");
    printf("PNB Test Suite (No Graphics)\n");
    printf("========================================\n\n");

    // Base Logic tests (Milestone 7 helpers)
    RUN_TEST(test_base_sequence);
    RUN_TEST(test_base_properties);
    RUN_TEST(test_base_comparisons);
    RUN_TEST(test_base_to_int_index);
    RUN_TEST(test_player_is_safe_from_fly);
    RUN_TEST(test_count_active_batting_players);

    // Base Control tests
    RUN_TEST(test_get_base_controller);
    RUN_TEST(test_get_ball_at_base_index);

    // Player Utils tests
    RUN_TEST(test_get_active_batter_index);
    RUN_TEST(test_get_batting_team_index);

    // Scoring Helpers tests
    RUN_TEST(test_should_period_end_last_half_inning_lead);
    RUN_TEST(test_should_period_end_last_half_inning_no_lead);
    RUN_TEST(test_should_period_end_mid_period);
    RUN_TEST(test_should_period_end_period1_last);
    RUN_TEST(test_should_period_end_period0_tiebreaker);
    RUN_TEST(test_should_period_end_super_inning);
    RUN_TEST(test_should_period_end_homerun_contest);

    // Fixture tests
    RUN_TEST(test_fixture_super_inning_setup);
    RUN_TEST(test_fixture_homerun_contest_setup);
    RUN_TEST(test_fixture_cup_final_super_inning_setup);
    RUN_TEST(test_fixture_arg_parsing);

    // New Cup logic tests
    RUN_TEST(test_cup_creation);
    RUN_TEST(test_cup_progression);
    RUN_TEST(test_cup_best_of_three);
    RUN_TEST(test_cup_save_load);
    RUN_TEST(test_cup_day_progression);
    RUN_TEST(test_cup_day_progression_best_of_three);

    // Batting Physics tests
    RUN_TEST(test_pitch_frame_time);
    RUN_TEST(test_swing_vertical_angle_matches_the_legacy_meter_law);
    RUN_TEST(test_swing_geometry_stays_inside_its_acceptance_bands);
    RUN_TEST(test_swing_difficulty_rises_with_the_toss);
    RUN_TEST(test_batted_ball_velocity);

    // Pitching Physics tests
    RUN_TEST(test_pitch_velocity_from_aim);

    // Collision Physics tests
    RUN_TEST(test_collision_resolve_boundaries_no_collision);
    RUN_TEST(test_collision_resolve_boundaries_z_front);
    RUN_TEST(test_collision_resolve_boundaries_z_back);
    RUN_TEST(test_collision_resolve_boundaries_x_right);
    RUN_TEST(test_collision_resolve_boundaries_x_left);

    // Batting AI Strategy tests
    RUN_TEST(test_batting_strategy_decision_tree);
    RUN_TEST(test_should_change_batter);
    RUN_TEST(test_batter_seat_verdict_regular_in_turn);
    RUN_TEST(test_batter_seat_verdict_spent_order);
    RUN_TEST(test_batter_seat_verdict_jokers);
    RUN_TEST(test_choose_batter_preference);
    RUN_TEST(test_is_wrong_pitch);
    RUN_TEST(test_calculate_ai_batting_angle);

    // Catching AI Strategy tests
    RUN_TEST(test_chase_point_outside_the_dead_zone_is_the_prediction);
    RUN_TEST(test_chase_point_inside_the_dead_zone_is_a_stop);
    RUN_TEST(test_carry_to_throw_point_walks_in_from_far_away);
    RUN_TEST(test_carry_to_throw_point_backs_out_when_too_close);
    RUN_TEST(test_carry_to_throw_point_standing_on_the_base_still_yields_a_throwable_spot);
    RUN_TEST(test_move_declaration_speaks_first_time_and_on_a_real_drift);
    RUN_TEST(test_move_declaration_heartbeat_restates_an_unchanged_destination);
    RUN_TEST(test_should_ai_throw_normal);
    RUN_TEST(test_should_ai_throw_replacer);
    RUN_TEST(test_should_ai_drop_ball_scenario);

    RUN_TEST(test_determine_lead_base_simple);
    RUN_TEST(test_determine_lead_base_random);

    // Pitching AI Strategy tests
    RUN_TEST(test_decide_pitch_aim);

    // Rules Outs tests
    RUN_TEST(test_forced_out_at_first_base);
    RUN_TEST(test_safe_on_first_base);
    RUN_TEST(test_runner_at_different_base);
    RUN_TEST(test_forced_out_at_second_base);
    RUN_TEST(test_free_walk_protection);
    RUN_TEST(test_out_of_bounds_protection);

    // Rules Runs tests
    RUN_TEST(test_is_regular_run);
    RUN_TEST(test_is_run_of_honor);
    RUN_TEST(test_designate_last_batter);
    RUN_TEST(test_is_last_batter_turn_reached);

    printf("\n--- RNG stream splitting (core/rng.c) ---\n");
    RUN_TEST(test_rng_split_parent_and_child_do_not_share_a_sequence);
    RUN_TEST(test_rng_split_siblings_are_independent);
    RUN_TEST(test_rng_split_child_stream_is_uniform);

    printf("\n========================================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests failed: %d\n", tests_failed);
    printf("========================================\n");

    return tests_failed;
}
