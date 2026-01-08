# PNB Development Plan

## Current Phase: Referee Architecture V2 & Centralization

We are refining the Referee system to move beyond the "Analyze/Apply" pattern towards a cleaner, sequential update model. This allows for better handling of rule dependencies and eliminates the need for large intermediate `RefereeDecisions` structs.

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

### 🚧 Milestone 15: Referee Architecture V2 (Sequential Update)
- **Goal:** Replace `Referee_Analyze` -> `RefereeDecisions` -> `Referee_Apply` with a sequential update pipeline.
- **New Pattern:** `updateReferee(const StateInfo* readOnly, RefereeState* writeOnly)`
- **Mechanism:** A chain of focused, testable functions that update the referee state step-by-step (e.g., `check_force_outs`, `check_runs`, `update_safety`).
- **Benefit:** Eliminates the `RefereeDecisions` middleware struct, handles inter-rule dependencies naturally.

### 🔮 Milestone 16: Centralized Mutation
- **Goal:** Move ALL rule and game state mutations (Outs, Runs, Wounding, Strikes) into the `updateReferee` pipeline.
- **Target:** Remove legacy mutations currently scattered in:
    - `game_manipulation.c`
    - `action_implementation.c`
- **Result:** Physics engine (`game_manipulation`) never decides rules; it only reports physical reality.

### 🔮 Milestone 17: Game Manipulation Decomposition
- **Goal:** Break the massive `gameManipulation` function into smaller, focused subsystems.
- **Concept:** Functions should only take the data they need (e.g., `updateBallPhysics(BallInfo*)` instead of `updateBall(StateInfo*)`).
- **Why:** Trivial unit testing, zero side effects.

### 🔮 Milestone 18: Action System Decoupling (Formerly M15)
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