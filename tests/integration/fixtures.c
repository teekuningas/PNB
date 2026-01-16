#include "fixtures.h"
#include "field_layout.h"
#include "fill_player_data.h"
#include "game_setup.h"
#include "mutable_world.h"
#include "common_logic.h"
#include "game_analysis.h"
#include <stdlib.h>
#include <string.h>

StateInfo* setup_test_state()
{
	StateInfo* state = malloc(sizeof(StateInfo));
	memset(state, 0, sizeof(StateInfo));

	state->teamData = malloc(sizeof(TeamData) * 2);
	memset(state->teamData, 0, sizeof(TeamData) * 2);

	for (int i = 0; i < 2; ++i) {
		state->teamData[i].players = malloc(sizeof(PlayerData) * 12);
		for (int j = 0; j < 12; ++j) {
			state->teamData[i].players[j].name = malloc(sizeof(char) * 10);
			sprintf(state->teamData[i].players[j].name, "Player %d", j + 1);
		}
	}

	state->fieldPositions = malloc(sizeof(FieldPositions));
	field_init_positions(state->fieldPositions);

	state->match = malloc(sizeof(MatchSession));
	memset(state->match, 0, sizeof(MatchSession));

	// Initialize indices to -1
	for(int i=0; i<4; i++) {
		state->match->pII.catcherOnBaseIndex[i] = -1;
		state->match->pII.catcherReplacerOnBaseIndex[i] = -1;
	}
	for(int i=0; i<PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		state->match->referee.battingPlayers[i].currentSafetyBase = BASE_NONE;
	}
	state->match->pII.hasBallIndex = -1;
	state->match->pII.lastHadBallIndex = -1;
	state->match->pII.controlIndex = -1;
	state->match->referee.woundingEvaluationTimer = -1;
	state->match->gameFlowState.endOfInningCounter = -1;


	return state;
}

void setup_runner_at_first_base(StateInfo* state)
{
	unsigned int seed = 0;
	GameSetup setup = {0};
	setup.launchType = GAME_LAUNCH_NEW;
	setup.gameMode = GAME_MODE_NORMAL;
	setup.team1 = 0;
	setup.team2 = 1;
	setup.team1_control = 0;
	setup.team2_control = 2;
	setup.halfInningsInPeriod = 4;
	setup.playsFirst = 0;

	initializeGameFromMenu(state, &setup, &seed);

	initGameAnalysis(&(state->match->gameFlowState));
	state->match->playerInfo[0].bTPI.baseId = BASE_FIRST;
	state->match->referee.battingPlayers[0].baseAtPitchStart = BASE_FIRST;
	state->match->referee.battingPlayers[0].currentSafetyBase = BASE_FIRST;
}

void setup_runner_at_third_base(StateInfo* state)
{
	unsigned int seed = 0;
	GameSetup setup = {0};
	setup.launchType = GAME_LAUNCH_NEW;
	setup.gameMode = GAME_MODE_NORMAL;
	setup.team1 = 0;
	setup.team2 = 1;
	setup.team1_control = 0;
	setup.team2_control = 2;
	setup.halfInningsInPeriod = 4;
	setup.playsFirst = 0;

	initializeGameFromMenu(state, &setup, &seed);

	initGameAnalysis(&(state->match->gameFlowState));
	state->match->playerInfo[0].bTPI.baseId = BASE_THIRD;
	state->match->referee.battingPlayers[0].baseAtPitchStart = BASE_THIRD;
	state->match->referee.battingPlayers[0].currentSafetyBase = BASE_THIRD;
}

void cleanup_test_state(StateInfo* state)
{
	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 12; ++j) {
			free(state->teamData[i].players[j].name);
		}
		free(state->teamData[i].players);
	}
	free(state->match);
	free(state->fieldPositions);
	free(state->teamData);
	free(state);
}

void set_test_player_state(StateInfo* state, int playerIndex, PlayerUnitState newState)
{
	BattingTeamPlayerInfo* b = &state->match->playerInfo[playerIndex].bTPI;

	// Simply set the new state enum - no legacy flags needed
	b->state = newState;
}

