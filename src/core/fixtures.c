#include "fixtures.h"
#include <string.h>

// Default teams and controls
#define DEFAULT_FIXTURE_TEAM1 0
#define DEFAULT_FIXTURE_TEAM2 1
#define DEFAULT_FIXTURE_CONTROL1 0  // Player 1
#define DEFAULT_FIXTURE_CONTROL2 1  // Player 2

void fixture_create_super_inning(GameSetup* setup, int team1, int team2,
                                 int team1_control, int team2_control)
{
	memset(setup, 0, sizeof(GameSetup));

	setup->gameMode = GAME_MODE_SUPER_INNING;
	setup->team1 = team1;
	setup->team2 = team2;
	setup->team1_control = team1_control;
	setup->team2_control = team2_control;
	setup->halfInningsInPeriod = 4;  // Standard
	setup->playsFirst = 0;  // Team 1 bats first

	// Initialize batting orders (default sequential)
	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		setup->team1_batting_order[i] = i;
		setup->team2_batting_order[i] = i;
	}

	setup->homerun_choice_count = 0;
}

void fixture_create_homerun_contest(GameSetup* setup, int team1, int team2,
                                    int team1_control, int team2_control)
{
	memset(setup, 0, sizeof(GameSetup));

	setup->gameMode = GAME_MODE_HOMERUN_CONTEST;
	setup->team1 = team1;
	setup->team2 = team2;
	setup->team1_control = team1_control;
	setup->team2_control = team2_control;
	setup->halfInningsInPeriod = 4;
	setup->playsFirst = 0;

	// Initialize batting orders
	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		setup->team1_batting_order[i] = i;
		setup->team2_batting_order[i] = i;
	}

	// Homerun contest needs player selections
	// First round: 5 batters + 5 runners = 10 total picks
	setup->homerun_choice_count = 5;

	// Default selections (indices 0-4 for batters, 5-9 for runners)
	for (int i = 0; i < 5; i++) {
		setup->homerun_choices1[0][i] = i;      // Team 1 batters (0-4)
		setup->homerun_choices1[1][i] = i + 5;  // Team 1 runners (5-9)
		setup->homerun_choices2[0][i] = i;      // Team 2 batters (0-4)
		setup->homerun_choices2[1][i] = i + 5;  // Team 2 runners (5-9)
	}
}

void fixture_create_period2_start(GameSetup* setup, int team1, int team2,
                                  int team1_control, int team2_control,
                                  int period1_score_team1, int period1_score_team2)
{
	memset(setup, 0, sizeof(GameSetup));

	setup->gameMode = GAME_MODE_NORMAL;
	setup->team1 = team1;
	setup->team2 = team2;
	setup->team1_control = team1_control;
	setup->team2_control = team2_control;
	setup->halfInningsInPeriod = 4;
	setup->playsFirst = 1;  // Switch sides for period 2

	// Initialize batting orders
	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		setup->team1_batting_order[i] = i;
		setup->team2_batting_order[i] = i;
	}

	setup->homerun_choice_count = 0;

	// Note: Period 1 scores would need to be set in globalGameInfo after initialization
	// This is handled by the caller
}

int fixture_parse_args(int argc, char* argv[], FixtureRequest* request)
{
	memset(request, 0, sizeof(FixtureRequest));
	request->enabled = 0;
	request->team1 = DEFAULT_FIXTURE_TEAM1;
	request->team2 = DEFAULT_FIXTURE_TEAM2;
	request->team1_control = DEFAULT_FIXTURE_CONTROL1;
	request->team2_control = DEFAULT_FIXTURE_CONTROL2;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--fixture") == 0 && i + 1 < argc) {
			request->enabled = 1;
			strncpy(request->name, argv[i + 1], sizeof(request->name) - 1);
			i++; // Skip next arg (the fixture name)
		} else if (strcmp(argv[i], "--team1") == 0 && i + 1 < argc) {
			request->team1 = atoi(argv[i + 1]);
			i++;
		} else if (strcmp(argv[i], "--team2") == 0 && i + 1 < argc) {
			request->team2 = atoi(argv[i + 1]);
			i++;
		} else if (strcmp(argv[i], "--control1") == 0 && i + 1 < argc) {
			request->team1_control = atoi(argv[i + 1]);
			i++;
		} else if (strcmp(argv[i], "--control2") == 0 && i + 1 < argc) {
			request->team2_control = atoi(argv[i + 1]);
			i++;
		}
	}

	return request->enabled;
}

void fixture_create_cup_final_super_inning(GameSetup* gameSetup, int team1, int team2, int team1_control, int team2_control)
{
	gameSetup->gameMode = GAME_MODE_SUPER_INNING;
	gameSetup->launchType = GAME_LAUNCH_NEW;
	gameSetup->team1 = team1;
	gameSetup->team2 = team2;
	gameSetup->team1_control = team1_control;
	gameSetup->team2_control = team2_control;
	gameSetup->playsFirst = 0;
	gameSetup->halfInningsInPeriod = 4; // Standard innings for periods 0 and 1

	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		gameSetup->team1_batting_order[i] = i;
		gameSetup->team2_batting_order[i] = i;
	}
}
