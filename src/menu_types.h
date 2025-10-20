#ifndef MENU_TYPES_H
#define MENU_TYPES_H

#include "globals.h" // For PLAYERS_IN_TEAM, JOKER_COUNT, etc.

// Describes the reason the menu is being entered, usually from the game screen.
// This determines the menu's starting screen.
typedef enum {
	MENU_ENTRY_NORMAL = 0,          // Normal startup, show the main front page.
	MENU_ENTRY_INTER_PERIOD = 1,    // Between periods, go to batting order.
	MENU_ENTRY_SUPER_INNING = 2,    // Game tied, go to batting order for super inning.
	MENU_ENTRY_HOMERUN_CONTEST = 3, // Super inning tied, go to homerun contest setup.
	MENU_ENTRY_GAME_OVER = 4        // Game is over, show the summary screen.
} MenuMode;

typedef struct _MenuInfo {
	MenuMode mode;
} MenuInfo;

typedef enum {
	MENU_STAGE_FRONT,
	MENU_STAGE_TEAM_SELECTION,
	MENU_STAGE_BATTING_ORDER_1,
	MENU_STAGE_BATTING_ORDER_2,
	MENU_STAGE_HUTUNKEITTO,
	MENU_STAGE_GAME_OVER,
	MENU_STAGE_HOMERUN_CONTEST_1,
	MENU_STAGE_HOMERUN_CONTEST_2,
	MENU_STAGE_CUP,
	MENU_STAGE_HELP
} MenuStage;

typedef enum {
	TEAM_SELECTION_STAGE_TEAM_1,
	TEAM_SELECTION_STAGE_CONTROL_1,
	TEAM_SELECTION_STAGE_TEAM_2,
	TEAM_SELECTION_STAGE_CONTROL_2,
	TEAM_SELECTION_STAGE_INNINGS
} TeamSelectionStage;

typedef struct MenuData {
	MenuStage stage;
	TeamSelectionStage stage_1_state;
	int pointer;
	int rem;
	int mark;
	int stage_4_state;
	int stage_8_state;
	int stage_9_state;
	int stage_8_state_1_level;
	int batTimer;
	int batTimerLimit;
	int batTimerCount;
	int updatingCanStart;
	int team1;
	int team2;
	int team1_control;
	int team2_control;
	int inningsInPeriod;
	int team1_batting_order[PLAYERS_IN_TEAM + JOKER_COUNT];
	int team2_batting_order[PLAYERS_IN_TEAM + JOKER_COUNT];
	int batting_order[PLAYERS_IN_TEAM + JOKER_COUNT];
	int team_1_choices[2][5];
	int team_2_choices[2][5];
	int choiceCounter;
	int choiceCount;
	int leftReady;
	int rightReady;
	int turnCount;
	int cupGame;
	float batHeight;
	float batPosition;
	float leftHandHeight;
	float leftHandPosition;
	float rightHandHeight;
	float rightHandPosition;
	float tempLeftHeight;
	float handsZ;
	float refereeHandHeight;
	int leftScaleCount;
	int rightScaleCount;
	int playsFirst;
	int teamSelection;
	GLuint arrowTexture;
	GLuint catcherTexture;
	GLuint batterTexture;
	GLuint slotTexture;
	GLuint trophyTexture;
	GLuint team1Texture;
	GLuint team2Texture;
	GLuint team3Texture;
	GLuint team4Texture;
	GLuint team5Texture;
	GLuint team6Texture;
	GLuint team7Texture;
	GLuint team8Texture;
	GLuint planeDisplayList;
	GLuint handDisplayList;
	GLuint batDisplayList;
	Vector3D cam, look, up;
} MenuData;

#endif /* MENU_TYPES_H */
