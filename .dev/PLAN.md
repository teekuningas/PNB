# PNB Development Plan

## Current Phase: Centralized Mutation & Cleanup

We have successfully migrated the Referee system to a sequential update pipeline (`Referee_Update`), eliminating the `RefereeDecisions` struct. The immediate next focus is to clean up the Referee implementation to ensure it only mutates `RefereeState` and `GameState`, removing dependencies on other structs like `GameControlFlags` and `PlayerCounters` where possible.

**Reference:** See `docs/ARCHITECTURE.md` for technical details.

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

### 🚧 Milestone 16: Centralized Mutation & Referee Cleanup (Current Focus)
- **Goal:** Purify `Referee_Update` to strictly mutate `RefereeState` and `GameState`.
- **Tasks:**
    - Refactor `update_runs` to avoid mutating `GameControlFlags`, `PlayerCounters`, and `GlobalGameInfo` directly.
    - Move all rule/state mutations out of `game_manipulation.c` and `action_implementation.c` into the Referee.
    - Potential Merge: Consider merging `RefereeState` and `GameState` if distinction becomes redundant.
- **Result:** Referee becomes the sole authority on game rules and state transitions.

### 🔮 Milestone 17: Game Manipulation Decomposition
- **Goal:** Break the massive `gameManipulation` function into smaller, focused subsystems.
- **Concept:** Functions should only take the data they need (e.g., `updateBallPhysics(BallInfo*)` instead of `updateBall(StateInfo*)`).
- **Why:** Trivial unit testing, zero side effects.

### 🔮 Milestone 18: Action System Decoupling
- **Goal:** Apply pure/impure separation to the action system (`actions_messy/`).
- **Target Files:** `batting_system.c`, `pitching_system.c`, `throwing_system.c`.

### 🔮 Milestone 19: The "User Intent" Phase
- **Goal:** Decouple Input from Action.
- **Concept:** Input generates an `Intent` (e.g., `INTENT_SWING_BAT`). The Engine consumes `Intent`.

---

## Ongoing Tasks

### Test Infrastructure
- **Migrate Legacy Integration Tests:** Convert all snapshot-style tests in `tests/integration/test_scenario_*.c` to full-scenario tests.
- **Status:** 2 full-scenario tests passing (runner scoring, force out).

### Technical Debt / Cleanup
- **Globals:** `src/include/globals.h` (773 LOC) - consider splitting after pipeline is complete.
