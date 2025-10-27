#ifndef MENU_TYPES_H
#define MENU_TYPES_H

#include "globals.h"
#include "render.h"

#define ARROW_SCALE 0.05f
#define ARROW_SMALLER_SCALE 0.03f

#define PLAYER_LIST_ARROW_CONTINUE_POS 0.35f
#define PLAYER_LIST_ARROW_POS 0.42f
#define PLAYER_LIST_TEAM_TEXT_HEIGHT -0.4f
#define PLAYER_LIST_TEAM_TEXT_POS -0.1f
#define PLAYER_LIST_INFO_HEIGHT -0.3f
#define PLAYER_LIST_INFO_OFFSET 0.22f
#define PLAYER_LIST_INFO_NAME_OFFSET 0.1f
#define PLAYER_LIST_INFO_FIRST -0.4f
#define PLAYER_LIST_FIRST_PLAYER_HEIGHT -0.2f
#define PLAYER_LIST_PLAYER_OFFSET 0.05f
#define PLAYER_LIST_NUMBER_OFFSET 0.1f
#define PLAYER_LIST_CONTINUE_HEIGHT 0.45f

#define DEFAULT_TEAM_1 0
#define DEFAULT_TEAM_2 1

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
	MENU_STAGE_HELP,
	MENU_STAGE_GO_TO_GAME,
	// Special stage for quitting the application
	MENU_STAGE_QUIT
} MenuStage;

typedef enum {
	TEAM_SELECTION_STAGE_TEAM_1,
	TEAM_SELECTION_STAGE_CONTROL_1,
	TEAM_SELECTION_STAGE_TEAM_2,
	TEAM_SELECTION_STAGE_CONTROL_2,
	TEAM_SELECTION_STAGE_INNINGS
} TeamSelectionStage;

// State for batting order screen
typedef struct {
	int pointer;
	int rem;
	int mark;
	int batting_order[PLAYERS_IN_TEAM + JOKER_COUNT];
	int team_index;
	int player_control;
} BattingOrderState;

// State specific to the team selection screen
typedef struct {
	TeamSelectionStage state;
	int pointer;
	int team1;
	int team2;
	int team1_controller;
	int team2_controller;
	int innings;
	int rem;
} TeamSelectionState;

// State for hutunkeitto screen
typedef struct {
	int state;
	int batTimer;
	int batTimerLimit;
	int batTimerCount;
	int leftReady;
	int rightReady;
	int turnCount;
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
	int pointer;
	int rem;
} HutunkeittoState;

// State for the main menu screen
typedef struct {
	int pointer;
	int rem;
} FrontMenuState;

// State for the game-over screen (no internal fields required yet)
typedef struct {
	int placeholder;
} GameOverState;

// State for the help screen
typedef struct {
	int page;
} HelpMenuState;

// State for the home-run contest stages (one per team)
typedef struct {
	int team_index;        // index into StateInfo.teamData
	int player_control;    // 0=Pad1,1=Pad2,2=AI
	int pointer;           // current menu cursor (0=Continue, 1+ = player slots)
	int rem;               // total cursor positions = players+1
	int choiceCount;       // total picks per team (batters+runners)
	int choiceCounter;     // how many picks made so far
	int choices[2][MAX_HOMERUN_PAIRS]; // [0]=batters, [1]=runners
} HomerunContestState;

typedef enum {
	CUP_MENU_SCREEN_INITIAL,
	CUP_MENU_SCREEN_ONGOING,
	CUP_MENU_SCREEN_NEW_CUP,
	CUP_MENU_SCREEN_LOAD_CUP,
	CUP_MENU_SCREEN_VIEW_SCHEDULE,
	CUP_MENU_SCREEN_VIEW_TREE,
	CUP_MENU_SCREEN_SAVE_CUP,
	CUP_MENU_SCREEN_END_CREDITS
} CupMenuScreen;

typedef enum {
	NEW_CUP_STAGE_TEAM_SELECTION,
	NEW_CUP_STAGE_WINS_TO_ADVANCE,
	NEW_CUP_STAGE_INNINGS
} NewCupStage;

typedef struct {
	float x;
	float y;
} TreeCoordinates;

typedef struct {
	CupMenuScreen screen;
	int pointer;
	int rem;
	NewCupStage new_cup_stage;
	int team_selection;
	TreeCoordinates treeCoordinates[SLOT_COUNT];
} CupMenuState;

typedef struct MenuData {
	MenuStage stage;
	FrontMenuState front_menu;
	TeamSelectionState team_selection;
	BattingOrderState batting_order;
	HutunkeittoState hutunkeitto;
	GameOverState game_over;  // state for GAME_OVER stage
	HelpMenuState help_menu;
	CupMenuState cup_menu;
	int pointer;
	int rem;
	int team1;
	int team2;
	int team1_control;
	int team2_control;
	int inningsInPeriod;
	int playsFirst;
	int team1_batting_order[PLAYERS_IN_TEAM + JOKER_COUNT];
	int team2_batting_order[PLAYERS_IN_TEAM + JOKER_COUNT];
	// home-run contest state for each team
	HomerunContestState homerun1;
	HomerunContestState homerun2;
	GLuint arrowTexture;
	// Background texture for the new front menu (orthographic)
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
	float lightPos[4];
	MeshObject* planeMesh;
	MeshObject* handMesh;
	MeshObject* batMesh;
} MenuData;

#endif /* MENU_TYPES_H */
