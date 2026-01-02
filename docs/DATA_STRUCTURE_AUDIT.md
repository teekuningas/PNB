# Data Structure Audit: The "God Objects"

**Date:** 2026-01-02
**Status:** Audit Complete
**Goal:** Map the pollution in `BattingTeamPlayerInfo` and `GameAnalysisInfo` to prepare for cleanup.

---

## 1. BattingTeamPlayerInfo (The Player State)

**Current Status:** Mixed Domain and Control state.

| Field Name | Type | Classification | Purpose | Plan |
|:---|:---|:---|:---|:---|
| `name` | char* | **Domain** | Player identity | Keep |
| `speed` | int | **Domain** | Attribute | Keep |
| `power` | int | **Domain** | Attribute | Keep |
| `number` | int | **Domain** | Identity | Keep |
| `originalBase` | int | **Domain** | Game Logic (Rules) | Keep |
| `joker` | int | **Domain** | Attribute/Role | Keep |
| `state` | PlayerUnitState | **Domain** | **Core State** (New) | Keep |
| `baseId` | BaseID | **Domain** | **Core State** (New) | Keep |
| `arrivedToBase` | int | **Control** | Optimization flag | **MOVE** to `PlayerRuntimeState` |
| `woundedApply` | int | **Control** | Deferred execution | **MOVE** to `PlayerRuntimeState` |
| `passedPathPoint` | int | **Control** | Pathfinding state | **MOVE** to `PlayerRuntimeState` |
| `goingForward` | int | **Control** | Movement direction | **MOVE** to `PlayerRuntimeState` |
| `hasMadeRunOnThirdBase`| int | **Control** | Rule guard | **MOVE** to `PlayerRuntimeState` |

### New Structure Proposal

```c
// src/include/globals.h

typedef struct _PlayerRuntimeState {
    int arrivedToBase;
    int woundedApply;
    int passedPathPoint;
    int goingForward;
    int hasMadeRunOnThirdBase;
} PlayerRuntimeState;

// Add to LocalGameInfo:
// PlayerRuntimeState playerRuntime[24];
```

---

## 2. GameAnalysisInfo (The God Object)

**Current Status:** 34 fields mixing Rules, UI, Control Flow, and Physics.

### Group A: Pure Game State (The "Scoreboard")
*What the Referee generates and the UI displays.*

| Field | Purpose | Target Struct |
|:---|:---|:---|
| `outs` | Number of outs (0-3) | `GameState` |
| `strikes` | Number of strikes (0-3) | `GameState` |
| `balls` | Number of balls (0-4) | `GameState` |
| `runsInTheInning` | Runs scored this turn | `GameState` |
| `event` | Last game event (Type-safe) | `GameState` |

### Group B: Control Flow Flags
*Internal bookkeeping for the game loop.*

| Field | Purpose | Target Struct |
|:---|:---|:---|
| `pause` | Game paused? | `GameControlFlags` |
| `endPeriod` | Signal to end period | `GameControlFlags` |
| `noMorePlayers` | Signal inning end | `GameControlFlags` |
| `initLocals` | Reset signal | `GameControlFlags` |
| `waitingForBatterDecision` | UI input block | `GameControlFlags` |
| `waitingForFreeWalkDecision`| UI input block | `GameControlFlags` |
| `freeWalkCalculationMade` | Optimization flag | `GameControlFlags` |
| `firstCatchMade` | Logic trigger | `GameControlFlags` |
| `playerArrivedToBase` | Global optimization flag | `GameControlFlags` |

### Group C: Physics/World State
*Physical reality checks.*

| Field | Purpose | Target Struct |
|:---|:---|:---|
| `ballHome` | Is ball at home plate? | `GamePhysicsState` (or similar) |
| `outOfBounds` | Is ball out of bounds? | `GamePhysicsState` |

### Group D: Wounding System
*Specific sub-system state.*

| Field | Purpose | Target Struct |
|:---|:---|:---|
| `woundingCatch` | Trigger for wounding check | `WoundingState` |
| `woundingCatchHandled` | Debounce flag | `WoundingState` |

### Group E: Camera/Visuals
*Rendering directives.*

| Field | Purpose | Target Struct |
|:---|:---|:---|
| `homeRunCameraFlag` | Camera mode trigger | `CameraState` |
| `targetPoint` | Camera focus point | `CameraState` |

### Group F: Player Tracking / Counters
*Counts of entities.*

| Field | Purpose | Target Struct |
|:---|:---|:---|
| `battingTeamPlayersOnFieldCount` | Helper count | `PlayerCounters` |
| `nonJokerPlayersLeft` | Helper count | `PlayerCounters` |
| `jokersLeft` | Helper count | `PlayerCounters` |

### Group G: Specific Logic Helpers (The "Misc" Drawer)
*These might need individual review.*

| Field | Purpose | Target Struct |
|:---|:---|:---|
| `batterStartedRunning` | AI hint | `AIState` (maybe) |
| `checkForRun` | Rule trigger | `GameState` (maybe) |
| `freeWalkIndex` | Temp variable | `GameControlFlags` |
| `freeWalkBase` | Temp variable | `GameControlFlags` |
| `runnerBatterPairCounter`| Tournament/Mode state | `GameModeState` |
| `canMakeRunOfHonor` | Game mode rule | `GameModeState` |
| `forceNextPair` | Game mode rule | `GameModeState` |

---

## Migration Strategy

**Phase 1: PlayerRuntimeState (High Value, Low Risk)**
1. Define `PlayerRuntimeState`.
2. Add to `LocalGameInfo`.
3. Move `arrivedToBase` (Optimization).
4. Move `woundedApply` (Logic).
5. Move remaining flags.

**Phase 2: Split GameAnalysisInfo (High Value, Medium Risk)**
1. Define `GameState` (Pure score data).
2. Move Group A fields.
3. Define `GameControlFlags`.
4. Move Group B fields.
5. Define remaining structs and migrate.

**Phase 3: Cleanup**
1. Update `StateInfo` to hold new structs (or keep in `LocalGameInfo`).
2. Update all references.
