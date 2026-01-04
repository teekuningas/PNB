## Refactoring Master Plan

## Current Status: Milestone 9 COMPLETE ✅ → Milestone 10 (Stabilization & Cleanup)

**Foundation Quality:** 10.0/10 - Excellent Type-Safe Domain State & Minimal Scope Functions
**Current Focus:** Stability & Code Hygiene
**Philosophy:** "Clean inputs, clean outputs, no hidden state."

---

## Milestone 10: Stabilization & Cleanup (CURRENT)

**Goal:** Eliminate globals, enforce const-correctness, and complete enum-ification.

**Why:**
- **Globals:** Hidden dependencies (`action_state.c`) make the Referee pattern impossible to implement safely.
- **Const:** We need to be 100% sure which functions *read* the world vs *change* the world.
- **Enums:** Finishing the job started in Milestone 9.

**Tasks:**
1.  **Eliminate Action Globals:** Move `meterCounter`, `throwGoingOn`, etc. to `LocalGameInfo`.
2.  **Enum-ify Remaining States:** `TeamControlMode`, `TeamSide`, `ReplacementState`.
3.  **Const-Correctness:** Lock down read-only parameters.

---

## Milestone 11: The Referee Architecture (Future)

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
