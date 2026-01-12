#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../../src/include/globals.h"
#include "../src/game/referee.h"
#include "fixtures.h"
#include "test_rules_referee.h"

// Mock for testing
StateInfo* create_mock_state()
{
	StateInfo* state = (StateInfo*)malloc(sizeof(StateInfo));
	state->match = (MatchSession*)malloc(sizeof(MatchSession));
	state->fieldPositions = (FieldPositions*)malloc(sizeof(FieldPositions));

	memset(state->match, 0, sizeof(MatchSession));
	memset(state->fieldPositions, 0, sizeof(FieldPositions));

	// Initialize referee state properly (wounding timer must be -1, not 0)
	state->match->referee.woundingCatchTimer = -1;

	// Setup standard field positions for distance checks
	state->fieldPositions->pitchPlate.x = 0.0f;
	state->fieldPositions->pitchPlate.z = 0.0f;
	state->fieldPositions->firstBase.x = 30.0f;
	state->fieldPositions->firstBase.z = -30.0f;
	state->fieldPositions->secondBase.x = -30.0f;
	state->fieldPositions->secondBase.z = -30.0f;
	state->fieldPositions->thirdBase.x = -30.0f;
	state->fieldPositions->thirdBase.z = 10.0f;

	return state;
}

void destroy_mock_state(StateInfo* state)
{
	free(state->match);
	free(state->fieldPositions);
	free(state);
}

void test_referee_force_out_at_second(void)
{
	StateInfo* state = create_mock_state();

	// Setup: Ball at 2nd Base
	state->match->pII.hasBallIndex = 15; // Fielder has ball
	state->match->ballInfo.location = state->fieldPositions->secondBase;

	// Runner physically at 1st Base
	int runnerIdx = 0;
	state->match->playerInfo[runnerIdx].bTPI.baseId = BASE_FIRST;

	// KEY for §33 Pesäkilpa: Runner has pesäturva (safety) at Base 1 but is RUNNING ("irti")
	state->match->referee.battingPlayers[runnerIdx].currentSafetyBase = BASE_FIRST;
	Referee_Update(
	    state,
	    &state->match->referee,
	    &state->match->halfInningState,
	    &state->match->gameControl,
	    &state->match->playerCounters,
	    &state->match->scoreboard
	);

	// Should be OUT: has safety at Base 1, is running (irti), ball at Base 2
	assert(state->match->referee.battingPlayers[runnerIdx].isOut == 1);
	assert(state->match->halfInningState.event == EVENT_OUT);

	destroy_mock_state(state);
	printf("test_referee_force_out_at_second PASSED\n");
}

void test_referee_safe_runner_not_out(void)
{
	StateInfo* state = create_mock_state();

	// Setup: Ball at 2nd Base
	state->match->pII.hasBallIndex = 15;
	state->match->ballInfo.location = state->fieldPositions->secondBase;

	// Runner AT 1st Base, but SAFE
	int runnerIdx = 0;
	state->match->playerInfo[runnerIdx].bTPI.baseId = BASE_FIRST;
	state->match->playerInfo[runnerIdx].bTPI.state = PLAYER_STATE_ON_BASE; // Protected
	state->match->referee.battingPlayers[runnerIdx].currentSafetyBase = BASE_FIRST;

	Referee_Update(
	    state,
	    &state->match->referee,
	    &state->match->halfInningState,
	    &state->match->gameControl,
	    &state->match->playerCounters,
	    &state->match->scoreboard
	);

	assert(state->match->referee.battingPlayers[runnerIdx].isOut == 0);

	destroy_mock_state(state);
	printf("test_referee_safe_runner_not_out PASSED\n");
}

void run_referee_tests(void)
{
	printf("Running Referee Unit Tests...\n");
	test_referee_force_out_at_second();
	test_referee_safe_runner_not_out();
}
