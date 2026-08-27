#ifndef ALL_UNIT_H
#define ALL_UNIT_H

// The unit tier's single aggregator — every unit test in one list, mirroring the other tiers'
// all_scenarios.h / all_scripted.h. A test is defined in its suite file (or, for the fixture
// tests, in test_runner.c itself) and run from test_runner.c's main.

// Base logic (rules_pure/base_logic.c — base algebra + rule predicates)
int test_base_sequence(void);
int test_base_properties(void);
int test_base_comparisons(void);
int test_base_to_int_index(void);
int test_player_is_safe_from_fly(void);
int test_count_active_batting_players(void);

// Base control (rules_pure/base_control.c)
int test_get_base_controller(void);
int test_get_ball_at_base_index(void);

// Player utils (rules_pure/player_utils.c)
int test_get_active_batter_index(void);
int test_get_batting_team_index(void);

// Scoring helpers (rules_pure/scoring_helpers.c — §8/§12 period cadence)
int test_should_period_end_last_half_inning_lead(void);
int test_should_period_end_last_half_inning_no_lead(void);
int test_should_period_end_mid_period(void);
int test_should_period_end_period1_last(void);
int test_should_period_end_period0_tiebreaker(void);
int test_should_period_end_super_inning(void);
int test_should_period_end_homerun_contest(void);

// Fixtures (core/fixtures.c; defined in test_runner.c)
int test_fixture_super_inning_setup(void);
int test_fixture_homerun_contest_setup(void);
int test_fixture_cup_final_super_inning_setup(void);
int test_fixture_arg_parsing(void);

// Cup logic (cup/cup.c)
int test_cup_creation(void);
int test_cup_progression(void);
int test_cup_best_of_three(void);
int test_cup_save_load(void);
int test_cup_day_progression(void);
int test_cup_day_progression_best_of_three(void);

// Batting physics (actions_pure/batting_physics.c)
int test_pitch_frame_time(void);
int test_meter_values(void);
int test_batting_vertical_angle(void);
int test_batted_ball_velocity(void);

// Pitching physics (actions_pure/pitching_physics.c)
int test_pitch_velocity_from_aim(void);

// Collision physics (physics/collision.c)
int test_collision_resolve_boundaries_no_collision(void);
int test_collision_resolve_boundaries_z_front(void);
int test_collision_resolve_boundaries_z_back(void);
int test_collision_resolve_boundaries_x_right(void);
int test_collision_resolve_boundaries_x_left(void);

// Batting AI strategy (ai_pure/batting_ai_strategy.c)
int test_batting_strategy_decision_tree(void);
int test_should_change_batter(void);
int test_is_wrong_pitch(void);
int test_calculate_ai_batting_angle(void);

// Catching AI strategy (ai_pure/catching_ai_strategy.c)
int test_chase_point_outside_the_dead_zone_is_the_prediction(void);
int test_chase_point_inside_the_dead_zone_is_a_stop(void);
int test_carry_to_throw_point_walks_in_from_far_away(void);
int test_carry_to_throw_point_backs_out_when_too_close(void);
int test_carry_to_throw_point_standing_on_the_base_still_yields_a_throwable_spot(void);
int test_move_declaration_speaks_first_time_and_on_a_real_drift(void);
int test_move_declaration_heartbeat_restates_an_unchanged_destination(void);
int test_should_ai_throw_normal(void);
int test_should_ai_throw_replacer(void);
int test_should_ai_drop_ball_scenario(void);
int test_determine_lead_base_simple(void);
int test_determine_lead_base_random(void);

// Pitching AI strategy (ai_pure/pitching_ai_strategy.c)
int test_decide_pitch_aim(void);

// Rules: outs (rules_pure/rules_outs.c — §33)
int test_forced_out_at_first_base(void);
int test_safe_on_first_base(void);
int test_runner_at_different_base(void);
int test_forced_out_at_second_base(void);
int test_free_walk_protection(void);
int test_out_of_bounds_protection(void);

// Rules: runs (rules_pure/rules_runs.c — §41/§42)
int test_is_regular_run(void);
int test_is_run_of_honor(void);

// Rules: the §12 side change (rules_pure/rules_side_change.c)
int test_designate_last_batter(void);
int test_is_last_batter_turn_reached(void);

// RNG stream splitting (core/rng.c)
int test_rng_split_parent_and_child_do_not_share_a_sequence(void);
int test_rng_split_siblings_are_independent(void);
int test_rng_split_child_stream_is_uniform(void);

#endif /* ALL_UNIT_H */
