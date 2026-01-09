#include "test_helpers.h"
#include "fixtures.h"

// NEW FULL-SCENARIO TESTS
#include "test_full_scenarios.h"

#include <string.h>

int tests_run = 0;
int tests_failed = 0;

int main(int argc, char* argv[]) {
	printf("========================================\n");
	printf("PNB Integration Test Suite\n");
	printf("========================================\n\n");
	
	// NEW Full-scenario tests
	run_full_scenario_tests();

	printf("\n========================================\n");
	printf("Tests run: %d\n", tests_run);
	printf("Tests failed: %d\n", tests_failed);
	printf("========================================\n");
	
	return tests_failed;
}