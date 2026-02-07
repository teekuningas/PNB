# PNB Architecture

**Last updated:** 2026-02-07
**Current Status:** Milestone 18.0 Complete ✅ | Ready for M18.1 (Test Fixture Unification) 🎯

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

## Recent Fixes (2026-01-26 to 2026-01-30)

### Strike Reset Bug (M17)
**Problem:** Strikes/balls were resetting 1-2 seconds after a pitch.

**Fix:** Moved `batterEntered` emission to batter selection time, not ready position. Event fires only once per batter.

### Homerun Contest Logic (M17.5)
**Problem:** Premature pair transitions, complex pair-ending logic.

**Fix:** 
- Added `homerunPairHasPitch` tracking flag
- Simplified pair-ending to 3 explicit conditions
- Permissive logic (allows play to continue unless stuck)

### Test Infrastructure (M17.5)
**Achievement:** All 15 integration tests refactored to follow Referee Supremacy pattern.
- Tests set physical state, emit events, let referee infer legal state
- Zero manual referee state manipulation in tests

### Initialization Cleanup (M18.0) - 2026-02-07
**Problem:** Confusing `gameInitialized` event created asymmetry between game start and transitions.

**Changes:**
- Added `initialize_referee()` - explicit function to scan physical world during setup
- Renamed `Referee_Update` → `update_referee` for naming consistency
- Removed `gameInitialized` event entirely
- Fixed double-initialization bug in `returnToGame()`
- Made all menu→game transitions consistent

**Result:** 
- Clear pattern: Setup = explicit calls, Transitions = state machines
- No confusing events suggesting deferred initialization
- Consistent snake_case naming throughout

### Debug Logging Issue Found (M18.0) - 2026-02-07
**Problem:** debug.log only shows "active" players, hiding IDLE players and critical game state.

**Impact:** Makes debugging period transitions and player selection nearly impossible.

**Plan (M18.1):**
- Show ALL 24 players regardless of state
- Include scoreboard (period, inning, batterOrder for both teams)
- Include halfInningState (outs, strikes, balls)
- Include playerCounters (nonJokerPlayersLeft, jokersLeft)
- Improve formatting for readability

---

## Current Roadmap

| # | Milestone | Goal | Status |
|---|-----------|------|--------|
| **17** | **Referee Consolidation** | **Referee is sole writer. Loop ordered.** | **✅ DONE** |
| **17.5** | **Homerun Contest & Final Cleanup** | **Test homerun mode. Complete consolidation.** | **✅ DONE** |
| **18.0** | **Initialization Cleanup** | **Remove gameInitialized event, explicit init.** | **✅ DONE** |
| **18.1** | **Debug Logging Improvements** | **Show all players, add metadata.** | **🎯 NEXT** |
| 18.2 | Test Fixture Unification | Unify all test initialization paths. | ⏳ TODO |
| 18.3 | Referee Internal Refactoring | Extract state machines, RefereeContext. | ⏳ TODO |
| 19 | Physics/State Split | Extract pure physics from `game_manipulation`. | 🔮 Future |
| 20 | Action Decoupling | Split `actions_messy/` into pure logic + execution. | 🔮 Future |
| 21 | User Intent Layer | Input → Intent → Engine (Replay support). | 🔮 Future |

---

## Build & Test

```bash
# Build
make main

# Test (63 tests: 48 unit + 15 integration)
devenv shell make test              # Unit tests
devenv shell make integration_test  # Scenario tests
```
