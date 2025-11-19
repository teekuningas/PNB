#include "test_helpers.h"
#include "cup.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

// Helper to check if a value is a power of two
static int is_power_of_two(int n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

// Test the creation and initial state of a cup
int test_cup_creation() {
    printf("Running test: %s\n", __func__);
    int initial_teams[] = {0, 1, 2, 3, 4, 5, 6, 7};
    int num_teams = 8;
    Cup* cup = cup_create(num_teams, 1, 0, 4, initial_teams);

    ASSERT_NOT_NULL(cup, "Cup should not be null");
    ASSERT_EQ(num_teams, cup->num_teams, "Number of teams should be set correctly");
    ASSERT_EQ(1, cup->wins_to_advance, "Wins to advance should be set correctly");
    ASSERT_EQ(0, cup->user_team_id, "User team ID should be set correctly");
    ASSERT_TRUE(is_power_of_two(cup->num_teams), "Number of teams should be a power of two");
    ASSERT_EQ(num_teams - 1, cup->num_matches, "Number of matches should be num_teams - 1");

    // Check initial pairings in the first round (quarter-finals for 8 teams)
    // First round matches are at the end of the array
    int first_round_start_index = num_teams - 1 - (num_teams / 2);
    ASSERT_EQ(0, cup->matches[first_round_start_index + 0].team_a_id, "QF 1 Team A");
    ASSERT_EQ(1, cup->matches[first_round_start_index + 0].team_b_id, "QF 1 Team B");
    ASSERT_EQ(6, cup->matches[first_round_start_index + 3].team_a_id, "QF 4 Team A");
    ASSERT_EQ(7, cup->matches[first_round_start_index + 3].team_b_id, "QF 4 Team B");

    // Check that later-round matches are unpopulated
    ASSERT_EQ(-1, cup->matches[0].team_a_id, "Final should not have teams yet");
    ASSERT_EQ(-1, cup->matches[0].team_b_id, "Final should not have teams yet");

    cup_destroy(cup);
    return TEST_PASSED;
}

// Test the progression of winners through the tournament tree
int test_cup_progression() {
    printf("Running test: %s\n", __func__);
    int initial_teams[] = {0, 1, 2, 3, 4, 5, 6, 7};
    int num_teams = 8;
    Cup* cup = cup_create(num_teams, 1, 0, 4, initial_teams);

    // --- Simulate Quarter-Finals ---
    // Winners: 1, 3, 5, 7
    cup_update_match_result(cup, 3, 1); // Match 3 (0 vs 1), winner is 1
    cup_update_match_result(cup, 4, 3); // Match 4 (2 vs 3), winner is 3
    cup_update_match_result(cup, 5, 5); // Match 5 (4 vs 5), winner is 5
    cup_update_match_result(cup, 6, 7); // Match 6 (6 vs 7), winner is 7

    // Assert that winners have been promoted to the semi-finals (matches 1 and 2)
    ASSERT_EQ(1, cup->matches[1].team_a_id, "SF 1 Team A should be winner of QF 1");
    ASSERT_EQ(3, cup->matches[1].team_b_id, "SF 1 Team B should be winner of QF 2");
    ASSERT_EQ(5, cup->matches[2].team_a_id, "SF 2 Team A should be winner of QF 3");
    ASSERT_EQ(7, cup->matches[2].team_b_id, "SF 2 Team B should be winner of QF 4");

    // --- Simulate Semi-Finals ---
    // Winners: 3, 7
    cup_update_match_result(cup, 1, 3); // Match 1 (1 vs 3), winner is 3
    cup_update_match_result(cup, 2, 7); // Match 2 (5 vs 7), winner is 7

    // Assert that winners have been promoted to the final (match 0)
    ASSERT_EQ(3, cup->matches[0].team_a_id, "Final Team A should be winner of SF 1");
    ASSERT_EQ(7, cup->matches[0].team_b_id, "Final Team B should be winner of SF 2");

    // --- Simulate Final ---
    // Winner: 7
    cup_update_match_result(cup, 0, 7);

    // Assert that the final match has the correct winner
    ASSERT_EQ(7, cup->matches[0].winner_id, "Final winner should be team 7");

    cup_destroy(cup);
    return TEST_PASSED;
}

// Test "best of N" win condition
int test_cup_best_of_three() {
    printf("Running test: %s\n", __func__);
    int initial_teams[] = {0, 1, 2, 3};
    int num_teams = 4;
    // Best of 3 means 2 wins are needed
    Cup* cup = cup_create(num_teams, 2, 0, 4, initial_teams); 

    // --- Simulate Semi-Finals (Matches 1 and 2 for 4 teams) ---
    
    // Match 1 (0 vs 1): Team 1 wins the first game
    cup_update_match_result(cup, 1, 1);
    ASSERT_EQ(1, cup->matches[1].wins_b, "Bo3: Team 1 should have 1 win");
    ASSERT_EQ(-1, cup->matches[0].team_a_id, "Bo3: No team should advance after one game");

    // Match 1 (0 vs 1): Team 0 wins the second game
    cup_update_match_result(cup, 1, 0);
    ASSERT_EQ(1, cup->matches[1].wins_a, "Bo3: Team 0 should have 1 win");
    ASSERT_EQ(1, cup->matches[1].wins_b, "Bo3: Team 1 should still have 1 win");
    ASSERT_EQ(-1, cup->matches[0].team_a_id, "Bo3: No team should advance when tied 1-1");

    // Match 1 (0 vs 1): Team 1 wins the third game and the match
    cup_update_match_result(cup, 1, 1);
    ASSERT_EQ(2, cup->matches[1].wins_b, "Bo3: Team 1 should have 2 wins");
    ASSERT_EQ(1, cup->matches[1].winner_id, "Bo3: Team 1 should be the match winner");
    ASSERT_EQ(1, cup->matches[0].team_a_id, "Bo3: Team 1 should advance to the final");

    cup_destroy(cup);
    return TEST_PASSED;
}

// Test save and load functionality with round-trip verification
int test_cup_save_load() {
    printf("Running test: %s\n", __func__);
    const char* test_filename = "test_cup_save.xml";
    
    // Create a cup with some progress
    int initial_teams[] = {0, 1, 2, 3, 4, 5, 6, 7};
    int num_teams = 8;
    Cup* original_cup = cup_create(num_teams, 2, 3, 4, initial_teams);
    ASSERT_NOT_NULL(original_cup, "Original cup should be created");
    
    // Simulate some matches to create interesting state
    // Quarter-finals with best-of-3 (need 2 wins each to advance)
    // QF1: Team 1 beats Team 0 (2-0)
    cup_update_match_result(original_cup, 3, 1);
    cup_update_match_result(original_cup, 3, 1);
    // QF2: Team 2 beats Team 3 (2-1)
    cup_update_match_result(original_cup, 4, 2);
    cup_update_match_result(original_cup, 4, 3);
    cup_update_match_result(original_cup, 4, 2);
    // QF3: Team 5 beats Team 4 (2-0)
    cup_update_match_result(original_cup, 5, 5);
    cup_update_match_result(original_cup, 5, 5);
    // QF4: Team 6 beats Team 7 (2-0)
    cup_update_match_result(original_cup, 6, 6);
    cup_update_match_result(original_cup, 6, 6);
    
    // Semi-finals: partially played (match 1 has one game, 1-0)
    // Now teams 1, 2, 5, 6 have advanced to semis
    cup_update_match_result(original_cup, 1, 1);
    
    // Save the cup
    int save_result = cup_save(original_cup, test_filename);
    ASSERT_EQ(0, save_result, "Save should succeed");
    
    // Load the cup
    Cup* loaded_cup = cup_load(test_filename);
    ASSERT_NOT_NULL(loaded_cup, "Loaded cup should not be null");
    
    // Verify all attributes match
    ASSERT_EQ(original_cup->num_teams, loaded_cup->num_teams, "Loaded: num_teams should match");
    ASSERT_EQ(original_cup->wins_to_advance, loaded_cup->wins_to_advance, "Loaded: wins_to_advance should match");
    ASSERT_EQ(original_cup->user_team_id, loaded_cup->user_team_id, "Loaded: user_team_id should match");
    ASSERT_EQ(original_cup->num_rounds, loaded_cup->num_rounds, "Loaded: num_rounds should match");
    ASSERT_EQ(original_cup->num_matches, loaded_cup->num_matches, "Loaded: num_matches should match");
    
    // Verify all match states
    for (int i = 0; i < original_cup->num_matches; i++) {
        char msg[100];
        sprintf(msg, "Match %d: team_a_id should match", i);
        ASSERT_EQ(original_cup->matches[i].team_a_id, loaded_cup->matches[i].team_a_id, msg);
        
        sprintf(msg, "Match %d: team_b_id should match", i);
        ASSERT_EQ(original_cup->matches[i].team_b_id, loaded_cup->matches[i].team_b_id, msg);
        
        sprintf(msg, "Match %d: wins_a should match", i);
        ASSERT_EQ(original_cup->matches[i].wins_a, loaded_cup->matches[i].wins_a, msg);
        
        sprintf(msg, "Match %d: wins_b should match", i);
        ASSERT_EQ(original_cup->matches[i].wins_b, loaded_cup->matches[i].wins_b, msg);
        
        sprintf(msg, "Match %d: winner_id should match", i);
        ASSERT_EQ(original_cup->matches[i].winner_id, loaded_cup->matches[i].winner_id, msg);
    }
    
    // Verify specific match states we set up
    ASSERT_EQ(1, loaded_cup->matches[3].winner_id, "QF1: winner should be team 1");
    ASSERT_EQ(2, loaded_cup->matches[4].winner_id, "QF2: winner should be team 2");
    ASSERT_EQ(1, loaded_cup->matches[1].wins_a, "SF1: team 1 should have 1 win");
    ASSERT_EQ(0, loaded_cup->matches[1].wins_b, "SF1: team 2 should have 0 wins");
    ASSERT_EQ(-1, loaded_cup->matches[1].winner_id, "SF1: should not have a winner yet");
    
    // Continue playing on loaded cup to verify functionality
    cup_update_match_result(loaded_cup, 1, 1); // Second game in semi-final
    ASSERT_EQ(2, loaded_cup->matches[1].wins_a, "SF1: team 1 should now have 2 wins");
    ASSERT_EQ(1, loaded_cup->matches[1].winner_id, "SF1: team 1 should be the winner");
    ASSERT_EQ(1, loaded_cup->matches[0].team_a_id, "Final: team 1 should advance");
    
    // Clean up
    cup_destroy(original_cup);
    cup_destroy(loaded_cup);
    remove(test_filename);
    
    return TEST_PASSED;
}