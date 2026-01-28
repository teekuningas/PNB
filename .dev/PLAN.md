# PNB Development Plan

## 🎯 CURRENT MILESTONE: Milestone 17.5 (Homerun Contest & Final Referee Cleanup)

**Current Status:** Test Infrastructure Refactored ✅ | Remaining: Homerun Contest + Final Cleanup
**Date:** 2026-01-28

Before proceeding to the major Physics/State Split, we need to properly test and clean up the special game mode (Homerun Contest) and complete the final Referee Supremacy task.

---

## 🏔️ The Plateau (Current State)

We have achieved **Referee Supremacy** (almost complete):
*   ✅ `Referee_Update` is the **sole writer** of `RefereeState` and `BetweenPitchState`.
*   ✅ All "initialization exceptions" (foul reset, batter entry) are now handled via **Events**.
*   ✅ `game_analysis` has been merged into **`game_consolidation.c`**.
*   ✅ The Main Loop is strictly ordered: **Input → Physics → Referee → Consolidation**.
*   ✅ All 63 tests (48 unit + 15 integration) are passing.
*   ✅ **Strike Reset Bug Fixed:** Strikes/balls no longer reset after swinging.
*   ✅ **Test Infrastructure Unified:** All 15 integration tests follow Referee Supremacy pattern.
*   ✅ **Test Directory Reorganized:** `tests/unit/` and `tests/integration/` properly separated.
*   ⚠️ **Exception:** `setRunnerAndBatter()` still writes to referee state for homerun contest setup.

---

## 🎯 Milestone 17.5: Homerun Contest & Final Referee Cleanup (1-2 sessions remaining)

**Goal:** Test the Homerun Contest mode thoroughly and complete the final referee consolidation.

### Phase 1: Test Infrastructure Refactoring ✅ COMPLETE
*   ✅ All 15 integration tests refactored to follow **Referee Supremacy Pattern**
*   ✅ Tests now set up physical state first, then let referee infer legal state via events
*   ✅ Zero manual referee state manipulation in tests or helpers
*   ✅ Unified test patterns across all test types (fly ball, pitching, free walk, timing edge cases)
*   ✅ Test directory structure reorganized: `tests/unit/` and `tests/integration/`
*   ✅ New standardized helpers: `initialize_referee_from_physical_state()`, `snapshot_pitch_start_state()`, `move_pitcher_away()`
*   ✅ Removed unused helpers: `give_ball_to_fielder()`, `simulate_until()`

### Phase 2: GameFlowState Cleanup (TODO)
*   ⏳ Remove unused timers from `GameFlowState`
*   ⏳ Move `endOfInningCounter` from `GameFlowState` to `RefereeState` for consistency
*   ⏳ Document remaining flow control state vs. referee state distinction

### Phase 3: Homerun Contest Testing (TODO)
*   ⏳ Create integration tests for Homerun Contest mode
*   ⏳ Test normal operation (batter + runner setup via `setRunnerAndBatter()`)
*   ⏳ Test foul play reset in homerun contest
*   ⏳ Verify strikes, outs, scoring work correctly in period >= 4
*   ⏳ Unify integration test and "human test fixture" setups for homerun contest scenarios

### Phase 4: Final Referee Cleanup (TODO)
*   ⏳ Refactor `setRunnerAndBatter()` to use events or move initialization to referee
*   ⏳ Remove the last direct writes to referee state from non-referee code
*   ⏳ Document any remaining special cases with justification

### Success Criteria
*   ✅ All integration tests follow unified Referee Supremacy pattern
*   ✅ Test directory structure properly organized
*   ⏳ `GameFlowState` cleaned up (unused timers removed, `endOfInningCounter` moved)
*   ⏳ Homerun Contest has test coverage
*   ⏳ All tests pass (including new homerun contest tests)
*   ⏳ Zero direct writes to referee state from non-referee code (except `initializeRefereeState()` calls)

---

## 🎯 Milestone 18: Physics/State Split (Next 7-8 sessions)

**Goal:** Deconstruct `game_manipulation.c` (~1500 LOC) into a pure Physics Engine.

**Why:** Currently, `game_manipulation.c` mixes pure physics integration (velocity/gravity) with game state mutations and event logic. We want to separate "what happened physically" from "game rules".

### Phase 1: Pure Physics Core
*   Create `PhysicsEngine` module.
*   Functions should take `State` + `dt` and return `NewPosition` / `CollisionEvents`.
*   No side effects, no dependencies on `RefereeState` or `GameControl`.

### Phase 2: Physics Observer
*   Create a bridge that watches Physics output and emits `GameEvents` (e.g., "Ball Hit Ground", "Catch Made").
*   This decouples the physics calculation from the event system.

### Phase 3: The Split
*   Refactor `game_manipulation.c`:
    *   Extract movement logic to `PhysicsEngine`.
    *   Extract event triggering to `PhysicsObserver`.
    *   Rename remaining logic to `game_state_updater.c` (or similar).

---

## 🔮 Future Milestones

*   **Milestone 19:** Action System Decoupling (Split `actions_messy/` into pure logic + execution).
*   **Milestone 20:** User Intent Layer (Input → Intent → Engine). Foundation for replay/network.

---

## 📋 Task Log (Completed)

| Milestone | Task | Result |
| :--- | :--- | :--- |
| **M17.5** | **Homerun Contest & Final Cleanup** | **IN PROGRESS** |
| | **Test Infrastructure Refactoring** | **✅ COMPLETE** |
| | All 15 integration tests unified | ✅ Follow Referee Supremacy pattern |
| | Test directory reorganization | ✅ tests/unit/ and tests/integration/ |
| | Standardized test helpers | ✅ initialize_referee, snapshot_pitch, move_pitcher_away |
| | Removed manual referee manipulation | ✅ Zero instances in tests |
| | Strike Reset Bug | ✅ Fixed batterEntered event timing |
| | Redundant Initialization | ✅ Removed initializeCriticalBattingTeamInformation |
| | Referee Write Violations | ✅ Fixed batting_system.c writes |
| | GameFlowState Cleanup | ⏳ TODO (unused timers, endOfInningCounter) |
| | Homerun Contest Testing | ⏳ TODO |
| | Final Referee Cleanup | ⏳ TODO (setRunnerAndBatter) |
| **M17** | **Referee Consolidation** | **✅ COMPLETED** |
| | GameControl Split | ✅ Separated Flow vs. Rules |
| | Wounding Logic | ✅ Fully timer-based in Referee |
| | Event-Driven Init | ✅ Batter Entry & Pitch Start use events |
| | Foul Reset Fix | ✅ Event-driven reset, zero writes outside Referee |
| | Loop Reordering | ✅ Analysis/Consolidation runs after Referee |
| | Consolidation Module | ✅ Merged Analysis + Reconciliation |

---

**See Also:**
*   `docs/ARCHITECTURE.md` - Detailed architectural vision.
*   `.dev/TASK_AGENT.md` - Protocol for task execution.