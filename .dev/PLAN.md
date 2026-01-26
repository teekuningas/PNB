# PNB Development Plan

## 🎯 NEXT MILESTONE: Milestone 17.5 (Homerun Contest & Final Referee Cleanup)

**Current Status:** Milestone 17 Complete ✅ + Strike Reset Bug Fixed 🐛
**Date:** 2026-01-26

Before proceeding to the major Physics/State Split, we need to properly test and clean up the special game mode (Homerun Contest) and complete the final Referee Supremacy task.

---

## 🏔️ The Plateau (Current State)

We have achieved **Referee Supremacy** (mostly):
*   ✅ `Referee_Update` is the **sole writer** of `RefereeState` and `BetweenPitchState`.
*   ✅ All "initialization exceptions" (foul reset, batter entry) are now handled via **Events**.
*   ✅ `game_analysis` has been merged into **`game_consolidation.c`**.
*   ✅ The Main Loop is strictly ordered: **Input → Physics → Referee → Consolidation**.
*   ✅ All 63 tests (48 unit + 15 integration) are passing.
*   ✅ **Strike Reset Bug Fixed:** Strikes/balls no longer reset after swinging.
*   ⚠️ **Exception:** `setRunnerAndBatter()` still writes to referee state for homerun contest setup.

---

## 🎯 Milestone 17.5: Homerun Contest & Final Referee Cleanup (1-2 sessions)

**Goal:** Test the Homerun Contest mode thoroughly and complete the final referee consolidation.

### Phase 1: Homerun Contest Testing
*   Create integration tests for Homerun Contest mode
*   Test normal operation (batter + runner setup)
*   Test foul play reset in homerun contest
*   Verify strikes, outs, scoring work correctly

### Phase 2: Final Referee Cleanup
*   Refactor `setRunnerAndBatter()` to use events or move initialization to referee
*   Remove the last direct writes to referee state from non-referee code
*   Document any remaining special cases with justification

### Success Criteria
*   ✅ Homerun Contest has test coverage
*   ✅ All tests pass (including new homerun contest tests)
*   ✅ Zero direct writes to referee state from non-referee code (except `initializeRefereeState()` calls)

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
| | Strike Reset Bug | Fixed batterEntered event timing |
| | Redundant Initialization | Removed initializeCriticalBattingTeamInformation |
| | Referee Write Violations | Fixed batting_system.c writes |
| | Homerun Contest Testing | TODO |
| | Final Referee Cleanup | TODO (setRunnerAndBatter) |
| **M17** | **Referee Consolidation** | **COMPLETED** |
| | GameControl Split | Separated Flow vs. Rules |
| | Wounding Logic | Fully timer-based in Referee |
| | Event-Driven Init | Batter Entry & Pitch Start use events |
| | Foul Reset Fix | Event-driven reset, zero writes outside Referee |
| | Loop Reordering | Analysis/Consolidation runs after Referee |
| | Consolidation Module | Merged Analysis + Reconciliation |

---

**See Also:**
*   `docs/ARCHITECTURE.md` - Detailed architectural vision.
*   `.dev/TASK_AGENT.md` - Protocol for task execution.