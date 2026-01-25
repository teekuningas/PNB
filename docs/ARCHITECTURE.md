# PNB Architecture

**Last updated:** 2026-01-19
**Current Status:** Milestone 17 Complete ✅ (Consolidation Finalized)

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
*   **`BetweenPitchState`:** Sticky flags for the current pitch (Catch Made, Ball Hit Ground, Out of Bounds). Reset at pitch start.
*   **`HalfInningState`:** Outs, Strikes, Balls, Runs.

### 2. The Physical State (Physics' Domain)
*   **`PlayerInfo`:** Location, velocity, animation state.
*   **`BallInfo`:** Location, velocity.
*   **`GameEvents`:** Transient flags cleared every frame (e.g., `catchMade`, `ballHitGround`, `foulResetCompleted`).

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
    *   Determines if a catch is a wound.
    *   Determines if a runner is forced out.
    *   Counts strikes/balls (including resetting on new batter).
    *   Restores legal safety after a Foul Play Reset event.

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
    *   **Physical Resets:** Teleports players back if `outOfBounds` is active (Foul Play). Emits `foulResetCompleted`.
    *   **Flow:** Checks for end of inning, decides when to prompt for user input (batter/free walk).

---

## The "Referee Supremacy" Pattern

1.  **Event:** Physics detects an event (e.g., Ball hits ground out of bounds).
    *   *Action:* `gameEvents.ballHitGround = 1`.
2.  **Judgment:** Referee sees event + location.
    *   *Action:* `betweenPitchState.outOfBounds = 1`.
3.  **Reaction:** Consolidation sees `outOfBounds`.
    *   *Action:* Starts timer. When expired, calls `executeFoulPlayTeleport()` (moves players) and emits `gameEvents.foulResetCompleted = 1`.
4.  **Restoration:** Referee sees `foulResetCompleted`.
    *   *Action:* Restores `currentSafetyBase` to `baseAtPitchStart`.

---

## Future Roadmap

| # | Milestone | Goal | Status |
|---|-----------|------|--------|
| **17** | **Referee Consolidation** | **Referee is sole writer. Loop ordered.** | **✅ DONE** |
| 18 | Physics/State Split | Extract pure physics from `game_manipulation`. | 🎯 NEXT |
| 19 | Action Decoupling | Split `actions_messy/` into pure logic + execution. | 🔮 Future |
| 20 | User Intent Layer | Input → Intent → Engine (Replay support). | 🔮 Future |

---

## Build & Test

```bash
# Build
make main

# Test (61 tests)
devenv shell make test              # Unit tests
devenv shell make integration_test  # Scenario tests
```
