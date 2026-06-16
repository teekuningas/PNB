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
    RUN_TEST(test_ai_vs_ai_homerun);
    RUN_TEST(test_ai_vs_ai_determinism);
    RUN_TEST(test_ai_offense_breakdown);

    printf("\n========================================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests failed: %d\n", tests_failed);
    printf("========================================\n");

    return tests_failed;
}
