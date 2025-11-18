#include "test_helpers.h"
#include "globals.h"
#include "cup.h"
#include "fixtures.h"
#include "batting_order_menu.h"
#include "menu_types.h"
#include <string.h>

// Test-specific helper to create a basic, valid StateInfo object
static void setup_test_state_info(StateInfo* stateInfo, TournamentState* ts, TeamData* teamData, int numTeams) {
	memset(stateInfo, 0, sizeof(StateInfo));
	stateInfo->tournamentState = ts;
	stateInfo->teamData = teamData;
	stateInfo->numTeams = numTeams;

	// Create mock team data with identical stats for predictable simulations
	for (int i = 0; i < numTeams; i++) {
		teamData[i].name = "Team";
		// The test requires players array to be allocated.
		teamData[i].players = malloc(sizeof(PlayerData) * (PLAYERS_IN_TEAM + JOKER_COUNT));
		for (int j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
			teamData[i].players[j].speed = 50;
			teamData[i].players[j].power = 50;
		}
	}
}

// Test-specific helper to free mock data
static void teardown_test_state_info(StateInfo* stateInfo, int numTeams) {
	for (int i = 0; i < numTeams; i++) {
		free(stateInfo->teamData[i].players);
	}
}

// A "glass-box" test to prove understanding of the simulation logic.
// We manually create a state, predict the outcome of one step, and assert it.
int test_deterministic_simulation_step() {
	// 1. Manual Setup - No randomness
	TournamentState ts;
	StateInfo si;
	TeamData teamData[8];

	memset(&ts, 0, sizeof(TournamentState));
	setup_test_state_info(&si, &ts, teamData, 8);

	// Use shared fixture helper for tournament initialization
	fixture_init_cup_state(&ts, 3, 0, -1, -1, -1); // Best of 5, day 0, no human player

	// Manually create the tournament tree
	for (int i = 0; i < 8; i++) {
		ts.cupInfo.cupTeamIndexTree[i] = i; // Team 0 in slot 0, Team 1 in slot 1, etc.
	}
	
	updateSchedule(&ts, &si); // Generate schedule from the clean state

	// 2. Assert Initial State
	ASSERT_EQ(0, ts.cupInfo.cupTeamIndexTree[0], "Initial team in slot 0 should be 0");
	ASSERT_EQ(1, ts.cupInfo.cupTeamIndexTree[1], "Initial team in slot 1 should be 1");
	ASSERT_EQ(0, ts.cupInfo.schedule[0][0], "Day 0, Match 0 should be slot 0");
	ASSERT_EQ(1, ts.cupInfo.schedule[0][1], "Day 0, Match 0 should be slot 1");
	ASSERT_EQ(6, ts.cupInfo.schedule[3][0], "Day 0, Match 3 should be slot 6");
	ASSERT_EQ(7, ts.cupInfo.schedule[3][1], "Day 0, Match 3 should be slot 7");
	ASSERT_EQ(0, ts.cupInfo.slotWins[0], "Initial wins for slot 0 should be 0");
	ASSERT_EQ(0, ts.cupInfo.slotWins[1], "Initial wins for slot 1 should be 0");

	// 3. Simulate One Step
	// The win condition is `if(randomNumber + difference*3 >= 50)`.
	// Our mock teams have equal stats, so `difference` is 0.
	// We pass 75 as the randomNumber, so the condition is always true,
	// meaning the second team in each scheduled pair will win.
	updateCupTreeAfterDay(&ts, &si, -1, 0, 75);

	// 4. Assert Next State
	ASSERT_EQ(0, ts.cupInfo.slotWins[0], "Slot 0 (loser) should have 0 wins");
	ASSERT_EQ(1, ts.cupInfo.slotWins[1], "Slot 1 (winner) should have 1 win");
	ASSERT_EQ(0, ts.cupInfo.slotWins[2], "Slot 2 (loser) should have 0 wins");
	ASSERT_EQ(1, ts.cupInfo.slotWins[3], "Slot 3 (winner) should have 1 win");
	ASSERT_EQ(0, ts.cupInfo.slotWins[4], "Slot 4 (loser) should have 0 wins");
	ASSERT_EQ(1, ts.cupInfo.slotWins[5], "Slot 5 (winner) should have 1 win");
	ASSERT_EQ(0, ts.cupInfo.slotWins[6], "Slot 6 (loser) should have 0 wins");
	ASSERT_EQ(1, ts.cupInfo.slotWins[7], "Slot 7 (winner) should have 1 win");
	ASSERT_EQ(-1, ts.cupInfo.cupTeamIndexTree[8], "No team should have advanced to slot 8 yet");

	teardown_test_state_info(&si, 8);
	return TEST_PASSED;
}

int test_best_of_one_full_simulation() {
	// This test simulates a full "Best of 1" tournament, asserting the state
	// after each round to prove full understanding of the logic.

	// 1. Manual Setup
	TournamentState ts;
	StateInfo si;
	TeamData teamData[8];

	memset(&ts, 0, sizeof(TournamentState));
	setup_test_state_info(&si, &ts, teamData, 8);

	// Use shared fixture helper for tournament initialization
	fixture_init_cup_state(&ts, 1, 0, -1, -1, -1); // Best of 1, day 0, no human player

	for (int i = 0; i < 8; i++) ts.cupInfo.cupTeamIndexTree[i] = i;
	
	updateSchedule(&ts, &si);

	// --- ROUND 1: QUARTERFINALS ---
	ts.cupInfo.dayCount++; // Day 1
	updateCupTreeAfterDay(&ts, &si, -1, 0, 75);
	updateSchedule(&ts, &si);

	// 3. Assert State After Round 1
	ASSERT_EQ(1, ts.cupInfo.slotWins[1], "R1: Slot 1 should have 1 win");
	ASSERT_EQ(1, ts.cupInfo.slotWins[3], "R1: Slot 3 should have 1 win");
	ASSERT_EQ(1, ts.cupInfo.slotWins[5], "R1: Slot 5 should have 1 win");
	ASSERT_EQ(1, ts.cupInfo.slotWins[7], "R1: Slot 7 should have 1 win");

	ASSERT_EQ(1, ts.cupInfo.cupTeamIndexTree[8], "R1: Team 1 should advance to slot 8");
	ASSERT_EQ(3, ts.cupInfo.cupTeamIndexTree[9], "R1: Team 3 should advance to slot 9");
	ASSERT_EQ(5, ts.cupInfo.cupTeamIndexTree[10], "R1: Team 5 should advance to slot 10");
	ASSERT_EQ(7, ts.cupInfo.cupTeamIndexTree[11], "R1: Team 7 should advance to slot 11");

	ASSERT_EQ(8, ts.cupInfo.schedule[0][0], "R1: New schedule should be slot 8");
	ASSERT_EQ(9, ts.cupInfo.schedule[0][1], "R1: New schedule should be slot 9");
	ASSERT_EQ(10, ts.cupInfo.schedule[1][0], "R1: New schedule should be slot 10");
	ASSERT_EQ(11, ts.cupInfo.schedule[1][1], "R1: New schedule should be slot 11");

	// --- ROUND 2: SEMIFINALS ---
	ts.cupInfo.dayCount++; // Day 2
	updateCupTreeAfterDay(&ts, &si, -1, 0, 75);
	updateSchedule(&ts, &si);

	// 5. Assert State After Round 2
	ASSERT_EQ(1, ts.cupInfo.slotWins[9], "R2: Slot 9 should have 1 win");
	ASSERT_EQ(1, ts.cupInfo.slotWins[11], "R2: Slot 11 should have 1 win");

	ASSERT_EQ(3, ts.cupInfo.cupTeamIndexTree[12], "R2: Team 3 should advance to slot 12");
	ASSERT_EQ(7, ts.cupInfo.cupTeamIndexTree[13], "R2: Team 7 should advance to slot 13");

	ASSERT_EQ(13, ts.cupInfo.schedule[0][1], "R2: New schedule should be slot 13");

	// --- ROUND 3: FINALS ---
	ts.cupInfo.dayCount++; // Day 3
	updateCupTreeAfterDay(&ts, &si, -1, 0, 75);
	updateSchedule(&ts, &si);

	// 7. Assert State After Round 3
	ASSERT_EQ(1, ts.cupInfo.slotWins[13], "R3: Slot 13 should have 1 win");
	ASSERT_EQ(7, ts.cupInfo.winnerIndex, "R3: Team 7 should be the tournament winner");

	teardown_test_state_info(&si, 8);
	return TEST_PASSED;
}
