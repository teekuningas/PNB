#include "test_helpers.h"
#include "fixtures.h"
#include "contracts/all_contracts.h"

#include <string.h>

int tests_run = 0;
int tests_failed = 0;

static void run_contract_tests(void)
{
    printf("Running Contract Tests (1-Frame Pipeline Proofs)...\n");

    // Lifecycle contracts
    RUN_TEST(test_clear_frame_events_completeness);

    // Referee reaction contracts
    RUN_TEST(test_referee_starts_wounding_on_catch);
    RUN_TEST(test_referee_snapshots_on_pitch_released);

    // Foul detection contracts
    RUN_TEST(test_foul_detected_on_out_of_bounds_hit);
}

int main(int argc, char* argv[])
{
    printf("========================================\n");
    printf("PNB Contract Test Suite\n");
    printf("========================================\n\n");

    run_contract_tests();

    printf("\n========================================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests failed: %d\n", tests_failed);
    printf("========================================\n");

    return tests_failed;
}
