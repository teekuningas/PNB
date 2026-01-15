# PNB Development Plan

## Current Phase: Referee Consolidation (Phase 2)

**Goal:** Decouple referee logic into a frame-independent monitoring system that is the sole authority on `RefereeState` and `HalfInningState`. The immediate objective is to dismantle `game_analysis.c` and make `referee.c` the source of truth for game rules.

**The Golden Rule:**
1.  **One Way Flow:** Physics -> Events -> Referee -> Legal State -> Reconciliation -> Physics
2.  **Limited Scope:** Referee MUST NOT mutate other structures (e.g., `PlayerInfo`, `BallInfo`, `pRAI`, `AIState`). Its output is strictly the Legal State (`RefereeState`, `HalfInningState`, `GameControl`).

### Ownership Map

| Structure | Owner (Writer) | Readers |
| :--- | :--- | :--- |
| `GameEvents` | Physics/Actions | Referee |
| `RefereeState` | **referee.c ONLY** | Reconcile, UI, AI |
| `HalfInningState` | **referee.c ONLY** | Everyone |
| `GameControl` | Referee (mostly) | Physics, AI |
| `PlayerInfo` | Physics/Actions | Referee (Read-only) |

---

## 📅 Roadmap

### ✅ Milestone 16: Structural Reorganization (Jan 2026)
- **Completed:**
    - Split `GameControlFlags` into `GameEvents` (transient) + `GameControl` (stateful).
    - Established strict Event -> Referee -> State pipeline.
    - Verified event system with 60+ passing tests.

### 🚧 Milestone 17: Referee Consolidation & The End of Analysis (In Progress)
- **Goal:** Complete the transition of rule logic to `referee.c` and delete `game_analysis.c`.
- **Status:**
    - ✅ **Strike/Ball Counting:** Moved to `referee.c`.
    - ✅ **Free Walk Logic:** Moved to `referee.c`.
    - ❌ **OutOfBounds:** Scattered (`game_manipulation.c`, `game_analysis.c`).
    - ❌ **Wounding Logic:** In `game_analysis.c` (frame-dependent).
    - ❌ **End Period:** Scattered.
- **Detailed Migration Plan:**
    1.  **State Expansion:** Add `woundingTimer`, `endInningTimer`, `nextPairTimer` to `RefereeState`.
    2.  **Wounding:** Move `woundingCatchEffects` to `referee.c`. Use `dt` (delta time) for timer.
    3.  **Flow Control:** Move `checkIfEndOfInning` and `checkIfNextPair` logic to `referee.c` (the *decision* to switch).
        - *Note:* The actual state reset (moving players) should be triggered by a flag set by Referee, executed by `mutable_world.c` or a dedicated `StateReset` system.
    4.  **Foul Play:** Move detection to `referee.c`.
    5.  **Visuals:** Move `homeRunCameraCounter` and `populateGameConclusion` to `game_screen.c`.
    6.  **Deletion:** Remove `src/game/game_analysis.c`.

### 🔮 Milestone 18: The Physics/State Split (New Major Phase)
- **Goal:** Deconstruct `game_manipulation.c`. This is a massive task requiring the separation of pure physics integration (Movement) from rule application (Events).
- **Tasks:**
    - Create a pure `PhysicsEngine` that takes `State + dt` and outputs `NewPositions`.
    - Decouple "Physical Safety" (touching base) from "Legal Safety" (referee decision).

### 🔮 Milestone 19: Action System Decoupling
- **Goal:** Pure/Impure separation for `batting_system.c`, `pitching_system.c`, `throwing_system.c`.

### 🔮 Milestone 20: The "User Intent" Phase
- **Goal:** Decouple Input from Action (`Input` -> `Intent` -> `Engine`).

---

## Technical Debt / Cleanup
- **Globals:** `src/include/globals.h` (700+ LOC) - Split into focused headers.
- **Test Suite Modernization:** 
    - **Unit Tests:** Many are brittle because they mock complex "messy" state. *Action:* Delete/rewrite brittle tests during refactoring. Focus unit tests on pure logic (`rules_pure/`).
    - **Integration Tests:** Consolidate scenario tests to serve as the primary regression net.
