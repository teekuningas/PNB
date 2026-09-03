#include "test_helpers.h"
#include "all_scripted.h"

int tests_run = 0;
int tests_failed = 0;

int main(void)
{
    printf("========================================\n");
    printf("PNB Scripted-Human Test Suite (headless, real input path)\n");
    printf("========================================\n\n");

    RUN_TEST(test_scripted_human_runs_headless);
    RUN_TEST(test_scripted_key_edges);
    RUN_TEST(test_scripted_input_reaches_pipeline);
    RUN_TEST(test_scripted_single_tap_does_not_run_batter);
    RUN_TEST(test_scripted_double_tap_runs_batter);
    RUN_TEST(test_scripted_pitch_two_pingpongs_strike);
    RUN_TEST(test_scripted_pitch_dropped_aim_is_valesyotto);
    RUN_TEST(test_scripted_human_pitch_ai_hit_flies);
    RUN_TEST(test_scripted_throw_hold_release_to_base);
    RUN_TEST(test_scripted_move_held_key_runs_at_run_speed);
    RUN_TEST(test_scripted_move_release_stops_within_a_frame);
    RUN_TEST(test_scripted_move_declared_during_a_pitch_resumes_after_it);
    RUN_TEST(test_scripted_move_held_key_survives_a_reset);
    RUN_TEST(test_scripted_aim_key_walks_the_batter_and_release_stops_him);
    RUN_TEST(test_scripted_swing_power_meter_arms_on_the_windup);
    RUN_TEST(test_scripted_swing_elevation_meter_fits_inside_the_flight);
    RUN_TEST(test_scripted_swing_can_be_withdrawn_after_the_power_is_committed);
    RUN_TEST(test_scripted_swing_silence_is_not_a_miss);
    RUN_TEST(test_scripted_swing_gesture_survives_a_declaration_still_in_flight);
    RUN_TEST(test_scripted_swing_power_beat_survives_into_later_pitches);
    RUN_TEST(test_scripted_swing_elevation_marker_always_starts_at_the_extreme);
    RUN_TEST(test_scripted_swing_a_batter_who_declares_nothing_does_not_swing);
    RUN_TEST(test_scripted_swing_committed_power_stays_on_the_bar_until_the_ball_is_up);
    RUN_TEST(test_scripted_swing_withdrawal_reshapes_the_body_mid_motion);
    RUN_TEST(test_scripted_batter_cursor_offers_only_legal_players);
    RUN_TEST(test_scripted_aim_leaves_no_interpolation_gap_when_it_stops);
    RUN_TEST(test_scripted_h2h_four_beats_on_one_pitch);
    RUN_TEST(test_scripted_h2h_both_pads_may_declare_on_one_frame);
    RUN_TEST(test_scripted_h2h_one_pads_key_never_moves_the_others_widget);
    RUN_TEST(test_scripted_h2h_the_dance_repeats_across_pitches);

    printf("\n========================================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests failed: %d\n", tests_failed);
    printf("========================================\n");

    return tests_failed;
}
