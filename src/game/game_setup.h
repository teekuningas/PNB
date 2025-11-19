#ifndef GAME_SETUP_H
#define GAME_SETUP_H

#include "globals.h"
#include "cup.h"

// Enum to represent the game mode, decoupling game logic from menu specifics.
typedef enum {
	GAME_MODE_NORMAL,
	GAME_MODE_SUPER_INNING,
	GAME_MODE_HOMERUN_CONTEST
} GameMode;

typedef enum {
	GAME_LAUNCH_NEW,
	GAME_LAUNCH_RETURN_INTER_PERIOD,
	GAME_LAUNCH_RETURN_HOMERUN_CONTEST
} GameLaunchType;

// This struct holds all the parameters needed to initialize a a game.
// It is populated from the menu and used to set up the game state.
typedef struct GameSetup {
	GameLaunchType launchType;
	GameMode gameMode;
	TeamID team1;
	TeamID team2;
	int team1_control;
	int team2_control;
	int halfInningsInPeriod;
	int playsFirst;
	int team1_batting_order[PLAYERS_IN_TEAM + JOKER_COUNT];
	int team2_batting_order[PLAYERS_IN_TEAM + JOKER_COUNT];
	int homerun_choices1[2][MAX_HOMERUN_PAIRS];
	int homerun_choices2[2][MAX_HOMERUN_PAIRS];
	int homerun_choice_count;
} GameSetup;

void initializeGameFromMenu(StateInfo* stateInfo, const GameSetup* gameSetup);
void returnToGame(StateInfo* stateInfo);

#endif /* GAME_SETUP_H */