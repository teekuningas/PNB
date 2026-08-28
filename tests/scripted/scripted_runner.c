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
    RUN_TEST(test_scripted_batter_cursor_offers_only_legal_players);
    RUN_TEST(test_scripted_aim_leaves_no_interpolation_gap_when_it_stops);

    printf("\n========================================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests failed: %d\n", tests_failed);
    printf("========================================\n");

    return tests_failed;
}
