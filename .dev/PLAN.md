## Refactoring Master Plan

## Current Status: Milestone 9 COMPLETE ✅ → Milestone 10 (The Referee Architecture)

**Foundation Quality:** 10.0/10 - Excellent Type-Safe Domain State & Minimal Scope Functions
**Current Focus:** Architectural Pattern Implementation
**Philosophy:** "Separate Rules from Reality" - The Referee decides, the Engine executes.

---

## Milestone 7.5: Data Structure Cleanup - COMPLETE ✅

**Achievement (2026-01-03):** Successfully decomposed `GameAnalysisInfo` God Object!

### What We Accomplished
- ✅ Created 6 focused structs: `GameState`, `GameControlFlags`, `WoundingState`, `CameraState`, `PlayerCounters`, `GameModeState`
- ✅ Migrated all fields from `GameAnalysisInfo`
- ✅ Deleted `GameAnalysisInfo` struct
- ✅ All 51 unit tests passed

---

## Milestone 8: The Great Narrowing - COMPLETE ✅

**Achievement (2026-01-04):** Decoupled utility/physics logic from global state.

### What We Accomplished
- ✅ Narrowed movement functions to use `PlayerInfo*` instead of `LocalGameInfo*`.
- ✅ Narrowed ball logic to use `BallInfo*`.
- ✅ Narrowed AI strategies to use `const GameState*`.
- ✅ Verified with full test suite.

---

## Milestone 9: Type Safety & State Machines - COMPLETE ✅

**Achievement (2026-01-04):** Replaced "int flags" with strong Enums.

### What We Accomplished
- ✅ Defined `PlayerAnimationModel` enum (replaced magic 0-16).
- ✅ Defined `PitchCycleState` enum (replaced loose flags).
- ✅ Defined `ActionTriggerState`, `PitchActionPhase`, `BatActionPhase` for `ActionFlags`.
- ✅ All magic numbers eliminated from action systems.

---

## Milestone 10: The Referee Architecture (CURRENT)

**Goal:** Implement the high-level logic arbitrator.

**The Vision:**
- **Referee Layer:** Analyzes physical world → Outputs Abstract State & Permissions.
- **Benefits:** Explicit permissions, synchronous breathing, replayability.

**Plan:**
1.  **Design Referee Interface:** Input (GameState), Output (GameEvents, Permissions).
2.  **Extract Rule Logic:** Move logic from `game_analysis.c` to `Referee` module.
3.  **Integrate:** Connect `Referee` to `game_manipulation.c` and `action_implementation.c`.

---

## Completed Milestones

### ✅ Milestone 9: Type Safety & State Machines (COMPLETE 2026-01-04)
- Strong enums for Animations, Pitching, and Actions.

### ✅ Milestone 8: The Great Narrowing (COMPLETE 2026-01-04)
- Function signatures narrowed to minimal scope.

### ✅ Milestone 7.5: Data Structure Cleanup (COMPLETE 2026-01-03)
- Decomposed `GameAnalysisInfo`.

### ✅ Milestone 7: Data Renaissance (COMPLETE 2026-01-01)
- Eliminated ALL legacy state flags.
- Type-safe enums: `PlayerUnitState`, `BaseID`.

### ✅ Milestone 6: Rules Engine Extraction
- Extracted pure rules logic.

---
