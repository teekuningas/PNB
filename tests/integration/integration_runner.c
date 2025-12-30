#include "test_helpers.h"
#include "fixtures.h"
#include "test_scenario_outs.h"
#include "test_scenario_runs.h"
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

int main(int argc, char* argv[]) {
	printf("========================================\n");
	printf("PNB Integration Test Suite\n");
	printf("========================================\n\n");
	
	// Integration tests
	run_scenario_outs_tests();
	run_scenario_runs_tests();

	printf("\n========================================\n");
	printf("Tests run: %d\n", tests_run);
	printf("Tests failed: %d\n", tests_failed);
	printf("========================================\n");
	
	return tests_failed;
}