#ifndef ACTION_IMPLEMENTATION_INTERNAL_H
#define ACTION_IMPLEMENTATION_INTERNAL_H

extern StateInfo stateInfo;

// here some constants used in the code
#define PITCH_BASE_SPEED 0.065f 
#define PITCHER_MOVE_AWAY_OFFSET 0.1f + DISTANCE_FROM_HOME_LOCATION_THRESHOLD + TARGET_ACHIEVED_THRESHOLD
#define PITCH_POWER_CONSTANT 0.12f
#define PITCH_ANGLE_CONSTANT 0.15f
#define THROW_TO_BASE_DISTANCE 1.0f
#define THROW_POWER_CONSTANT 0.65f
#define THROW_DISTANCE_CONSTANT 0.0012f
#define THROW_MAX 50
#define BATTER_ANGLE_SPEED_CONSTANT 0.02f
#define BATTER_ANGLE_LIMIT PI/7
#define GENERIC_BATTER_ADVANCE_SPEED_CONSTANT 0.7f

#define PITCH_FRAME_TIME_TWEAK 3
#define DROP_BALL_CONSTANT 0.02f
#define PITCH_DOWN_MAX 9
#define PITCH_UP_MAX 13
#define ANIMATION_FREQUENCY 3
#define BAT_LOAD_MAX (4*9) 
#define BAT_SWING_MAX (4*13) 
#define BAT_ANIMATION_FRAME_HIT_COUNT (20*ANIMATION_FREQUENCY)
#define BAT_ANIMATION_FRAME_TOTAL_COUNT (34*ANIMATION_FREQUENCY)
#define BALL_MAX_OFFSET 1.0f
#define BUNT_THRESHOLD 20
#define VERTICAL_ANGLE_LIMIT 5

#define BUNT_ADVANCE 1.0f
#define SWING_ADVANCE 0.5f
#define SPREAD_ADVANCE 0.3f

#define BATTER_ANGLE_FIX (2*PI / 4)

// ai action flags
#define AI_NO_LOCK -1
#define AI_PITCH_LOCK 0
#define AI_THROW_LOCK 1
#define AI_DROP_LOCK 2

#define AI_WAITING_BATTER_LOCK 3
#define AI_WAITING_WALK_LOCK 4
#define AI_BATTING_LOCK 5
#define AI_CHANGE_LOCK 6

#define AI_CLICK_LOCK 7
#define AI_DOUBLE_CLICK_LOCK 8
#define AI_COME_BACK_LOCK 9
#define AI_COME_BACK_WRONG_PITCH_LOCK 10

#define TIMEOUT_CONSTANT 200

#define CLICK_BREAK_CONSTANT 3


// some static variables that are used only in action_implementation.c
// how they work is explained where they are needed, so if interested should check
// the code.
static float throwDistance;
static Vector3D throwDirection;

static unsigned int meterCounter;
static unsigned int meterCounterMax;

static int doubleClickCounter[BASE_COUNT];
static int pitchFrameTime;
static float pitchPower;
static int batterSelect;
static int battingFrameCount;
static int increaseBattingFrameCount;
static int selectedBattingPowerCount;
static int selectedBattingAngleCount;
static float batterAngle;
static int batterAngleSpeed;
static float batterAdvanceSpeed;
static float batterAdvance;
static int battingMode;
static float batterAdvanceLimit;
static int battingStopped;
static int batterMoving;
static int updateBatterLocationAndOrientation;
static int throwGoingOn; // to ensure that no throws going different directions at the same time and that throwing player's orientation changes correctly
static int runBatFlag;
// ai

static int aiPitchStage;
static int aiDropStage;
static unsigned int aiPitchFirstLimit;
static unsigned int aiPitchSecondLimit;
static int aiThrowStage;
static int aiActionEventLock;
static int aiLockUpdate;
static int aiMoveCounter;
// we need timeouts as sometimes ai tries to do somthing too quickly before the other 
// action implementation machinery is ready for it
static int aiLockTimeoutCounter;
static int aiPitchTime;
static int aiPitchPreviousTime;
static int aiBatterReadyTimer;

// batting team ai

static int aiBattingKeyDown;
static int aiActionKeyLock;

static int aiChangingKeyDown;

static int aiIncreaseKeyDown;
static int aiDecreaseKeyDown;
static int aiAngleDecided;
static float aiDecidedAngle;

static int aiWrongPitch;

static int aiBaseRunnerKeyDown[BASE_COUNT];
static int aiBaseRunnerDecisionMade[BASE_COUNT];
static int aiLastSafeOnBaseIndex[BASE_COUNT];
static int aiBaseRunnerLock[BASE_COUNT];
static int aiAmountOfClicks[BASE_COUNT];
static int aiClickBreak[BASE_COUNT];

static int aiBattingStyle;
static int aiRunningBatter;
static int aiRunningBaseRunners;

static int aiPlanCalculated;
static int aiFirstIndex;
static int aiFirstIndexSelected;
static int aiChange;
static int aiChangeHasHappened;

// and here are the functions. __inline as __inline is :3
static __inline void genericThrowRelease();
static __inline void genericThrowLoad(int base);
static __inline void genericMove(int direction);
static __inline void genericStopMove(int direction);
static __inline void genericSlingBall(float x, float y, float z);
static __inline void dropBall();
static __inline void updateControlledPlayerSpeed();
static __inline void startPitch();
static __inline void continuePitch();
static __inline void releasePitch();
static __inline void changeBatter();
static __inline void selectBatter();
static __inline void takeFreeWalkDecision();
static __inline void updateBatting();
static __inline void startIncreaseBatterAngle();
static __inline void stopIncreaseBatterAngle();
static __inline void startDecreaseBatterAngle();
static __inline void stopDecreaseBatterAngle();
static __inline void selectPower();
static __inline void selectAngle();
static __inline void baseRun(int base);

static __inline void updateMeters();
// ai
static __inline void aiLogic();
static __inline void moveControlledPlayerToLocation(Vector3D* target);
static __inline void flushKeys();
static __inline void throwBallToBase(int base);

#endif /* ACTION_IMPLEMENTATION_INTERNAL_H */