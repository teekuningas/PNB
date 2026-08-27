#include "test_helpers.h"
#include "all_sims.h"

int tests_run = 0;
int tests_failed = 0;

int main(void)
{
    printf("========================================\n");
    printf("PNB Simulation Test Suite (headless AI vs AI)\n");
    printf("========================================\n\n");

    RUN_TEST(test_ai_vs_ai_half_inning);
    RUN_TEST(test_half_inning_seed_sweep);
    RUN_TEST(test_ai_vs_ai_homerun);
    RUN_TEST(test_homerun_contest_seed_sweep);
    RUN_TEST(test_ai_vs_ai_determinism);
    RUN_TEST(test_different_seeds_produce_different_games);
    RUN_TEST(test_sim_hash_matches_recorded_baseline);
    RUN_TEST(test_ai_offense_breakdown);
    RUN_TEST(test_no_batter_lock_stall);
    RUN_TEST(test_world_snapshot_retick_is_identical);
    RUN_TEST(test_retick_diverges_when_engine_seed_is_not_restored);
    RUN_TEST(test_ai_ignores_frame_events);

    printf("\n========================================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests failed: %d\n", tests_failed);
    printf("========================================\n");

    return tests_failed;
}
