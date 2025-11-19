#ifndef MENU_TYPES_H
#define MENU_TYPES_H

#include "globals.h"
#include "render.h"
#include "game_setup.h"

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

typedef struct {
	char name[32];
	int speed;
	int power;
} BattingOrderPlayer;

// State for batting order screen
typedef struct {
	int pointer;
	int rem;
	int mark;
	int batting_order[PLAYERS_IN_TEAM + JOKER_COUNT];
	int team_index;
	int player_control;
	BattingOrderPlayer players[PLAYERS_IN_TEAM + JOKER_COUNT];
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
	int numTeams;
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
	float creditsScrollX;
} CreditsMenuState;

typedef struct {
	int pointer;
	int rem;
} CupInitialState;

typedef struct {
	int pointer;
	int rem;
} CupOngoingState;

typedef struct {
	int pointer;
	int rem;
	NewCupStage new_cup_stage;
	int team_selection;
	int wins_to_advance;
} CupNewState;

typedef struct {
	int pointer;
	int rem;
} CupLoadSaveState;

typedef struct {
	int exists;
	int user_team_id;
	int opponent_team_id;
	int is_finished;
	int current_day;
} SaveSlotInfo;

typedef struct {
	TreeCoordinates treeCoordinates[SLOT_COUNT];
} CupViewTreeState;

typedef struct {
	CupMenuScreen screen;

	CupInitialState initial;
	CupOngoingState ongoing;
	CupNewState new_cup;
	CupLoadSaveState load_save;
	CupViewTreeState view_tree;
	CreditsMenuState credits_menu;
	
	SaveSlotInfo save_slots[5];  // Cached information about save files
} CupMenuState;

typedef struct MenuData {
	// Game Setup Data
	GameSetup pendingGameSetup;

	// Menu State
	MenuStage stage;
	FrontMenuState front_menu;
	TeamSelectionState team_selection;
	BattingOrderState batting_order;
	HutunkeittoState hutunkeitto;
	HomerunContestState homerun1;
	HomerunContestState homerun2;
	CupMenuState cup_menu;
	HelpMenuState help_menu;
} MenuData;

#endif /* MENU_TYPES_H */
