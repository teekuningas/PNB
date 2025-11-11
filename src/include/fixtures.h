#ifndef FIXTURES_H
#define FIXTURES_H

#include "game_setup.h"
#include "globals.h"

// Fixture configuration for command-line parsing
typedef struct {
	int enabled;
	char name[64];
	int team1;
	int team2;
	int team1_control;
	int team2_control;
} FixtureRequest;

// Pure functions that create GameSetup structs
// These can be used by both main.c (visual) and tests (automated)

// Creates a game setup for super inning scenario (period 3, tied 0-0)
void fixture_create_super_inning(GameSetup* setup, int team1, int team2,
                                 int team1_control, int team2_control);

// Creates a game setup for homerun contest scenario (period 4, tied 0-0)
void fixture_create_homerun_contest(GameSetup* setup, int team1, int team2,
                                    int team1_control, int team2_control);

// Creates a game setup starting at period 2
void fixture_create_period2_start(GameSetup* setup, int team1, int team2,
                                  int team1_control, int team2_control,
                                  int period1_score_team1, int period1_score_team2);

// Helper to parse --fixture command line args for main.c
// Returns 1 if fixture was parsed, 0 otherwise
int fixture_parse_args(int argc, char* argv[], FixtureRequest* request);

#endif /* FIXTURES_H */
