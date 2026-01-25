# PNB Development Plan

## 🎯 NEXT MILESTONE: Milestone 18 (Physics/State Split)

**Current Status:** Milestone 17 Complete ✅ (The Plateau Reached 🏔️)
**Date:** 2026-01-19

We have successfully consolidated the Referee logic and established a clean, unidirectional main loop. The codebase is stable, tested, and ready for the next major architectural shift.

---

## 🏔️ The Plateau (Current State)

We have achieved **Referee Supremacy**:
*   ✅ `Referee_Update` is the **sole writer** of `RefereeState` and `BetweenPitchState`.
*   ✅ All "initialization exceptions" (foul reset, batter entry) are now handled via **Events**.
*   ✅ `game_analysis` has been merged into **`game_consolidation.c`**.
*   ✅ The Main Loop is strictly ordered: **Input → Physics → Referee → Consolidation**.
*   ✅ All 61 tests (48 unit + 13 integration) are passing.

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