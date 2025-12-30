#include "test_helpers.h"
#include "integration/fixtures.h"
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

// Integration test declarations
extern int test_scenario_runner_forced_out();
extern int test_scenario_run_scored();
extern int test_scenario_wounded();


int main(int argc, char* argv[]) {
	printf("========================================\n");
	printf("PNB Integration Test Suite\n");
	printf("========================================\n\n");
	
	// Integration tests
	RUN_TEST(test_scenario_runner_forced_out);
	RUN_TEST(test_scenario_run_scored);
	RUN_TEST(test_scenario_wounded);

	printf("\n========================================\n");
	printf("Tests run: %d\n", tests_run);
	printf("Tests failed: %d\n", tests_failed);
	printf("========================================\n");
	
	return tests_failed;
}