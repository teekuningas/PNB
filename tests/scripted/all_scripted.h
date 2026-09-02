#ifndef ALL_SCRIPTED_H
#define ALL_SCRIPTED_H

// Headless scripted-human tests (the "scripted" tier — the second of the three headless drivers).
int test_scripted_human_runs_headless(void);
int test_scripted_key_edges(void);
int test_scripted_input_reaches_pipeline(void);
int test_scripted_single_tap_does_not_run_batter(void);
int test_scripted_double_tap_runs_batter(void);
int test_scripted_pitch_two_pingpongs_strike(void);
int test_scripted_pitch_dropped_aim_is_valesyotto(void);
int test_scripted_human_pitch_ai_hit_flies(void);
int test_scripted_throw_hold_release_to_base(void);

// Movement: the human sends a destination, and the engine owns the walk.
int test_scripted_move_held_key_runs_at_run_speed(void);
int test_scripted_move_release_stops_within_a_frame(void);
int test_scripted_move_declared_during_a_pitch_resumes_after_it(void);
int test_scripted_move_held_key_survives_a_reset(void);

// Batting: the human's cursor over the legal batters, and held keys that become an aim.
int test_scripted_aim_key_walks_the_batter_and_release_stops_him(void);
int test_scripted_batter_cursor_offers_only_legal_players(void);
int test_scripted_aim_leaves_no_interpolation_gap_when_it_stops(void);

// The human swing: the meter that arms on the windup, the one that fits inside the flight, the
// withdrawal, and silence.
int test_scripted_swing_power_meter_arms_on_the_windup(void);
int test_scripted_swing_elevation_meter_fits_inside_the_flight(void);
int test_scripted_swing_can_be_withdrawn_after_the_power_is_committed(void);
int test_scripted_swing_silence_is_not_a_miss(void);

#endif /* ALL_SCRIPTED_H */
