# Data Structure Audit: The "God Objects"

**Date:** 2026-01-03
**Status:** COMPLETE ✅
**Goal:** Map and eliminate the pollution in `BattingTeamPlayerInfo` and `GameAnalysisInfo`.

---

## 1. BattingTeamPlayerInfo (The Player State) - COMPLETE ✅

**Status:** Successfully separated Domain Identity from Runtime Control.

| Field Name | Classification | Purpose | Outcome |
|:---|:---|:---|:---|
| `name`, `speed`, `power`, `number`, `joker` | **Domain** | Identity & Attributes | Kept in `BattingTeamPlayerInfo` |
| `state`, `baseId`, `originalBase` | **Domain** | **Core Rule State** | Kept in `BattingTeamPlayerInfo` |
| `arrivedToBase` | **Control** | Optimization flag | **MOVED** to `PlayerRuntimeState` ✅ |
| `woundedApply` | **Control** | Deferred execution | **MOVED** to `PlayerRuntimeState` ✅ |
| `passedPathPoint` | **Control** | Pathfinding state | **MOVED** to `PlayerRuntimeState` ✅ |
| `goingForward` | **Control** | Movement direction | **MOVED** to `PlayerRuntimeState` ✅ |
| `hasMadeRunOnThirdBase`| **Control** | Rule guard | **MOVED** to `PlayerRuntimeState` ✅ |

### New Structure

```c
typedef struct _PlayerRuntimeState {
	int arrivedToBase;       // Optimization flag
	int woundedApply;        // Deferred execution
	int passedPathPoint;     // State machine variable
	int goingForward;        // Direction tracking
	int hasMadeRunOnThirdBase; // Guard flag
} PlayerRuntimeState;

// In LocalGameInfo:
// PlayerRuntimeState playerRuntime[2*PLAYERS_IN_TEAM + JOKER_COUNT];
```

---

## 2. GameAnalysisInfo (The God Object) - DECOMPOSED ✅

**Status:** The 34-field `GameAnalysisInfo` struct has been deleted. Its fields are now in focused, single-responsibility structs.

### Group A: GameState (The "Scoreboard")
*Pure game rules state. Input for Rules Engine.*

| Field | Purpose | Target Struct |
|:---|:---|:---|
| `outs` | Number of outs | `GameState.outs` |
| `strikes` | Number of strikes | `GameState.strikes` |
| `balls` | Number of balls | `GameState.balls` |
| `runsInTheInning` | Runs scored | `GameState.runsInTheInning` |
| `event` | Last game event | `GameState.event` |
| `outOfBounds` | Rule state | `GameState.outOfBounds` |
| `ballHome` | Rule state | `GameState.ballHome` |
| `endPeriod` | Rule state | `GameState.endPeriod` |

### Group B: GameControlFlags (The "Engine Room")
*Internal loop management and implementation bookkeeping.*

| Field | Purpose | Target Struct |
|:---|:---|:---|
| `pause` | Game paused? | `gameControl.pause` |
| `initLocals` | Reset signal | `gameControl.initLocals` |
| `waitingForBatterDecision` | UI input block | `gameControl.waitingForBatterDecision` |
| `waitingForFreeWalkDecision`| UI input block | `gameControl.waitingForFreeWalkDecision` |
| `freeWalkCalculationMade` | Optimization flag | `gameControl.freeWalkCalculationMade` |
| `freeWalkIndex` | Temp variable | `gameControl.freeWalkIndex` |
| `freeWalkBase` | Temp variable | `gameControl.freeWalkBase` |
| `checkForRun` | Rule trigger | `gameControl.checkForRun` |
| `playerArrivedToBase` | Optimization flag | `gameControl.playerArrivedToBase` |
| `firstCatchMade` | Logic trigger | `gameControl.firstCatchMade` |
| `batterStartedRunning` | AI/Rule hint | `gameControl.batterStartedRunning` |

### Group C: WoundingState
*Specialized state for the wounding system.*

| Field | Purpose | Target Struct |
|:---|:---|:---|
| `woundingCatch` | Trigger for wounding check | `woundingState.woundingCatch` |
| `woundingCatchHandled` | Debounce flag | `woundingState.woundingCatchHandled` |

### Group D: CameraState
*Presentation directives.*

| Field | Purpose | Target Struct |
|:---|:---|:---|
| `homeRunCameraFlag` | Camera mode | `cameraState.homeRunCameraFlag` |
| `targetPoint` | Focus point | `cameraState.targetPoint` |

### Group E: PlayerCounters
*Helper counts for tracking players.*

| Field | Purpose | Target Struct |
|:---|:---|:---|
| `battingTeamPlayersOnFieldCount` | Helper count | `playerCounters.battingTeamPlayersOnFieldCount` |
| `nonJokerPlayersLeft` | Helper count | `playerCounters.nonJokerPlayersLeft` |
| `jokersLeft` | Helper count | `playerCounters.jokersLeft` |
| `noMorePlayers` | Inning end signal | `playerCounters.noMorePlayers` |

### Group F: GameModeState
*State specific to tournament or alternate game modes.*

| Field | Purpose | Target Struct |
|:---|:---|:---|
| `runnerBatterPairCounter`| Mode state | `gameModeState.runnerBatterPairCounter` |
| `canMakeRunOfHonor` | Mode rule | `gameModeState.canMakeRunOfHonor` |
| `forceNextPair` | Mode rule | `gameModeState.forceNextPair` |

---

## Summary of Results

1.  **Strict Typing:** Grouped data by semantics rather than usage site.
2.  **Logic Isolation:** Rules engine (Referee) can now operate on `GameState` without touching `cameraState`.
3.  **Clean Player State:** `BattingTeamPlayerInfo` is now clean, domain-only data.
4.  **Zero God Objects:** `GameAnalysisInfo` is completely removed.