# The Vision: Data Structure Refactoring

**Date:** 2026-01-02
**Status:** Vision/Draft

This document visualizes the transformation of the game's core data structures.

---

## 1. The Player: Separating "Who" from "How"

We separate **Domain Identity** (Serializable, "Real") from **Runtime Control** (Ephemeral, Implementation Detail).

### BEFORE
```c
typedef struct _BattingTeamPlayerInfo {
    // Mixed Concerns!
    char* name;             // Domain
    int speed;              // Domain
    PlayerUnitState state;  // Domain
    BaseID baseId;          // Domain
    
    int arrivedToBase;      // Control (Optimization)
    int woundedApply;       // Control (Logic Queue)
    int passedPathPoint;    // Control (Pathfinding)
    int goingForward;       // Control (Movement)
} BattingTeamPlayerInfo;
```

### AFTER

**Domain Object (Clean)**
*Can be passed to UI, Rules, and Save System.*
```c
typedef struct _BattingTeamPlayerInfo {
    char* name;
    int speed;
    int power;
    int number;
    int joker;
    
    // The "Truth" of the game
    PlayerUnitState state;
    BaseID baseId;
    int originalBase;
} BattingTeamPlayerInfo;
```

**Control Object (Hidden)**
*Only needed by Update/Movement logic.*
```c
typedef struct _PlayerRuntimeState {
    int arrivedToBase;        // Optimization flag
    int woundedApply;         // Deferred wounding
    int passedPathPoint;      // Path index
    int goingForward;         // Direction state
    int hasMadeRunOnThirdBase;// Logic guard
} PlayerRuntimeState;
```

---

## 2. The Game State: Exploding the God Object

We break `GameAnalysisInfo` (34 mixed fields) into semantic components.

### BEFORE
```c
typedef struct _GameAnalysisInfo {
    int outs; int strikes; int balls;        // Rules
    int pause; int endPeriod;                // Control
    int ballHome; int outOfBounds;           // Physics
    int woundingCatch;                       // Specific System
    int homeRunCameraFlag;                   // Camera
    // ... + 25 more
} GameAnalysisInfo;
```

### AFTER

**1. GameState (The "Scoreboard")**
*Pure input for Rules Engine. Output of Referee.*
```c
typedef struct _GameState {
    int outs;
    int strikes;
    int balls;
    int runsInTheInning;
    GameEventType lastEvent;  // e.g., EVENT_OUT, EVENT_RUN
} GameState;
```

**2. GameControlFlags (The "Engine Room")**
*Internal loop management.*
```c
typedef struct _GameControlFlags {
    int pause;
    int endPeriod;
    int noMorePlayers;
    int initLocals;
    int waitingForBatterDecision;
    int waitingForFreeWalkDecision;
} GameControlFlags;
```

**3. WorldSensors (The "Eyes")**
*Physics results used by logic.*
```c
typedef struct _WorldSensors {
    int ballAtHomePlate;      // was ballHome
    int ballOutOfBounds;      // was outOfBounds
    int firstCatchMade;
} WorldSensors;
```

**4. Subsystem States**
```c
typedef struct _WoundingState {
    int pendingCatch;         // was woundingCatch
    int handled;              // was woundingCatchHandled
} WoundingState;

typedef struct _CameraState {
    int homeRunMode;          // was homeRunCameraFlag
    Vector3D focusTarget;     // was targetPoint
} CameraState;
```

---

## 3. The Grand Assembly: LocalGameInfo

How it all fits together in the main state container.

### BEFORE
```c
typedef struct _LocalGameInfo {
    PlayerInfo playerInfo[...]; // Polluted inside
    GameAnalysisInfo gAI;       // The God Object
    // ...
} LocalGameInfo;
```

### AFTER
```c
typedef struct _LocalGameInfo {
    // --- ENTITIES ---
    // Domain Data (Who they are)
    PlayerInfo playerInfo[...]; 
    
    // Runtime Data (What they are doing - Parallel Array)
    PlayerRuntimeState playerRuntime[...]; 

    // --- GAME LOGIC ---
    // The "Truth" (Rules engine operates on this)
    GameState gameState;

    // --- ENGINE SUPPORT ---
    // Control Flow
    GameControlFlags control;
    
    // Physics/World Sensing
    WorldSensors sensors;

    // Subsystem specific
    WoundingState wounding;
    CameraState camera;
    PlayerCounters counters;

    // --- LEGACY (To be refactored later) ---
    ActionFlags aF;
    PlayerIndexInfo pII;
    PlayerRelatedActionInfo pRAI;
    BallInfo ballInfo;
} LocalGameInfo;
```

---

## 4. The Payoff: Function Signatures

This structure allows us to write **Pure Functions** with **Specific Scope**.

**Bad (Current):**
```c
// Needs the whole universe to check if an out happened
void check_out(StateInfo* s, int playerIndex); 
```

**Good (Target):**
```c
// 1. Rules: Depends ONLY on GameState
int is_inning_over(const GameState* gs);

// 2. Logic: Depends ONLY on Player Domain State
int can_player_advance(const BattingTeamPlayerInfo* player, const GameState* gs);

// 3. Engine: Updates Runtime State independently
void update_movement(PlayerRuntimeState* runtime, const BattingTeamPlayerInfo* domain, ...);
```

### Benefits
1.  **Testability:** We can create a `GameState` struct in a test without mocking the entire engine.
2.  **Safety:** A rendering function taking `const BattingTeamPlayerInfo*` CANNOT accidentally modify `woundedApply` or `arrivedToBase`.
3.  **Clarity:** The signature tells you exactly what data the function reads and writes.
