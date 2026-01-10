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

### 🚧 Milestone 16: Structural Reorganization (Current Focus - Phase 1)
- **Goal:** Create clean, semantically clear structures as foundation for referee consolidation.
- **Reference:** See `docs/REFEREE_CONSOLIDATION_PLAN.md` for complete strategy.
- **Tasks:**
    - Split `GameControlFlags` → `GameEvents` (transient) + `GameControl` (stateful)
    - Eliminate `GameFlowState` (move frame counters to game_analysis.c internals, events to GameEvents)
    - Clarify `GameModeState` ownership (defer major changes to Phase 2)
    - Update all references throughout codebase
    - Implement event clearing mechanism in game loop
- **Result:** Clear event communication pattern, reduced structure confusion, foundation for Phase 2.

### 🔮 Milestone 17: Referee Consolidation (Phase 2)
- **Goal:** Make referee.c the sole authority on `RefereeState` and `GameState`.
- **Reference:** See `docs/REFEREE_CONSOLIDATION_PLAN.md` for complete strategy.
- **Tasks:**
    - Move strike reset, event setting to referee
    - Migrate free walk safety grants and run scoring
    - Redesign wounding state machine for frame independence
    - Remove all RefereeState/GameState mutations from other systems
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
- **Status:** 2 full-scenario tests passing (runner scoring, force out).

### Technical Debt / Cleanup
- **Globals:** `src/include/globals.h` (773 LOC) - consider splitting after pipeline is complete.
