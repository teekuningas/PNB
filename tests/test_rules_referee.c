#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../../src/include/globals.h"
#include "../../src/game/rules_pure/referee.h"
#include "fixtures.h"
#include "test_rules_referee.h"

// Mock for testing
StateInfo* create_mock_state() {
	StateInfo* state = (StateInfo*)malloc(sizeof(StateInfo));
	state->localGameInfo = (LocalGameInfo*)malloc(sizeof(LocalGameInfo));
	state->fieldPositions = (FieldPositions*)malloc(sizeof(FieldPositions));
	
	memset(state->localGameInfo, 0, sizeof(LocalGameInfo));
	memset(state->fieldPositions, 0, sizeof(FieldPositions));
	
	// Setup standard field positions for distance checks
	state->fieldPositions->pitchPlate.x = 0.0f; state->fieldPositions->pitchPlate.z = 0.0f;
	state->fieldPositions->firstBase.x = 30.0f; state->fieldPositions->firstBase.z = -30.0f;
	state->fieldPositions->secondBase.x = -30.0f; state->fieldPositions->secondBase.z = -30.0f;
	state->fieldPositions->thirdBase.x = -30.0f; state->fieldPositions->thirdBase.z = 10.0f;
	
	return state;
}

void destroy_mock_state(StateInfo* state) {
	free(state->localGameInfo);
	free(state->fieldPositions);
	free(state);
}

void test_referee_ball_at_home(void) {
	StateInfo* state = create_mock_state();
	
	// Setup: Ball at Home Plate
	state->localGameInfo->pII.hasBallIndex = 0; // Player 0 has ball
	state->localGameInfo->ballInfo.location.x = 0.0f;
	state->localGameInfo->ballInfo.location.z = 1.0f; // > HOME_LINE_Z (-0.65)
	
	RefereeDecisions decisions = Referee_Analyze(state);
	
	assert(decisions.ballHome == 1);
	
	destroy_mock_state(state);
	printf("test_referee_ball_at_home PASSED\n");
}

void test_referee_force_out_at_second(void) {
	StateInfo* state = create_mock_state();
	
	// Setup: Ball at 2nd Base
	state->localGameInfo->pII.hasBallIndex = 15; // Fielder
	state->localGameInfo->ballInfo.location = state->fieldPositions->secondBase; // Exact match
	
	// Runner at 1st Base (forced to run)
	int runnerIdx = 0;
	state->localGameInfo->playerInfo[runnerIdx].bTPI.baseId = BASE_FIRST; 
	// Not safe -> forcing "force out" condition if ball beats them to 2nd
	// Wait, is_runner_forced_out logic:
	// ball at 2 (index 2). checkBaseId = 1.
	// player at 1. MATCH.
	
	RefereeDecisions decisions = Referee_Analyze(state);
	
	assert(decisions.playerDecisions[runnerIdx].isOut == 1);
	assert(decisions.eventOut == 1);
	
	destroy_mock_state(state);
	printf("test_referee_force_out_at_second PASSED\n");
}

void test_referee_safe_runner_not_out(void) {
	StateInfo* state = create_mock_state();
	
	// Setup: Ball at 2nd Base
	state->localGameInfo->pII.hasBallIndex = 15; 
	state->localGameInfo->ballInfo.location = state->fieldPositions->secondBase;
	
	// Runner AT 1st Base, but SAFE
	int runnerIdx = 0;
	state->localGameInfo->playerInfo[runnerIdx].bTPI.baseId = BASE_FIRST;
	state->localGameInfo->playerInfo[runnerIdx].bTPI.state = PLAYER_STATE_SAFE_ON_BASE; // Protected
	state->localGameInfo->referee.battingPlayers[runnerIdx].currentSafetyBase = BASE_FIRST;
	
	RefereeDecisions decisions = Referee_Analyze(state);
	
	assert(decisions.playerDecisions[runnerIdx].isOut == 0);
	
	destroy_mock_state(state);
	printf("test_referee_safe_runner_not_out PASSED\n");
}

void run_referee_tests(void) {
	printf("Running Referee Unit Tests...\n");
	test_referee_ball_at_home();
	test_referee_force_out_at_second();
	test_referee_safe_runner_not_out();
}
