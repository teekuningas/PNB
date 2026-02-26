# PNB Architecture

**Last updated:** 2026-02-26
**Current Status:** FINAL_PLAN.md Phase 1 Complete ✅ | Starting Phase 2 (WOUNDED Enforcement) 🎯

## Vision: The Functional Pipeline

We have transformed the game loop into a strict functional pipeline where data flows in one direction.

```
State_Next = Pipeline(Physics(Input(State)))
```

**The Core Loop:**
1. **Input:** `actionInvocations` → Transforms key presses into Intent/Action Flags.
2. **Physics:** `actionImplementation`, `gameManipulation` → Updates physical world, emits transient `GameEvents`.
3. **Referee:** `update_referee` → Reads Events, updates **Legal State** (Outs, Runs, Strikes). **SOLE WRITER**.
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

### `src/game/game_manipulation.c` (The Physics Engine)
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

---

## Initialization & Transitions

### Two Distinct Patterns

**Setup (Game Start):**
- Explicit function call: `Referee_InitializeFromPhysicalWorld()`
- Called during setup phase (before main loop)
- Scans physical world and establishes initial legal tracking

**Transitions (Runtime):**
- Self-managed via state machines in Referee
- Three transitions: Foul Play, Next Pair, End of Inning
- Pattern: NONE → DETECTED (timer) → RESETTING → NONE
- Referee clears own state, signals Consolidation, world resets

### Transition Timing (Critical Understanding)

When a transition occurs (e.g., End of Inning):

**Frame N+200:**
1. Referee detects condition, transitions to RESETTING
2. Referee clears its own state (strikes, balls, safety)
3. Consolidation sees RESETTING, calls `loadMutableWorldSettings()`
4. Physical world is reset

**Frame N+201:**
1. Referee transitions RESETTING → NONE
2. Normal update logic resumes
3. Safety naturally updates via `update_safety_status()` seeing new positions

**Key Insight:** Referee clears state at Frame N+200 because it knows Consolidation will reset the physical world in the same frame. No event needed - it's a state machine handshake.

---

## Current Roadmap

The active refactoring plan lives in **`docs/FINAL_PLAN.md`**. Summary:

| Phase | Goal | Status |
|-------|------|--------|
| **1** | **Dead Code Cleanup** — remove dead fixtures, empty blocks, tab→space conversion | **✅ DONE** |
| **2** | **WOUNDED Enforcement** — referee decides, consolidation acts (eliminate `processPendingWounds`) | **🎯 NEXT** |
| 3 | Referee Ownership — eliminate all const-casts in `referee.c` | ⏳ TODO |
| 4 | Extract Pure Helpers — `get_batting_team_index()`, `should_period_end()` | ⏳ TODO |
| 5 | Test Strengthening — unit tests for pure helpers, pipeline cooperation tests | ⏳ TODO |
| Future | `game_manipulation.c` decomposition, `common_logic.c` decomposition, action decoupling, intent layer | 🔮 Future |

---

## Build & Test

```bash
# Build
make main

# Test (63 tests: 48 unit + 15 integration)
devenv shell make test              # Unit tests
devenv shell make integration_test  # Scenario tests
```
