# PNB Development Plan

## Current Phase: Referee Consolidation (Phase 2)

**Goal:** Decouple referee logic into a frame-independent monitoring system that is the sole authority on `RefereeState` and `HalfInningState`.

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

### 🚧 Milestone 17: Referee Consolidation (In Progress)
- **Goal:** Make referee.c the sole authority on `RefereeState` and `HalfInningState`.
- **Status:**
    - ✅ **Strike/Ball Counting:** Moved to `referee.c`.
    - ✅ **Free Walk Logic:** Moved to `referee.c`.
    - ❌ **OutOfBounds:** Still scattered (`game_manipulation.c`, `game_analysis.c`).
    - ❌ **Wounding Logic:** Still in `game_analysis.c` (frame-dependent).
    - ❌ **End Period:** Still scattered.
- **Next Steps:**
    1.  **Consolidate Flags:** Move `outOfBounds` and `endPeriod` management to Referee.
    2.  **Event-Driven Wounding:** Replace frame counters in `game_analysis.c` with `RefereeState` timers.
    3.  **Eliminate Redundancy:** Remove `batterIndex` and rely on `PLAYER_STATE_AT_BAT`.
    4.  **Delete game_analysis.c:** Move remaining logic to `game_setup.c` or Referee.

### 🔮 Milestone 18: Game Manipulation Decomposition
- **Goal:** Break `gameManipulation` into smaller, pure subsystems (`updateBallPhysics`, `updatePlayerPhysics`).

### 🔮 Milestone 19: Action System Decoupling
- **Goal:** Pure/Impure separation for `batting_system.c`, `pitching_system.c`, `throwing_system.c`.

### 🔮 Milestone 20: The "User Intent" Phase
- **Goal:** Decouple Input from Action (`Input` -> `Intent` -> `Engine`).

---

## Technical Debt / Cleanup
- **Globals:** `src/include/globals.h` (700+ LOC) - Split into focused headers.
- **Test Coverage:** Migrate legacy integration tests to full-scenario tests.
