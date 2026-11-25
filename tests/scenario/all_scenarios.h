#ifndef ALL_SCENARIOS_H
#define ALL_SCENARIOS_H

int test_runner_scores_from_third(void);
int test_batter_forced_out_at_first(void);
int test_fly_ball_runner_wounded(void);
int test_runner_chain_reaction_no_catch(void);
int test_fly_ball_double_wound(void);
int test_fly_ball_double_wound_late_arrival(void);
int test_out_of_bounds_reset(void);
int test_batter_may_leave_home_after_foul_reset(void);
int test_pitching_strike(void);
int test_pitching_ball(void);
int test_free_walk_resolution(void);
int test_free_walk_is_not_offered_to_nobody(void); // §26 — an offer names a player, or there is none
int test_free_walk_offered_to_running_batter_from_home(void); // §26+§32 — running batter off home gets 1-ball walk
int test_free_walk_skips_player_out_or_scored_on_ball_frame(void); // ENFORCE order — OUT/SCORED excluded on BALL tick
int test_run_of_honor(void); // §42 Kunniajuoksu (Run of Honor)
int test_run_arrival_before_ball_lands(void); // Regression test for "Lost Run" bug
int test_run_arrival_before_catch(void); // Pending run voided by catch
int test_fly_ball_out_and_wound(void); // Fly ball OUT + wound scenario
int test_fly_ball_early_arrival(void); // Batter arrives at base before catch
int test_burnt_player_bats_again(void); // A burnt player keeps their place in the batting order
int test_last_batter_ends_half_inning(void); // §12(3) — the side change keys on the viimeinen lyöjä
int test_scoreless_lap_of_the_order_ends_half_inning(void); // §12(2) — one lap of the order, not a pool
int test_joker_opening_does_not_take_the_turn(void); // §7/§12(2) — a joker opener is not the turn's opener
int test_batter_retreats_to_home_on_koppi(void); // Safe home retreat on koppi restores AT_BAT and readiness
int test_batter_retreat_closes_stale_seat_offer(void); // Safe home retreat closes an already-open seat offer
int test_contest_first_strike_foul_resumes_same_pair(void); // 1st-strike foul in contest restores same pair
int test_contest_third_strike_foul_advances_pair_cleanly(
    void
); // 3rd-strike foul advances pair without teleporting batter
int test_kotipesakoppi_wounds_off_base_runner(void); // §30/§36 — lukkari catches home pop-up and wounds off-base runner
int test_close_infield_koppi_within_15_units_of_lukkari(void); // §30/§36 — fielder <15u from lukkari catches fly ball

#endif // ALL_SCENARIOS_H