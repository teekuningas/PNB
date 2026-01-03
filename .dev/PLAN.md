## Refactoring Master Plan

## Current Status: Milestone 7.5 COMPLETE ✅ → Milestone 8 (The Great Narrowing)

**Foundation Quality:** 10.0/10 - Excellent Type-Safe Domain State
**Current Focus:** Function Signature Narrowing
**Philosophy:** "Write-only what you need" - Functions should not see the whole world.

---

## Milestone 7.5: Data Structure Cleanup - COMPLETE ✅

**Achievement (2026-01-03):** Successfully decomposed `GameAnalysisInfo` God Object!

### What We Accomplished
- ✅ Created 6 focused structs: `GameState`, `GameControlFlags`, `WoundingState`, `CameraState`, `PlayerCounters`, `GameModeState`
- ✅ Migrated all fields from `GameAnalysisInfo`
- ✅ Deleted `GameAnalysisInfo` struct
- ✅ All 51 unit tests passed

---

## Milestone 8: The Great Narrowing (CURRENT - 1 week)

**Goal:** Refactor function signatures to take specific structs instead of `StateInfo*`.

**Why:**
- Prove we understand dependencies.
- Prevent accidental side effects (e.g., movement logic changing the score).
- Simplify unit tests (no need to mock `StateInfo`).

**Tasks:**
1.  **Narrow Movement Logic:** `moveToTarget`, `runToTarget` → Take `LocalGameInfo*` or `PlayerInfo*`.
2.  **Narrow Ball Logic:** `updateBall`, `genericSlingBall` → Take `BallInfo*` and `PlayerInfo*`.
3.  **Narrow AI Strategy:** Verify `*_ai_strategy.c` use `const GameState*`.
4.  **Test Suite Upgrade:** Refactor tests to setup only required structs.

---

## Milestone 9: Type Safety & State Machines (Next)

**Goal:** Replace "int flags" with strong Enums and State Machines.

**Why:**
- `player.model = 12` is unreadable.
- `pitchGoingOn` + `battingGoingOn` is fragile.

**Tasks:**
1.  **Animation Enums:** Replace magic numbers (0-16) with `PlayerAnimationModel`.
2.  **Pitch Cycle State Machine:** Replace loose flags in `PlayerRelatedActionInfo` with `PitchCycleState`.
3.  **Action Flag Types:** Investigate typing `ActionFlags`.

---

## Milestone 10: The Referee Architecture (Future)

**Goal:** Implement the high-level logic arbitrator.

**The Vision:**
- **Referee Layer:** Analyzes physical world → Outputs Abstract State & Permissions.
- **Benefits:** Explicit permissions, synchronous breathing, replayability.

**Why Wait?**
- Needs clean data (Milestone 7.5 ✅).
- Needs narrow functions (Milestone 8).
- Needs strong types (Milestone 9).

---

## Completed Milestones

### ✅ Milestone 7.5: Data Structure Cleanup (COMPLETE 2026-01-03)
- Decomposed `GameAnalysisInfo` into `GameState`, `GameControlFlags`, etc.
- Migrated 34+ fields.
- 51 Tests passing.

### ✅ Milestone 7: Data Renaissance (COMPLETE 2026-01-01)
- Eliminated ALL legacy state flags.
- Type-safe enums: `PlayerUnitState`, `BaseID`.

### ✅ Milestone 6: Rules Engine Extraction
- Extracted pure rules logic.

---

## Decision Log

### 2026-01-03: The Three-Step Climb
**Decision:** Split future work into Narrowing (M8) -> Type Safety (M9) -> Referee (M10).
**Rationale:**
- Doing everything at once is too risky.
- Narrowing validates the data structure changes immediately.
- Type safety makes the code readable before we write complex Referee logic.

### 2026-01-01: Data First, Then Architecture
**Decision:** Do Milestone 7.5 (data cleanup) BEFORE Milestone 8 (Referee pattern)
**Rationale:**
- Data structures shape architecture.
- Can't build clean architecture on messy data.

---
