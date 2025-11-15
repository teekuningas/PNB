// Example tests for fixtures and menu logic
// These tests run WITHOUT graphics - no OpenGL/GLFW needed

#include "test_helpers.h"
#include "fixtures.h"
#include "menu_types.h"
#include "globals.h"
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

// Test fixture creation
int test_fixture_super_inning_setup() {
	GameSetup setup;
	fixture_create_super_inning(&setup, 0, 1, 0, 1);
	
	ASSERT_EQ(GAME_MODE_SUPER_INNING, setup.gameMode, "Should be super inning mode");
	ASSERT_EQ(0, setup.team1, "Team 1 should be 0");
	ASSERT_EQ(1, setup.team2, "Team 2 should be 1");
	ASSERT_EQ(0, setup.team1_control, "Team 1 should be player 1");
	ASSERT_EQ(1, setup.team2_control, "Team 2 should be player 2");
	ASSERT_EQ(4, setup.halfInningsInPeriod, "Should have 4 half-innings per period (2 full innings)");
	
	return TEST_PASSED;
}

int test_fixture_homerun_contest_setup() {
	GameSetup setup;
	fixture_create_homerun_contest(&setup, 0, 1, 0, 1);
	
	ASSERT_EQ(GAME_MODE_HOMERUN_CONTEST, setup.gameMode, "Should be homerun contest mode");
	ASSERT_EQ(5, setup.homerun_choice_count, "Should have 5 batter/runner pairs (first round)");
	
	// Check that batters and runners are set correctly
	// Batters: indices 0-4, Runners: indices 5-9
	ASSERT_EQ(0, setup.homerun_choices1[0][0], "First team batter 0");
	ASSERT_EQ(5, setup.homerun_choices1[1][0], "First team runner 5");
	ASSERT_EQ(4, setup.homerun_choices1[0][4], "First team batter 4 (last)");
	ASSERT_EQ(9, setup.homerun_choices1[1][4], "First team runner 9 (last)");
	
	return TEST_PASSED;
}

int test_fixture_cup_final_super_inning_setup() {
	GameSetup setup;
	fixture_create_cup_final_super_inning(&setup, 0, 1, 0, 1);

	ASSERT_EQ(GAME_MODE_SUPER_INNING, setup.gameMode, "Should be super inning mode");
	ASSERT_EQ(4, setup.halfInningsInPeriod, "Should have 4 half-innings per period");

	return TEST_PASSED;
}

int test_fixture_arg_parsing() {
	char* argv[] = {"program", "--fixture", "super-inning", "--team1", "2", "--team2", "3"};
	int argc = 7;
	FixtureRequest request;
	
	int result = fixture_parse_args(argc, argv, &request);
	
	ASSERT_EQ(1, result, "Should detect fixture flag");
	ASSERT_EQ(1, request.enabled, "Fixture should be enabled");
	ASSERT_STR_EQ("super-inning", request.name, "Fixture name should match");
	ASSERT_EQ(2, request.team1, "Team 1 should be 2");	
	ASSERT_EQ(3, request.team2, "Team 2 should be 3");
	
	return TEST_PASSED;
}

// Example test for menu helpers (we'll add more after refactoring)
int test_text_width_calculation() {
	// This would test getTextWidth2D if we extract the logic
	// For now, placeholder to show the pattern
	return TEST_PASSED;
}

int main(int argc, char* argv[]) {
	printf("========================================\n");
	printf("PNB Test Suite (No Graphics)\n");
	printf("========================================\n\n");
	
	// Fixture tests
	RUN_TEST(test_fixture_super_inning_setup);
	RUN_TEST(test_fixture_homerun_contest_setup);
	RUN_TEST(test_fixture_cup_final_super_inning_setup);
	RUN_TEST(test_fixture_arg_parsing);
	
	// Menu helper tests
	RUN_TEST(test_text_width_calculation);
	
	printf("\n========================================\n");
	printf("Tests run: %d\n", tests_run);
	printf("Tests failed: %d\n", tests_failed);
	printf("========================================\n");
	
	return tests_failed;
}
