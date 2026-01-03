# The Vision: Data Structure Refactoring

**Date:** 2026-01-03
**Status:** IMPLEMENTED ✅

This document visualizes the transformation of the game's core data structures.

---

## 1. The Player: Separating "Who" from "How"

We have successfully separated **Domain Identity** (Serializable, "Real") from **Runtime Control** (Ephemeral, Implementation Detail).

**Domain Object (Clean)**
*Can be passed to UI, Rules, and Save System.*
```c
typedef struct _BattingTeamPlayerInfo {
    char* name;
    int speed;
    int power;
    int number;
    int originalBase;
    int joker;

    // The "Truth" of the game
    PlayerUnitState state;
    BaseID baseId;
} BattingTeamPlayerInfo;
```

**Control Object (Hidden)**
*Only needed by Update/Movement logic. Contained in `LocalGameInfo.playerRuntime`.*
```c
typedef struct _PlayerRuntimeState {
	int arrivedToBase;       // Optimization flag
	int woundedApply;        // Deferred execution
	int passedPathPoint;     // State machine variable
	int goingForward;        // Direction tracking
	int hasMadeRunOnThirdBase; // Guard flag
} PlayerRuntimeState;
```

---

## 2. The Game State: Exploding the God Object

`GameAnalysisInfo` (34 mixed fields) has been broken into semantic components.

**1. GameState (The "Scoreboard")**
*Pure input for Rules Engine. Output of Referee.*
```c
typedef struct _GameState {
	int outs;
	int balls;
	int strikes;
	int runsInTheInning;
	GameEventType event;
	int outOfBounds; 
	int ballHome;    
	int endPeriod;   
} GameState;
```

**2. GameControlFlags (The "Engine Room")**
*Internal loop management.*
```c
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
```

**3. Subsystem States**
```c
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
```

---

## 3. The Grand Assembly: LocalGameInfo

The current state of the main container.

```c
typedef struct _LocalGameInfo {
	PlayerInfo playerInfo[...];
	PlayerRuntimeState playerRuntime[...]; // Parallel Array
	ActionFlags aF;
	PlayerIndexInfo pII;
	PlayerRelatedActionInfo pRAI;
	
	// NEW FOCUSED STATE
	GameState gameState;
	GameControlFlags gameControl;
	WoundingState woundingState;
	CameraState cameraState;
	PlayerCounters playerCounters;
	GameModeState gameModeState;
	
	BallInfo ballInfo;
} LocalGameInfo;
```

---

## 4. The Payoff: Next Steps (Refining Signatures)

This structure now allows us to rewrite logic functions with restricted scope.

**Example: Rules Engine**
Instead of `void check_outs(StateInfo* state)`, we can now have:
`void update_out_logic(GameState* gameState, PlayerInfo* players, ...)`

This is the foundation for the **Referee Pattern** in Milestone 8.