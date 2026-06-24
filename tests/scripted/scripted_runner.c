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

    printf("\n========================================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests failed: %d\n", tests_failed);
    printf("========================================\n");

    return tests_failed;
}
