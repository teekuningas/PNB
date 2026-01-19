#ifndef ALL_SCENARIOS_H
#define ALL_SCENARIOS_H

int test_full_runner_scores_from_third(void);
int test_full_batter_forced_out_at_first(void);
int test_full_fly_ball_runner_wounded(void);
int test_full_runner_chain_reaction_no_catch(void);
int test_full_tuplahaava_double_wound(void);
int test_full_tuplahaava_late_arrival_between_bases(void);
int test_full_out_of_bounds_reset(void);
int test_full_pitching_strike(void);
int test_full_pitching_ball(void);
int test_full_free_walk_resolution(void);
int test_full_run_of_honor(void); // §42 Kunniajuoksu (Run of Honor)
int test_run_arrival_before_ball_lands(void); // Regression test for "Lost Run" bug
int test_run_arrival_before_catch(void); // Pending run voided by catch
int test_full_fly_ball_out_and_wound(void); // New test

#endif // ALL_SCENARIOS_H