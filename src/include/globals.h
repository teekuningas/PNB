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

// game states
#define MAIN_MENU 0
#define GAME_SCREEN 1

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
/*
Action flags. used in action_invocation.c and action_implementation.c.
flag is set when key event happens in action_invocation, then its set off or modified when its handled in
action_implementation
*/
typedef struct _BattingTeamActionFlags {
	int baseRun[4];
	int chooseBatter;
	int takeFreeWalk;
	int swing;
	int increaseBatterAngle;
	int decreaseBatterAngle;
} BattingTeamActionFlags;

typedef struct _CatchingTeamActionFlags {
	int move[4];
	int throwToBase[4];
	int changePlayer;
	int run;
	int dropBall;
	int pitch;
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
// batting team related flags.
typedef struct _BattingTeamPlayerInfo {
	char* name; // some info to be shown in screen to guide player
	int speed; // used to make some player a bit faster than others
	int power; // used to make some players bat a bit harder than others
	int number; // number is shown on screen
	int base; // base is defined as the base you are on, or the base you were last if you arent on any base now
	int originalBase; // base when pitch started
	int joker; // 0 original player, 1 has right to be used, 2 has been used already
	int arrivedToBase; // set to 1 when player arrives to base, purpose to reduce overhead
	int wounded; // set to 1 when ball is caught and player is no in his original base
	int woundedApply;
	int takingFreeWalk; // 1 when taking a free walk and is inbetween the bases
	int out; // if out of made, this will be set to player
	int passedPathPoint; // path point used in walking players home and running from third base to home.
	int goingForward; // is the player running forward or not. used to determine actions when arriving a base
	int isOnBase; // is the player at a base right now
	int leading; // is player leading
	int hasMadeRunOnThirdBase; // cant make a run always when checkForRun flag is set.
} BattingTeamPlayerInfo;

typedef struct _CommonPlayerInfo {
	// player information. name is shown on the screen sometimes, team is used in players.c
	// and stats are used to make players behave in different ways on some situations.
	int team;

	int moving; // 0 doesnt, 1 moves
	int running; // 0 doesnt, 1 runs
	int looksForTarget; // used in conjuction with targetLocation when trying to go catching a ball for example
	int lastLastLocationUpdate; // player needs to have his lastLocation updated, necessary when controlling player and moving stops. twitching without this
	/* how to intepret the model number:
		0 standing_without_ball
		1 standing_with_ball
		2 walking_without_ball
		3 walking_with_ball
		4 running_without_ball
		5 running_with_ball
		6 pitching part1
		7 pitching part2
		8 throwing part1
		9 throwing part2
		10 standing_bare_hands
	    11 walking_bare_hands
		12 running_bare_hands
		13 batter ready
		14 batting v1
		15 batting v2
		16 batting v3
	*/
	int model;
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

typedef struct _GameAnalysisInfo {
	int battingTeamPlayersOnFieldCount; // how many batting team players on field
	int nonJokerPlayersLeft; // number of nonJokers to be used for batting in this inning
	int jokersLeft; // jokers to be used for batting in this inning

	int outs; // how many outs has batting team
	int balls; // how many balls have been pitched for current batter
	int strikes; // how many strikes for current batter
	int runsInTheInning; // runs in this inning. used to give new rounds of batters.

	int freeWalkCalculationMade; // freeWalk calculation done only at the moment when ball is caught.
	int waitingForBatterDecision; // user is prompted for batter decision
	int waitingForFreeWalkDecision; // user is prompted for free walk decision
	int outOfBounds; // if ball has landed out of bounds, this is 1
	int noMorePlayers; // if no more players, we can end the inning
	int ballHome; // in that case ball also needed to be at homebase
	int endPeriod;
	int woundingCatch; // if some conditions hold, we will have a situation where we can check
	// if there are players not on their original bases that we could wound.
	int woundingCatchHandled;
	int batterStartedRunning; // used to help AI to drop ball sometimes
	int gameInfoEvent; // used to give information for user about the events in game.
	// check game_screen.c for more information
	int checkForRun; // if player arrives homebase or third base after running through all the base on same pitch
	// we can check if the run is valid. it is decided when ball lands.
	int freeWalkIndex; // index used in free walk decision
	int freeWalkBase; // base for the that player

	int playerArrivedToBase; // if player arrives base we put this flag, so that we dont do useless
	// base-player-checking when nothing has changed.
	int firstCatchMade; // first catch made. many decisions depend on knowledge of ball landing. but it is
	// often ok if ball doesn hit ground but is caught instead.
	int initLocals; // when foul play and when starting the game we need to init local variables of
	// game analysis and action implementation
	int runnerBatterPairCounter; // what pair to be used in the homerun-batting contest
	int canMakeRunOfHonor;
	int forceNextPair;

	int homeRunCameraFlag;
	Vector3D targetPoint; // point where players start to move when busy catchin'

	int pause;

} GameAnalysisInfo;

typedef struct _PlayerRelatedActionInfo {
	float meterValue; // meter for pitching and throwing
	float swingMeterValue; // meter for batting

	int pitchGoingOn; // sets to 1 when pitcher starts to crouch and goes to 0 when bat hits or ball hits ground.
	int pitchInAir; // this goes to 1 when ball is released from hand and goes to 0 in same spots as last one

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
	int inningsInPeriod;
	int inning;
	int period;
	int pairCount;
	int playsFirst;
	int winner;
} GlobalGameInfo;

typedef struct _LocalGameInfo {
	PlayerInfo playerInfo[2*PLAYERS_IN_TEAM + JOKER_COUNT];
	ActionFlags aF;
	PlayerIndexInfo pII;
	PlayerRelatedActionInfo pRAI;
	GameAnalysisInfo gAI;
	BallInfo ballInfo;


} LocalGameInfo;




typedef struct _CupInfo {
	int inningCount;
	int gameStructure;
	int dayCount;
	int userTeamIndexInTree; // in cupTeamIndexTree the element that is the position of user's team right now.
	int cupTeamIndexTree[14]; // indices to teamData-array, -1 means no team in this slot yet
	int slotWins[14];
	int schedule[4][2]; // indices to cupTeamIndexTree and slotWins. -1:s in the end mean no match for them.
	int winnerIndex;
} CupInfo;

typedef struct _StateInfo {
	// Main menu or game screen active
	int screen;
	int changeScreen;
	int updated;
	int numTeams;
	int playSoundEffect;
	// addresses to important information structures
	KeyStates *keyStates;
	TeamData *teamData;
	FieldPositions *fieldPositions;
	LocalGameInfo* localGameInfo;
	GlobalGameInfo* globalGameInfo;
} StateInfo;

#endif /* GLOBALS_H */
