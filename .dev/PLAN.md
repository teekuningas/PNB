# PNB Development Plan

## Current Phase: Ready for Referee Consolidation Phase 2

✅ **Phase 1 Complete** (Jan 10, 2026): Successfully reorganized structures, fixed critical bugs, and established clean event system. Split `GameControlFlags` into `GameEvents` (transient) + `GameControl` (stateful). Fixed ball physics bugs from "Organize structs" commit. All 60 tests passing (54 unit + 6 integration).

**Event System Status:** ✅ PERFECT - All patterns audited and verified. `catchMade`→`catchHasBeenMade` pattern working correctly. Unused `batterStartedRunning` removed.

**Next:** Begin Phase 2 - Move mutation responsibilities to referee.c so it becomes the sole authority on `RefereeState` and `GameState`.

**Reference:** See `docs/REFEREE_CONSOLIDATION_PLAN.md` for complete two-phase strategy.

---

## 📅 Roadmap

### ✅ Milestone 10-13: State Consolidation (Completed Jan 2026)
- **Result:** `StateInfo` is the single source of truth. `RefereeState` and `AIState` exist.
- **Key Win:** Integration tests for §33 (Outs) and §36 (Tuplahaava) are passing.

### ✅ Milestone 14: The Great Decoupling V1 (Completed Jan 2026)
- **Goal:** Split "Messy" coordinators into "Pure Logic" + "State Applicators".
- **Completed:**
    - `Referee_Analyze` (pure) + `Referee_Apply` (impure) + `reconcile` (phys-logic sync)
    - State validator with runtime checks
    - Eliminated `baseControlIndex` array
    - 67 tests passing (53 unit + 14 integration)

### ✅ Milestone 15: Referee Architecture V2 (Sequential Update) (Completed Jan 2026)
- **Goal:** Replace `Referee_Analyze` -> `RefereeDecisions` -> `Referee_Apply` with a sequential update pipeline.
- **Result:** `Referee_Update` implemented in `referee.c` calling sequential helpers (`update_safety`, `update_outs`, `update_runs`).
- **Cleanup:** `RefereeDecisions` struct removed. `Referee_Analyze` and `Referee_Apply` removed. `ballHome` logic moved to `game_manipulation.c`.

### ✅ Milestone 16: Structural Reorganization & Event System (Completed Jan 10, 2026)
- **Goal:** Create clean, semantically clear structures and establish robust event system.
- **Reference:** See `docs/REFEREE_CONSOLIDATION_PLAN.md` for complete strategy.
- **Completed:**
    - ✅ Split `GameControlFlags` → `GameEvents` (transient) + `GameControl` (stateful)
    - ✅ Migrated all fields (catchMade, playerArrivedAtBase, etc.)
    - ✅ Added `clearFrameEvents()` mechanism
    - ✅ Fixed critical ball physics bugs:
        - Missing closing brackets in `game_manipulation.c`
        - Comment-code merge that broke `if` condition
        - Restored proper block nesting
    - ✅ Fixed event system patterns:
        - Changed `gameEvents.catchMade` → `gameControl.catchHasBeenMade` 
        - Added `catchHasBeenMade` initialization
        - Verified all event patterns (transient vs persistent)
    - ✅ Removed unused `batterStartedRunning` event
    - ✅ Updated 14+ files across codebase
    - ✅ Removed deprecated `GameControlFlags` structure
    - ✅ All tests passing (54 unit + 6 integration)
- **Result:** Clean event communication pattern established. Event system verified perfect. Ready for Phase 2.
- **Note:** `GameFlowState` elimination deferred - will be addressed during Phase 2 when moving frame counters.

### 🚧 Milestone 17: Referee Consolidation (Next - Phase 2)
- **Goal:** Make referee.c the sole authority on `RefereeState` and `GameState`.
- **Principle:** **Limited Scope** - Referee MUST NOT mutate other structures (e.g., `PlayerInfo`, `BallInfo`, `pRAI`). It only writes to `RefereeState`, `GameState`, and `GameControl`.
- **Reference:** See `docs/REFEREE_CONSOLIDATION_PLAN.md` for complete strategy.
- **Current Status:** In Progress (Jan 11, 2026).
- **Completed:**
    - ✅ Moved strike/ball counting from `game_manipulation.c` to `referee.c`.
    - ✅ Implemented `pitchResolutionProcessed` flag in `GameControl` for state cleanup synchronization.
    - ✅ Migrated free walk safety grants and run scoring to `referee.c`.
    - ✅ Added full-scenario integration tests for pitching and free walk resolution.
- **Next Steps:**
    1. Redesign wounding state machine for frame independence.
    2. Remove all RefereeState/GameState mutations from other systems.
- **Result:** Decoupled, frame-independent referee that only reads events and only writes legal state.

### 🔮 Milestone 18: Game Manipulation Decomposition
- **Goal:** Break the massive `gameManipulation` function into smaller, focused subsystems.
- **Concept:** Functions should only take the data they need (e.g., `updateBallPhysics(BallInfo*)` instead of `updateBall(StateInfo*)`).
- **Why:** Trivial unit testing, zero side effects.

### 🔮 Milestone 19: Action System Decoupling
- **Goal:** Apply pure/impure separation to the action system (`actions_messy/`).
- **Target Files:** `batting_system.c`, `pitching_system.c`, `throwing_system.c`.

### 🔮 Milestone 20: The "User Intent" Phase
- **Goal:** Decouple Input from Action.
- **Concept:** Input generates an `Intent` (e.g., `INTENT_SWING_BAT`). The Engine consumes `Intent`.

---

## Ongoing Tasks

### Test Infrastructure
- **Migrate Legacy Integration Tests:** Convert all snapshot-style tests in `tests/integration/test_scenario_*.c` to full-scenario tests.
- **Status:** 6 full-scenario tests passing.

### Technical Debt / Cleanup
- **Globals:** `src/include/globals.h` (773 LOC) - consider splitting after pipeline is complete.
