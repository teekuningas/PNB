# PNB Architecture

**Last updated:** 2026-01-26
**Current Status:** Milestone 17 Complete ✅ + Strike Reset Bug Fixed 🐛

## Vision: The Functional Pipeline

We have transformed the game loop into a strict functional pipeline where data flows in one direction.

```
State_Next = Pipeline(Physics(Input(State)))
```

**The Core Loop:**
1. **Input:** `actionInvocations` → Transforms key presses into Intent/Action Flags.
2. **Physics:** `actionImplementation`, `gameManipulation` → Updates physical world, emits transient `GameEvents`.
3. **Referee:** `Referee_Update` → Reads Events, updates **Legal State** (Outs, Runs, Strikes). **SOLE WRITER**.
4. **Consolidation:** `GameConsolidation_Update` → Reacts to Legal State, updates Flow, enforces physical outcomes (e.g., removing OUT players).
5. **Render:** Draws the state.

---

## Key Data Structures

### 1. The Legal State (Referee's Domain)
*   **`RefereeState`:** Persistent legal status of players (Safe, Wounded, Out) and pitch snapshots.
*   **`BetweenPitchState`:** Sticky flags for the current pitch (Catch Made, Ball Hit Ground, Foul State). Reset at pitch start.
*   **`HalfInningState`:** Outs, Strikes, Balls, Runs.

### 2. The Physical State (Physics' Domain)
*   **`PlayerInfo`:** Location, velocity, animation state, baseId.
*   **`BallInfo`:** Location, velocity.
*   **`GameEvents`:** Transient flags cleared every frame (e.g., `catchMade`, `ballHitGround`, `batterEntered`, `pitchReleased`).

### 3. Flow Control (Consolidation's Domain)
*   **`FlowControl`:** User interaction flags (Waiting for batter decision, Free walk offers).
*   **`GameFlowState`:** Timers for innings, out-of-bounds resets.

---

## Module Responsibilities

### `src/game/referee.c` (The Judge)
*   **Role:** The **SOLE WRITER** of the Legal State.
*   **Input:** Read-only access to Physical State and `GameEvents`.
*   **Output:** Updates `RefereeState`, `BetweenPitchState`, `HalfInningState`.
*   **Logic:**
    *   Responds to events: `batterEntered` (reset strikes/balls), `pitchReleased` (snapshot state).
    *   Determines if a catch is a wound.
    *   Determines if a runner is forced out.
    *   Counts strikes/balls.
    *   Restores legal safety after a Foul Play Reset.

### `src/game/game_manipulation.c` (The Physics Engine - *Target of M18*)
*   **Role:** Simulates the physical world.
*   **Logic:**
    *   Moves ball and players based on velocity.
    *   Detects collisions and boundary checks.
    *   Emits `GameEvents` (e.g., `ballHitGround`).

### `src/game/game_consolidation.c` (The Enforcer & Game Master)
*   **Role:** Reacts to the Referee's decisions and manages flow.
*   **Logic:**
    *   **Enforcement:** Physically removes players marked `OUT` or `WOUNDED`. Triggers "panic runs" if safety is lost.
    *   **Physical Resets:** Teleports players back if foul play detected. 
    *   **Flow:** Checks for end of inning, decides when to prompt for user input (batter/free walk).

---

## The "Referee Supremacy" Pattern

1.  **Event:** Physics/Actions detect an event (e.g., Batter selected, Ball hits ground out of bounds).
    *   *Action:* Set appropriate `gameEvents` flag (e.g., `batterEntered = 1`, `ballHitGround = 1`).
2.  **Judgment:** Referee processes event.
    *   *Action:* Updates legal state (e.g., reset strikes/balls, set `foulState = DETECTED`).
3.  **Reaction:** Consolidation sees referee's decision.
    *   *Action:* Starts timer. When expired, executes physical changes (moves players).
4.  **Restoration:** Referee restores legal state after physical reset.
    *   *Action:* Restores `currentSafetyBase` from `baseAtPitchStart`.

**Critical Rule:** Only `referee.c` writes to `RefereeState`, `BetweenPitchState`, and `HalfInningState`. All other code reads these structures or emits events.

**Current Exception:** `setRunnerAndBatter()` in homerun contest mode (to be addressed in M17.5).

---

## Recent Fixes (2026-01-26)

### Strike Reset Bug
**Problem:** Strikes/balls were resetting 1-2 seconds after a pitch, allowing batters to hit indefinitely.

**Root Cause:** `prepareBatter()` was emitting `batterEntered` event every time the batter reached ready position, including after swinging. The referee would then reset strikes/balls.

**Fix:** Moved `batterEntered` emission to batter selection time (in `batting_system.c`), not when reaching ready position. Now the event fires only once per batter.

### Referee Write Violations Cleanup
**Removed:**
- Direct writes to `baseAtPitchStart` and `currentSafetyBase` in `batting_system.c`
- Redundant `initializeCriticalBattingTeamInformation()` function (duplicated `initializeRefereeState()`)

**Result:** Referee now properly handles initialization via events and its own initialization function.

---

## Future Roadmap

| # | Milestone | Goal | Status |
|---|-----------|------|--------|
| **17** | **Referee Consolidation** | **Referee is sole writer. Loop ordered.** | **✅ DONE** |
| **17.5** | **Homerun Contest & Final Cleanup** | **Test homerun mode. Fix setRunnerAndBatter().** | **🎯 NEXT** |
| 18 | Physics/State Split | Extract pure physics from `game_manipulation`. | 🔮 Future |
| 19 | Action Decoupling | Split `actions_messy/` into pure logic + execution. | 🔮 Future |
| 20 | User Intent Layer | Input → Intent → Engine (Replay support). | 🔮 Future |

---

## Build & Test

```bash
# Build
make main

# Test (63 tests: 48 unit + 15 integration)
devenv shell make test              # Unit tests
devenv shell make integration_test  # Scenario tests
```
