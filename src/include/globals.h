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
#define PERSPECTIVE_ASPECT_RATIO (16.0f / 9.0f)
#define VIRTUAL_WIDTH 1920
#define VIRTUAL_HEIGHT 1080

// game states
typedef enum { SCREEN_LOADING, SCREEN_MAIN_MENU, SCREEN_GAME } ScreenState;

// NEW ENUMS FOR MILESTONE 7 (DATA RENAISSANCE)
// --------------------------------------------

typedef enum { PITCH_RESULT_NONE = 0, PITCH_RESULT_STRIKE = 1, PITCH_RESULT_BALL = 2 } PitchResult;

// --------------------------------------------

typedef enum {
    PLAYER_STATE_IDLE = 0,
    PLAYER_STATE_AT_BAT,
    PLAYER_STATE_ON_BASE, // Replaces isOnBase=1
    PLAYER_STATE_RUNNING, // Replaces isOnBase=0, out=0, wounded=0
    PLAYER_STATE_ADVANCING_FREELY, // Replaces takingFreeWalk=1
    PLAYER_STATE_LEADING, // Replaces leading=1
    PLAYER_STATE_OUT, // Replaces out=1
    PLAYER_STATE_WOUNDED, // Replaces wounded=1
    PLAYER_STATE_SCORED // Explicit state for having scored
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
    GAME_MODE_SUPER_INNING = 1, // Potential mapping for period 2/3 if distinct
    GAME_MODE_HOMERUN_CONTEST = 2 // Replaces period >= 4
} GamePeriodMode;

// --------------------------------------------

// a lot of constants
#define LIGHT_SOURCE_POSITION_X 30.0f
#define LIGHT_SOURCE_POSITION_Y 50.0f
#define LIGHT_SOURCE_POSITION_Z -50.0f

#define PLATE_WIDTH 1.5f
#define GRAVITY 0.003f
#define ZERO_BATTING_ANGLE (19 * PI / 16)
#define SHADOW_HEIGHT 0.01f

#define WALK_SPEED 0.06f
#define RUN_SPEED 0.12f
#define LEAD_STEP 0.05f
#define BATTING_TEAM_RUN_FACTOR 1.1f

#define GROUND_WIDTH 40.0f
#define GROUND_LENGTH 30.0f
#define GROUND_OFFSET_X (-GROUND_WIDTH / 34)
#define GROUND_OFFSET_Z (-GROUND_LENGTH / 6)
#define FIELD_BACK (-4.5f * GROUND_LENGTH + GROUND_OFFSET_Z)
#define FIELD_FRONT (1.5f * GROUND_LENGTH + GROUND_OFFSET_Z)
#define FIELD_LEFT (-2.5f * GROUND_WIDTH + GROUND_OFFSET_X)
#define FIELD_RIGHT (2.5f * GROUND_WIDTH + GROUND_OFFSET_X)
#define FENCE_OFFSET 0.5f

#define HOME_RADIUS 6.0f
#define BASE_RADIUS 2.0f
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

#define GROUND_UNIT_COUNT 30

typedef struct _GroundUnit {
    GLuint texture;
    int x;
    int y;
} GroundUnit;

#define DISTANCE_FROM_HOME_LOCATION_THRESHOLD 1.75f
#define TARGET_ACHIEVED_THRESHOLD                                                                                      \
    0.25f // has to have some room here because we are calculating velocity only at the start of movement and everything
          // approximations anyway
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
    int currentFlightHasHitGround; // has the ball hit ground
    int onGround; // is ball rolling on ground
    int hitsGroundToUnWound; // if ball hits ground after being catched as wounding catch, is set to 1. checked only
                             // after wounding catch so, it is set to 0 when the catch is made.
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
    PITCH_ACTION_START = 1,     // Intent: start windup
    PITCH_ACTION_POWER_WAIT = 2, // (unused after Phase B — state lives in pitchPhase)
    PITCH_ACTION_POWER_SET = 3, // Intent: power selected
    PITCH_ACTION_ANGLE_WAIT = 4, // (unused after Phase B — state lives in pitchPhase)
    PITCH_ACTION_ANGLE_SET = 5  // Intent: angle selected, release ball
} PitchActionPhase;

typedef enum {
    BAT_ACTION_IDLE = 0,
    BAT_ACTION_WAIT_FOR_BALL = 1, // Active: batter ready, ball in air
    BAT_ACTION_POWER_SET = 2, // Trigger: swing button pressed (select power)
    BAT_ACTION_ANGLE_WAIT = 3, // Active: swinging, waiting for angle select
    BAT_ACTION_ANGLE_SET = 4, // Trigger: angle selected
    BAT_ACTION_DONE = 5 // Active: animation finishing
} BatActionPhase;

typedef enum { CHOOSE_BATTER_IDLE = 0, CHOOSE_BATTER_NEXT = 1, CHOOSE_BATTER_SELECT = 2 } ChooseBatterAction;

typedef enum { FREE_WALK_IDLE = 0, FREE_WALK_ACCEPT = 1, FREE_WALK_REJECT = 2 } FreeWalkAction;

/*
Action flags. Set in action_invocations.c (human input) or AI, consumed by execute_actions.c.
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
    ActionTriggerState change_player;
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
    int arrivedToBase; // Optimization flag
    int passedPathPoint; // State machine variable
    int goingForward; // Direction tracking
    int hasMadeRunOnThirdBase; // Guard flag
} PlayerRuntimeState;

// Flow control state machine enums (used by RefereeState)
typedef enum {
    FOUL_STATE_NONE = 0,
    FOUL_STATE_DETECTED = 1, // Ball hit ground out of bounds, waiting for grace period
    FOUL_STATE_RESETTING = 2 // Grace period over, performing reset this frame
} FoulPlayState;

typedef enum { HR_PAIR_STATE_NONE = 0, HR_PAIR_STATE_DETECTED = 1, HR_PAIR_STATE_RESETTING = 2 } HomeRunPairState;

typedef enum {
    END_INNING_STATE_NONE = 0,
    END_INNING_STATE_DETECTED = 1,
    END_INNING_STATE_RESETTING = 2
} EndOfInningTransitionState;

// Period transition result — set by referee at DETECTED→RESETTING,
// read by consolidation to determine menu/screen routing.
typedef enum {
    PERIOD_TRANSITION_NONE = 0, // Normal next-inning (no menu, just physical reset)
    PERIOD_TRANSITION_INTER_PERIOD, // Period 0→1 transition
    PERIOD_TRANSITION_SUPER_INNING, // Period 1→2 transition (super inning)
    PERIOD_TRANSITION_HOMERUN_CONTEST, // Period 2→4 or round→round+2
    PERIOD_TRANSITION_GAME_OVER // Game ended (winner stored separately)
} PeriodTransitionType;

// Player status enum - replaces flag-based status tracking
typedef enum {
    PLAYER_STATUS_ACTIVE = 0, // Playing normally, no special status
    PLAYER_STATUS_WOUND_MARKED = 1, // Vulnerable: evaluation timer running, can cancel if ball drops (normal wound)
    PLAYER_STATUS_WOUND_MARKED_DOUBLE = 2, // Vulnerable: evaluation timer running (tuplahaava/double wound)
    PLAYER_STATUS_WOUND_PENDING = 3, // DOOMED (normal wound): must advance, will get WOUNDED or OUT
    PLAYER_STATUS_WOUND_PENDING_DOUBLE = 4, // DOOMED (tuplahaava): can retreat to avoid OUT
    PLAYER_STATUS_WOUNDED = 5, // TERMINAL: Out of play, returns to home base
    PLAYER_STATUS_OUT = 6 // TERMINAL: Out of play, returns to home base
} RefereePlayerStatus;

typedef struct _RefereePlayerState {
    // === PRIMARY STATUS ===
    RefereePlayerStatus status; // Mutually exclusive player status (wounding/out lifecycle)

    // === PITCH START SNAPSHOT (for foul play) ===
    BaseID baseAtPitchStart; // Where was player when pitch started

    // === CURRENT SAFETY STATUS ===
    BaseID currentSafetyBase; // Which base has their safety (-1 if none)
    int hasScored; // Logical scored status (decided by Referee)
    int runOfHonorScored; // Logical run of honor scored status (decided by Referee)

    // === EVENT SNAPSHOTS (updated at key moments) ===
    BaseID baseAtLastEvent; // Where they were at last important event
    int hadSafetyAtLastEvent; // Did they have safety at last event

    // === PENDING RUNS (Milestone 17) ===
    int hasPendingRun; // Arrived at home while ball in air
    int hasPendingRunOfHonor; // Arrived at 3rd (homerun) while ball in air

} RefereePlayerState;

typedef struct _RefereeState {
    // Per-player tracking
    RefereePlayerState battingPlayers[PLAYERS_IN_TEAM + JOKER_COUNT];

    // Global events
    int strikesAtPitchStart; // Snapshot of strikes when pitch started

    // Wounding evaluation state (Referee monitors and decides)
    int woundingEvaluationActive; // Referee is evaluating a potential wounding catch
    int woundingEvaluationFinished; // Milestone 17: Flag set when evaluation completes (catch confirmed)
    int woundingEvaluationTimer; // Timer counting up during evaluation period

    // Flow Control State Machines (Milestone 17 consolidation)
    FoulPlayState foulState; // Out-of-bounds transition state
    int foulTimer; // Timer for foul play grace period

    HomeRunPairState nextPairTransitionState; // Next-pair transition state
    int nextPairTimer; // Timer for next-pair transition

    EndOfInningTransitionState endOfInningState; // End-of-inning transition state
    int endInningTimer; // Timer for end-of-inning transition
    PeriodTransitionType periodTransition; // Result of period logic (set at DETECTED→RESETTING)
    int periodTransitionWinner; // Winner index (valid when periodTransition == GAME_OVER)

    // Run of Honor tracking (Homerun Contest)
    int ballInThirdBaseSincePitch; // Has ball been held at 3rd base by catching team since pitch started

    int homerunPairHasPitch; // Has at least one pitch been released for current pair
} RefereeState;

typedef enum { JOKER_REGULAR = 0, JOKER_AVAILABLE = 1, JOKER_USED = 2 } JokerStatus;

// batting team related flags.
typedef struct _BattingTeamPlayerInfo {
    char* name; // some info to be shown in screen to guide player
    int speed; // used to make some player a bit faster than others
    int power; // used to make some players bat a bit harder than others
    int number; // number is shown on screen
    JokerStatus joker; // 0 regular player, 1 has right to be used, 2 has been used already

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

typedef enum { CONTROL_PLAYER_1 = 0, CONTROL_PLAYER_2 = 1, CONTROL_AI = 2 } TeamControlMode;

typedef enum { TEAM_BATTING = 0, TEAM_CATCHING = 1 } TeamSide;

typedef enum { REPLACEMENT_IDLE = 0, REPLACEMENT_ACTIVE = 1 } ReplacementState;

typedef struct _CommonPlayerInfo {
    // player information. name is shown on the screen sometimes, team is used in players.c
    // and stats are used to make players behave in different ways on some situations.
    TeamSide team;

    int moving; // 0 doesnt, 1 moves
    int running; // 0 doesnt, 1 runs
    int looksForTarget; // used in conjuction with targetLocation when trying to go catching a ball for example
    int lastLastLocationUpdate; // player needs to have his lastLocation updated, necessary when controlling player and
                                // moving stops. twitching without this
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
    ReplacementState
        replacingStage; // 1 is going to replace or is at the base, 0 is coming back or is at home location.
    BaseID replacingBase; // in which base is the player replacing
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
    TeamControlMode control; // who controls, 0 player 1, 1, player 2, 2 AI
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
    int changePlayerArrayIndex; // fielderRankedIndices[changePlayerArrayIndex]is currently selected
} PlayerIndexInfo;

// MILESTONE 7.5 - Focused Structs
typedef struct _HalfInningState {
    int outs;
    int balls;
    int strikes;
    int runsInTheInning;
    GameEventType event;
    int endPeriod; // Rule state: period should end
} HalfInningState;

// MILESTONE 16 (Phase 1): Transient event notifications
// Events are set this frame, cleared next frame
// Written by: execute_actions.c, game_manipulation.c
// Read by: referee.c, others
typedef struct _GameEvents {
    // Event flags (cleared each frame)
    int catchMade; // Fly ball was caught
    int playerArrivedAtBase; // Runner arrived at base

    // Future events (not yet used)
    int pitchReleased; // Ball left pitcher's hand
    int ballHitByBat; // Bat made contact
    int ballMissedByBat; // Bat swung and missed
    int ballHitGround; // Ball touched the ground this frame
    int freeWalkAccepted; // Player accepted free walk
    int freeWalkRejected; // Player rejected free walk
    int batterEntered; // New: Signal that new batter is ready
} GameEvents;

// Batting outcome for the current pitch (promoted from GameEvents by referee)
typedef enum {
    BAT_OUTCOME_NONE, // No batting outcome yet
    BAT_OUTCOME_HIT, // Bat made contact with ball
    BAT_OUTCOME_MISSED // Batter swung and missed
} BatOutcome;

// MILESTONE 17: Between-pitch state (reset at pitch start, written by referee)
// Sticky flags that persist across frames but reset when new pitch starts
typedef struct _BetweenPitchState {
    int catchHasBeenMade; // Fly ball was caught (promoted from catchMade)
    int hasBallHitGround; // Ball has touched ground (promoted from ballHitGround)
    PitchResult pitchResult; // Referee's pitch adjudication (NONE until resolved)
    BatOutcome batOutcome; // Bat hit or missed (promoted from ballHitByBat/ballMissedByBat)
} BetweenPitchState;

// MILESTONE 17: User interaction flow control
typedef struct _FlowControl {
    int pause;
    int waitingForBatterDecision;
    int waitingForFreeWalkDecision;

    // Free walk offer data (calculated by game_analysis, consumed by UI/actions)
    int freeWalkCalculationMade;
    int freeWalkIndex; // Which player to offer free walk (-1 = none)
    BaseID freeWalkBase; // From which base (BASE_NONE = none)
} FlowControl;

typedef struct _CameraState {
    int homeRunCameraFlag;
    Vector3D targetPoint; // For camera or AI focus
    int homeRunCameraCounter;

    // View vectors
    Vector3D cam, look, up;
    Vector3D statCam, statLook, statUp;
    Vector3D skyBoxCam, skyBoxLook;

    // Interpolation
    Vector3D lastCamTargetLocation;
    Vector3D lastCamLocation;
    Vector3D camLocation;
    Vector3D camTargetLocation;

    // Misc
    float lightPos[4];
} CameraState;

typedef struct _UIState {
    int gameInfoEventTimer;
    int gameInfoEvent;
    float lastMeterX;
    float lastSwingMeterX;
} UIState;

typedef struct _PlayerCounters {
    int nonJokerPlayersLeft;
    int jokersLeft;
    int noMorePlayers;
} PlayerCounters;

typedef enum {
    AI_NO_LOCK = -1,
    AI_PITCH_LOCK = 0,
    AI_THROW_LOCK = 1,
    AI_DROP_LOCK = 2,
    AI_WAITING_BATTER_LOCK = 3,
    AI_WAITING_WALK_LOCK = 4,
    AI_BATTING_LOCK = 5,
    AI_CHANGE_LOCK = 6,
    AI_CLICK_LOCK = 7,
    AI_DOUBLE_CLICK_LOCK = 8,
    AI_COME_BACK_LOCK = 9,
    AI_COME_BACK_WRONG_PITCH_LOCK = 10
} AILockType;

typedef enum { BATTING_MODE_SWING = 0, BATTING_MODE_BUNT = 1, BATTING_MODE_STOP = 2 } BattingMode;

typedef struct _AIState {
    // Catching AI
    int moveCounter;
    int throwStage;
    int dropStage;

    // Batting AI
    int battingKeyDown;
    AILockType actionKeyLock;
    int changingKeyDown;
    int increaseKeyDown;
    int decreaseKeyDown;
    int angleDecided;
    float decidedAngle;
    int baseRunnerKeyDown[BASE_COUNT];
    int baseRunnerDecisionMade[BASE_COUNT];
    int lastSafeOnBaseIndex[BASE_COUNT];
    AILockType baseRunnerLock[BASE_COUNT];
    int amountOfClicks[BASE_COUNT];
    int clickBreak[BASE_COUNT];
    int battingStyle;
    int runningBatter;
    int runningBaseRunners;
    int planCalculated;
    int firstIndex;
    int firstIndexSelected;
    int change;
    int changeHasHappened;

    // Pitching AI
    int pitchStage;
    unsigned int pitchFirstLimit;
    unsigned int pitchSecondLimit;
    int pitchTime;
    int pitchPreviousTime;
    int batterReadyTimer;
    int aiWrongPitch;
} AIState;

typedef struct _GameFlowState {
    int closeToGround;
    int homeRunCameraCounter;
    int ballHome; // Moved from HalfInningState (Logic state: ball is at home base)
} GameFlowState;

typedef enum {
    CATCHING_ACTION_NONE = 0,
    CATCHING_ACTION_THROWING,
    CATCHING_ACTION_PITCHING,
    CATCHING_ACTION_DROPPING,
    CATCHING_ACTION_CHANGING
} CatchingTeamCurrentAction;

typedef enum {
    PITCH_PHASE_NONE = 0,
    PITCH_PHASE_POWER_WAIT,
    PITCH_PHASE_ANGLE_WAIT
} PitchPhase;

typedef struct _PendingActionState {
    unsigned int meterCounter;
    unsigned int meterCounterMax;

    CatchingTeamCurrentAction currentCatchingAction;
    PitchPhase pitchPhase;

    int throwGoingOn;
    int runBatFlag;
    int batterSelect;

    AILockType aiActionEventLock;
    int aiLockUpdate;
    int aiLockTimeoutCounter;

    // Batting related
    int battingFrameCount;
    int increaseBattingFrameCount;
    int selectedBattingPowerCount;
    int selectedBattingAngleCount;
    float batterAngle;
    int batterAngleSpeed;
    float batterAdvanceSpeed;
    float batterAdvance;
    BattingMode battingMode;
    float batterAdvanceLimit;
    int battingStopped;
    int batterMoving;
    int updateBatterLocationAndOrientation;
    int pitchFrameTime; // from batting_system.c

    // Throwing related
    float throwDistance; // from throwing_system.c
    Vector3D throwDirection; // from throwing_system.c

    // Pitching related
    float pitchPower; // from pitching_system.c

    // Input interpretation
    int doubleClickCounter[BASE_COUNT];
} PendingActionState;

typedef struct _HomeRunContestState {
    int runnerBatterPairCounter;
} HomeRunContestState;

typedef enum {
    PITCH_STAGE_NONE = 0,
    PITCH_STAGE_WINDUP = 1, // Pitcher winding up
    PITCH_STAGE_AIRBORNE = 2 // Ball in air towards plate
} PitchCycleState;

typedef struct _PlayerRelatedActionInfo {
    float meterValue; // meter for pitching and throwing
    float swingMeterValue; // meter for batting

    PitchCycleState pitchState; // Replaces pitchGoingOn and pitchInAir

    int throwGoingToBase; // to avoid moving basecatchers out in the wild when ball is thrown to them.

    int willStartRunning[BASE_COUNT];

    int initBatter; // used to initialize batter locally in
    int batterReady; // is batter ready to swing
    int battingGoingOn; // time starts when batter reaches ready position and ball is not in air and quits when batting
                        // animation is over
    int batterCanAdvance;

    int refreshCatchAndChange; // when ball hits ground or stops etc we want to refresh changePlayerArrays.
    int initPlayerSelection; // and sometimes with refreshCatchAndChange we set also initPlayerSelection
    // so that

} PlayerRelatedActionInfo;

typedef struct _Scoreboard {
    TeamInfo teams[2];
    int halfInningsInPeriod;
    int inning;
    int period;
    GamePeriodMode mode; // NEW field for Milestone 7
    int pairCount;
    int playsFirst;
    int isCupGame;
} Scoreboard;

// GameRulesState — Referee-owned state, structurally separated from physical/action state.
// Only the referee writes these at runtime. Other stages receive const pointers.
typedef struct _GameRulesState {
    RefereeState referee;
    HalfInningState halfInningState;
    BetweenPitchState betweenPitchState;
    PlayerCounters playerCounters;
    HomeRunContestState homeRunContestState;
    Scoreboard scoreboard;
} GameRulesState;

typedef struct _MatchSession {
    PlayerInfo playerInfo[2 * PLAYERS_IN_TEAM + JOKER_COUNT];
    PlayerRuntimeState playerRuntime[2 * PLAYERS_IN_TEAM + JOKER_COUNT]; // Milestone 7.5 - Control state
    ActionFlags aF;
    PlayerIndexInfo pII;
    PlayerRelatedActionInfo pRAI;
    GameEvents gameEvents; // MILESTONE 16 - Event notifications (Phase 1)
    FlowControl flowControl; // MILESTONE 17 - User flow gates
    CameraState cameraState; // MILESTONE 7.5 - Camera and UI state
    PendingActionState pendingActionState; // Milestone 10 - Action globals
    AIState aiState; // Milestone 11 - AI globals
    GameFlowState gameFlowState; // Milestone 11 - Game flow globals
    UIState uiState; // Milestone 11 - UI state
    GroundUnit groundUnit[GROUND_UNIT_COUNT];
    BallInfo ballInfo;

} MatchSession;

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
    int soundEnabled;
    int currently_played_cup_match_index; // Used to pass match context to/from game screen
    // addresses to important information structures
    KeyStates* keyStates;
    TeamData* teamData;
    FieldPositions* fieldPositions;
    MatchSession* match;
    GameRulesState* rules;
    Cup* cup; // New dynamic tournament state
    GameConclusion* gameConclusion;
} StateInfo;

#endif /* GLOBALS_H */
