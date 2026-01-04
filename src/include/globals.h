#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

// linux and windows code
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// global constants
#define PI 3.141f
#define EPSILON 0.005f
#define UPDATE_INTERVAL 20
#define PERSPECTIVE_ASPECT_RATIO (16.0f/9.0f)
#define VIRTUAL_WIDTH 1920
#define VIRTUAL_HEIGHT 1080

// game states
typedef enum {
	SCREEN_LOADING,
	SCREEN_MAIN_MENU,
	SCREEN_GAME
} ScreenState;

// NEW ENUMS FOR MILESTONE 7 (DATA RENAISSANCE)
// --------------------------------------------

typedef enum {
	PLAYER_STATE_IDLE = 0,
	PLAYER_STATE_AT_BAT,
	PLAYER_STATE_SAFE_ON_BASE,      // Replaces isOnBase=1
	PLAYER_STATE_RUNNING,           // Replaces isOnBase=0, out=0, wounded=0
	PLAYER_STATE_ADVANCING_FREELY,  // Replaces takingFreeWalk=1
	PLAYER_STATE_LEADING,           // Replaces leading=1
	PLAYER_STATE_OUT,               // Replaces out=1
	PLAYER_STATE_WOUNDED,           // Replaces wounded=1
	PLAYER_STATE_SCORED             // Explicit state for having scored
} PlayerUnitState;

typedef enum {
	BASE_HOME = 0,
	BASE_FIRST = 1,
	BASE_SECOND = 2,
	BASE_THIRD = 3,
	BASE_HOME_SCORED = 4, // Explicitly distinct from starting at home
	BASE_NONE = -1
} BaseID;

typedef enum {
	EVENT_NONE = 0,
	EVENT_OUT = 1,
	EVENT_WOUNDED = 2,
	EVENT_RUN_SCORED = 3,
	EVENT_OUT_OF_BOUNDS = 4,
	EVENT_STRIKE = 5,
	EVENT_BALL = 6,
	EVENT_INNING_ENDING = 7,
	EVENT_NEXT_PAIR = 8,
	EVENT_TWO_RUNS_SCORED = 9
} GameEventType;

typedef enum {
	GAME_MODE_NORMAL = 0,
	GAME_MODE_SUPER_INNING = 1,    // Potential mapping for period 2/3 if distinct
	GAME_MODE_HOMERUN_CONTEST = 2  // Replaces period >= 4
} GamePeriodMode;

// --------------------------------------------

// a lot of constants
#define LIGHT_SOURCE_POSITION_X 30.0f
#define LIGHT_SOURCE_POSITION_Y 50.0f
#define LIGHT_SOURCE_POSITION_Z -50.0f

#define PLATE_WIDTH 1.5f
#define GRAVITY 0.003f
#define ZERO_BATTING_ANGLE (19*PI/16)
#define SHADOW_HEIGHT 0.01f

#define WALK_SPEED 0.06f
#define RUN_SPEED 0.12f
#define LEAD_STEP 0.05f
#define BATTING_TEAM_RUN_FACTOR 1.1f

#define GROUND_WIDTH 40.0f
#define GROUND_LENGTH 30.0f
#define GROUND_OFFSET_X (-GROUND_WIDTH/34)
#define GROUND_OFFSET_Z (-GROUND_LENGTH/6)
#define FIELD_BACK (-4.5f*GROUND_LENGTH + GROUND_OFFSET_Z)
#define FIELD_FRONT (1.5f*GROUND_LENGTH + GROUND_OFFSET_Z)
#define FIELD_LEFT (-2.5f*GROUND_WIDTH + GROUND_OFFSET_X)
#define FIELD_RIGHT (2.5f*GROUND_WIDTH + GROUND_OFFSET_X)
#define FENCE_OFFSET 0.5f

#define HOME_RADIUS 6.0f
#define HOME_LINE_Z -0.65f
#define BATTING_RADIUS 3.5f

#define BALL_HEIGHT_WITH_PLAYER 1.45f

#define BALL_INIT_SPEED_X 0.075f
#define BALL_INIT_SPEED_Z 0.03f
#define BALL_INIT_SPEED_Y 0.1f
#define BALL_INIT_LOCATION_X -5.0f
#define BALL_INIT_LOCATION_Y 3.0f
#define BALL_INIT_LOCATION_Z -2.0f

#define BALL_SIZE 0.1f

#define INIT_LOCALS_COUNT 5

#define DISTANCE_FROM_HOME_LOCATION_THRESHOLD 1.75f
#define TARGET_ACHIEVED_THRESHOLD 0.25f // has to have some room here because we are calculating velocity only at the start of movement and everything approximations anyway
// counts
#define JOKER_COUNT 3
#define PLAYERS_IN_TEAM 9
#define RANKED_FIELDERS_COUNT 5
#define CHANGE_PLAYER_COUNT 3
#define BASE_COUNT 4
#define DIRECTION_COUNT 4
#define MAX_HOMERUN_PAIRS 5

// Menu slots
#define SLOT_COUNT 14

// keycodes

#define KEY_COUNT 11

#define KEY_A 0
#define KEY_B 1
#define KEY_PLUS 2
#define KEY_MINUS 3
#define KEY_1 4
#define KEY_2 5
#define KEY_UP 6
#define KEY_DOWN 7
#define KEY_LEFT 8
#define KEY_RIGHT 9
#define KEY_HOME 10

// soundcodes

#define SOUND_MENU 1
#define SOUND_SWING 2
#define SOUND_CATCH 3
// down[i][j] is 1 when key is down
// released[i][j] is 1 for one frame when key is released
typedef struct _KeyStates {
	int released[3][KEY_COUNT];
	int down[3][KEY_COUNT];
	int imitateKeyPress[KEY_COUNT];
} KeyStates;
// simple struct to keep spatial information
typedef struct _Vector3D {
	float x;
	float y;
	float z;
} Vector3D;
// relevant field positions. hardcoded in the immutable_world.c
typedef struct _FieldPositions {
	Vector3D pitchPlate;
	Vector3D homeRunPoint;
	Vector3D pitcher;
	Vector3D firstBase;
	Vector3D secondBase;
	Vector3D thirdBase;
	Vector3D firstBaseRun;
	Vector3D secondBaseRun;
	Vector3D thirdBaseRun;
	Vector3D leftPoint;
	Vector3D runLeftPoint;
	Vector3D backLeftPoint;
	Vector3D backRightPoint;
	Vector3D rightPoint;
	Vector3D bottomRightCatcher;
	Vector3D middleLeftCatcher;
	Vector3D middleRightCatcher;
	Vector3D backLeftCatcher;
	Vector3D backRightCatcher;
} FieldPositions;

// ball related information
typedef struct _BallInfo {
	Vector3D velocity;
	Vector3D location;
	Vector3D lastLocation;
	int visible; // is ball visible. ball is not visible when some player has it
	int moving; // is ball moving, only update player orientations and ball's position if ball is moving
	int hasHitGround; // has the ball hit ground
	int onGround; // is ball rolling on ground
	int hasHitGroundOutOfBounds; // is set to 1 when ball hits ground out of bounds, back to 0 when ball is catched
	int hitsGroundToUnWound; // if ball hits ground after being catched as wounding catch, is set to 1. checked only after wounding catch so, it is set to 0 when the catch is made.
	int needsMoveUpdate; // when ball having players' velocity changes, ball's velocity must change too
	int lastLastLocationUpdate; // when ball stops, we must sync lastLocation and location.
} BallInfo;
// struct to keep player data
typedef struct _PlayerData {
	char* id;
	char* name;
	int speed;
	int power;
} PlayerData;

// struct keeps information about teams
typedef struct _TeamData {
	char* id;
	char* name;
	int numPlayers;
	PlayerData* players;
} TeamData;
typedef enum {
	ACTION_IDLE = 0,
	ACTION_TRIGGER_START = 1,
	ACTION_ACTIVE = 2,
	ACTION_TRIGGER_STOP = 3
} ActionTriggerState;

typedef enum {
	PITCH_ACTION_IDLE = 0,
	PITCH_ACTION_START = 1,      // Trigger: start windup
	PITCH_ACTION_POWER_WAIT = 2,  // Active: winding up, waiting for power select
	PITCH_ACTION_POWER_SET = 3,   // Trigger: power selected
	PITCH_ACTION_ANGLE_WAIT = 4,  // Active: throw animation, waiting for angle select
	PITCH_ACTION_ANGLE_SET = 5    // Trigger: angle selected, release ball
} PitchActionPhase;

typedef enum {
	BAT_ACTION_IDLE = 0,
	BAT_ACTION_WAIT_FOR_BALL = 1, // Active: batter ready, ball in air
	BAT_ACTION_POWER_SET = 2,     // Trigger: swing button pressed (select power)
	BAT_ACTION_ANGLE_WAIT = 3,    // Active: swinging, waiting for angle select
	BAT_ACTION_ANGLE_SET = 4,      // Trigger: angle selected
	BAT_ACTION_DONE = 5           // Active: animation finishing
} BatActionPhase;

typedef enum {
	CHOOSE_BATTER_IDLE = 0,
	CHOOSE_BATTER_NEXT = 1,
	CHOOSE_BATTER_SELECT = 2
} ChooseBatterAction;

typedef enum {
	FREE_WALK_IDLE = 0,
	FREE_WALK_ACCEPT = 1,
	FREE_WALK_REJECT = 2
} FreeWalkAction;

/*
Action flags. used in action_invocation.c and action_implementation.c.
flag is set when key event happens in action_invocation, then its set off or modified when its handled in
action_implementation
*/
typedef struct _BattingTeamActionFlags {
	ActionTriggerState baseRun[4];
	ChooseBatterAction chooseBatter;
	FreeWalkAction takeFreeWalk;
	BatActionPhase swing;
	ActionTriggerState increaseBatterAngle;
	ActionTriggerState decreaseBatterAngle;
} BattingTeamActionFlags;

typedef struct _CatchingTeamActionFlags {
	ActionTriggerState move[4];
	ActionTriggerState throwToBase[4];
	ActionTriggerState changePlayer;
	ActionTriggerState run;
	ActionTriggerState dropBall;
	PitchActionPhase pitch;
	int actionKeyLock;
} CatchingTeamActionFlags;

typedef struct _ActionFlags {
	BattingTeamActionFlags bTAF;
	CatchingTeamActionFlags cTAF;
} ActionFlags;
// spatial data for every player
typedef struct _TechnicalPlayerInfo {
	Vector3D location;
	Vector3D lastLocation;
	Vector3D homeLocation;
	Vector3D targetLocation;
	Vector3D velocity;
	Vector3D orientation;
} TechnicalPlayerInfo;

// MILESTONE 7.5 - Separating Control State from Domain State
typedef struct _PlayerRuntimeState {
	int arrivedToBase;       // Optimization flag
	int woundedApply;        // Deferred execution
	int passedPathPoint;     // State machine variable
	int goingForward;        // Direction tracking
	int hasMadeRunOnThirdBase; // Guard flag
} PlayerRuntimeState;

// batting team related flags.
typedef struct _BattingTeamPlayerInfo {
	char* name; // some info to be shown in screen to guide player
	int speed; // used to make some player a bit faster than others
	int power; // used to make some players bat a bit harder than others
	int number; // number is shown on screen
	int originalBase; // base when pitch started
	int joker; // 0 original player, 1 has right to be used, 2 has been used already

	// MILESTONE 7 (DATA RENAISSANCE) - Type-safe state fields
	PlayerUnitState state;
	BaseID baseId;
} BattingTeamPlayerInfo;

typedef enum {
	PLAYER_ANIM_STAND_NO_BALL = 0,
	PLAYER_ANIM_STAND_WITH_BALL = 1,
	PLAYER_ANIM_WALK_NO_BALL = 2,
	PLAYER_ANIM_WALK_WITH_BALL = 3,
	PLAYER_ANIM_RUN_NO_BALL = 4,
	PLAYER_ANIM_RUN_WITH_BALL = 5,
	PLAYER_ANIM_PITCH_WINDUP = 6,
	PLAYER_ANIM_PITCH_THROW = 7,
	PLAYER_ANIM_THROW_WINDUP = 8,
	PLAYER_ANIM_THROW_RELEASE = 9,
	PLAYER_ANIM_STAND_BARE = 10,
	PLAYER_ANIM_WALK_BARE = 11,
	PLAYER_ANIM_RUN_BARE = 12,
	PLAYER_ANIM_BATTER_READY = 13,
	PLAYER_ANIM_BAT_SWING_1 = 14,
	PLAYER_ANIM_BAT_SWING_2 = 15,
	PLAYER_ANIM_BAT_SWING_3 = 16
} PlayerAnimationModel;

typedef struct _CommonPlayerInfo {
	// player information. name is shown on the screen sometimes, team is used in players.c
	// and stats are used to make players behave in different ways on some situations.
	int team;

	int moving; // 0 doesnt, 1 moves
	int running; // 0 doesnt, 1 runs
	int looksForTarget; // used in conjuction with targetLocation when trying to go catching a ball for example
	int lastLastLocationUpdate; // player needs to have his lastLocation updated, necessary when controlling player and moving stops. twitching without this
	/* how to intepret the model number:
		See PlayerAnimationModel enum above
	*/
	PlayerAnimationModel model;
	int animationStage;
	int animationStageCount;
	int animationFrequency;

} CommonPlayerInfo;

typedef struct _CatchingTeamPlayerInfo {
	int position; // pitcher, catcher, 1st baseman..
	int movesToDirection[4]; // does player move to direction x ( north, east, south, west )
	int isNearHomeLocation; // used to do base replacing stuff.
	int replacingStage; // 1 is going to replace or is at the base, 0 is coming back or is at home location.
	int replacingBase; // in which base is the player replacing
	int busyCatching; // flag set when player is trying to run in hopes of catching the ball
	int throwRecoil; // 1 when throwing animation is still going on. set one when ball leaves.
} CatchingTeamPlayerInfo;

typedef struct _PlayerInfo {
	TechnicalPlayerInfo tPI;
	CommonPlayerInfo cPI;
	CatchingTeamPlayerInfo cTPI;
	BattingTeamPlayerInfo bTPI;

} PlayerInfo;

typedef struct _TeamInfo {
	int value; // which team, like 1 for ankkurit, 2 for vimpeli etc.
	int control; // who controls, 0 player 1, 1, player 2, 2 AI
	int runs; // how many runs has this team
	int period0Runs;
	int period1Runs;
	int period2Runs;
	int period3Runs;
	int batterRunnerIndices[2][5];
	int batterOrder[12]; // this teams' batting order, used in many innings
	int batterOrderIndex; // what player is next
} TeamInfo;

typedef struct _PlayerIndexInfo {
	int safeOnBaseIndex[4]; // who is safe on base i
	int battingTeamOnFieldIndices[4]; // here is a list of batting team players currently on field.
	// they arent in particular order, and there can be gaps ( - 1 ).
	int catcherOnBaseIndex[4]; // whos is baseman on base i
	int catcherReplacerOnBaseIndex[4]; // who is the guy replacing base i, if normal catcher is away
	int fielderRankedIndices[RANKED_FIELDERS_COUNT]; // indices of currently important players. user can change between
	// these, and these are also used to select players for busyCatching.
	int jokerIndices[3]; // indices of all jokers.
	int batterSelectionIndex; // index of the batter that would be selected if there was decision available
	int hasBallIndex; // who has the ball
	int lastHadBallIndex; // who has the ball. set to hasBallIndex when throwing, pitching, or dropping
	// set to -1 when someone catches.
	int controlIndex; // index of the controlled catching team player
	int batterIndex; // index of batter.
	int changePlayerArrayIndex; // fielderRankedIndices[changePlayerArrayIndex]is currently selected
} PlayerIndexInfo;

// MILESTONE 7.5 - Focused Structs
typedef struct _GameState {
	int outs;
	int balls;
	int strikes;
	int runsInTheInning;
	GameEventType event;
	int outOfBounds; // Rule state: ball is out of bounds
	int ballHome;    // Rule state: ball is at home base
	int endPeriod;   // Rule state: period should end
} GameState;

typedef struct _GameControlFlags {
	int pause;
	int initLocals;
	int waitingForBatterDecision;
	int waitingForFreeWalkDecision;
	int freeWalkCalculationMade;
	int freeWalkIndex;
	int freeWalkBase;
	int checkForRun;
	int playerArrivedToBase;
	int firstCatchMade;
	int batterStartedRunning;
} GameControlFlags;

typedef struct _WoundingState {
	int woundingCatch;        // Pending wounding opportunity
	int woundingCatchHandled; // Has been processed
} WoundingState;

typedef struct _CameraState {
	int homeRunCameraFlag;
	Vector3D targetPoint; // For camera or AI focus
} CameraState;

typedef struct _PlayerCounters {
	int battingTeamPlayersOnFieldCount;
	int nonJokerPlayersLeft;
	int jokersLeft;
	int noMorePlayers;
} PlayerCounters;

typedef struct _GameModeState {
	int runnerBatterPairCounter;
	int canMakeRunOfHonor;
	int forceNextPair;
} GameModeState;

typedef enum {
	PITCH_STAGE_NONE = 0,
	PITCH_STAGE_WINDUP = 1,   // Pitcher winding up
	PITCH_STAGE_AIRBORNE = 2  // Ball in air towards plate
} PitchCycleState;

typedef struct _PlayerRelatedActionInfo {
	float meterValue; // meter for pitching and throwing
	float swingMeterValue; // meter for batting

	PitchCycleState pitchState; // Replaces pitchGoingOn and pitchInAir

	int throwGoingToBase; // to avoid moving basecatchers out in the wild when ball is thrown to them.

	int willStartRunning[BASE_COUNT];

	int initBatter; // used to initialize batter locally in
	int batterReady; // is batter ready to swing
	int batHit; // tells if bat hit the ball. set to back 0 again when next pitch starts
	int batMiss; // tells if bat missed the ball. set to back 0 when next pitch starts.
	int battingGoingOn; // time starts when batter reaches ready position and ball is not in air and quits when batting animation is over
	int batterCanAdvance;

	int refreshCatchAndChange; // when ball hits ground or stops etc we want to refresh changePlayerArrays.
	int initPlayerSelection; // and sometimes with refreshCatchAndChange we set also initPlayerSelection
	// so that

} PlayerRelatedActionInfo;

typedef struct _GlobalGameInfo {
	TeamInfo teams[2];
	int halfInningsInPeriod;
	int inning;
	int period;
	GamePeriodMode mode; // NEW field for Milestone 7
	int pairCount;
	int playsFirst;
	int isCupGame;
} GlobalGameInfo;

typedef struct _LocalGameInfo {
	PlayerInfo playerInfo[2*PLAYERS_IN_TEAM + JOKER_COUNT];
	PlayerRuntimeState playerRuntime[2*PLAYERS_IN_TEAM + JOKER_COUNT]; // Milestone 7.5 - Control state
	ActionFlags aF;
	PlayerIndexInfo pII;
	PlayerRelatedActionInfo pRAI;
	GameState gameState; // MILESTONE 7.5 - New core state
	GameControlFlags gameControl; // MILESTONE 7.5 - Implementation flags
	WoundingState woundingState; // MILESTONE 7.5 - Wounding system state
	CameraState cameraState; // MILESTONE 7.5 - Camera and UI state
	PlayerCounters playerCounters; // MILESTONE 7.5 - Player tracking
	GameModeState gameModeState; // MILESTONE 7.5 - Game mode specific state
	BallInfo ballInfo;

} LocalGameInfo;

typedef struct _GameConclusion {
	int winner;
	int period0Runs[2];
	int period1Runs[2];
	int period2Runs[2];
	int period3Runs[2];
	int isCupGame;
} GameConclusion;

// Forward declaration for the new Cup structure to avoid circular dependencies
typedef struct _Cup Cup;

typedef struct _StateInfo {
	// Main menu or game screen active
	ScreenState screen;
	int changeScreen;
	int updated;
	int numTeams;
	int playSoundEffect;
	int stopSoundEffect;
	int currently_played_cup_match_index; // Used to pass match context to/from game screen
	// addresses to important information structures
	KeyStates *keyStates;
	TeamData *teamData;
	FieldPositions *fieldPositions;
	LocalGameInfo* localGameInfo;
	GlobalGameInfo* globalGameInfo;
	Cup* cup;                  // New dynamic tournament state
	GameConclusion* gameConclusion;
} StateInfo;

#endif /* GLOBALS_H */
