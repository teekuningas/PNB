# PNB Architecture

**Last updated:** 2026-04-10
**Current Status:** Phases 1–3 Complete ✅ (const-casts 12→3, 73 tests, contract testing, BatOutcome consolidation) | Phase 4 Zero Const-Casts 🎯
**Deep Analysis:** See `PLAN.md` for lifecycle architecture and verified forward plan. See `OPUS_VISION.md` for the post-refactoring architectural target.

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
*   **`GameEvents`:** Transient flags cleared every frame (e.g., `catchMade`, `ballHitGround`, `ballHitByBat`, `pitchReleased`).
*   **`BetweenPitchState`:** Sticky flags promoted from events by referee (e.g., `catchHasBeenMade`, `hasBallHitGround`, `batOutcome`). Reset at pitch start.

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
- Explicit function call: `initialize_referee()` (will become `Referee_ScanPhysicalWorld()` in Phase 7)
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

The active refactoring plan lives in **`docs/PLAN.md`**. The architectural target is in **`docs/OPUS_VISION.md`**.

| Phase | Goal | Status |
|-------|------|--------|
| **1** | **Consolidate & Knight** — remove trivial const-casts (9→3), add unit tests for stable pure functions | ✅ Done |
| **2** | **1-Frame Contract Tests** — prove pipeline stage cooperation at the frame level | ✅ Done |
| **3** | **GameEvents Migration** — BatOutcome enum in BetweenPitchState, event→sticky pattern, fixed event semantics | ✅ Done |
| **4** | **Zero Const-Casts** — eliminate last 3 const-casts, compiler enforces ownership | **🎯 NEXT** |
| **5** | **Extract get_batting_team_index** — replace 10 copies of the batting-team formula | ⏳ TODO |
| **6** | **Bug Fix + Period Logic** — extract `should_period_end()`, fix Bug #1 (pending runs ignore endPeriod) | ⏳ TODO |
| **7** | **Initialization Unification** — reset recipes, split init by ownership, eliminate dual-init | ⏳ TODO |
| **8** | **Organization** — rename files, split large files, standardize tests | ⏳ TODO |

*Previous phases (Dead Code Cleanup, WOUNDED Enforcement, homerunPairHasPitch move) are complete. See `docs/archive/` for history.*

---

## Build & Test

```bash
# Build
make main

# Test (73 tests: 54 unit + 4 contract + 15 scenario)
devenv shell make test              # Unit tests
devenv shell make integration_test  # Contract tests (1-frame pipeline proofs)
devenv shell make scenario_test     # Scenario tests (full-game simulations)
```
