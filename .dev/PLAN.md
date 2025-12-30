# Refactoring Master Plan

## Current Status: Milestone 7 - Phase 2 (Data Migration) 🔄

**Foundation Quality:** 8.5/10 - Production Ready
**Current Task:** Setting up the "Dual State" adapter (Enum <-> Flags).

---

## The Mountain Climb: Migration Roadmap 🏔️

We are currently at **Base Camp** (Phase 2). Here is the path to the Summit (removing the old flags).

### Base Camp: Dual State Adapter (Current)
- **Goal:** The system maintains both old flags (`out`, `wounded`) and new Enums (`PlayerState`) in perfect sync.
- **Status:** Adapter created, hooking up to Game Loop now.
- **Verification:** Integration tests pass transparently.

### Ascent Stage 1: Rendering & UI Migration (Next)
- **Goal:** Update the "Eyes" of the game to see the new Enums.
- **Tasks:**
    - Refactor `src/game/game_screen.c` to read `p->state` instead of flags.
    - Refactor `src/renderer/player_renderer.c` to read `p->state`.
- **Safety:** The game logic still runs on flags, so physics/rules remain untouched.
- **Benefit:** Visual verification that Enums are correct.

### Ascent Stage 2: Logic & AI Migration
- **Goal:** Update the "Brains" of the game to think in Enums.
- **Tasks:**
    - Refactor `src/game/common_logic.c` (Animation states).
    - Refactor `src/game/ai_messy/` and `src/game/ai_pure/`.
    - Refactor `src/game/rules_pure/`.
- **Safety:** The Adapter ensures flags are still updated for any remaining legacy code.

### The Summit: Deletion 🚩
- **Goal:** Remove the old flag fields (`out`, `wounded`, `isOnBase`, etc.) from `BattingTeamPlayerInfo`.
- **Task:** Delete the fields and the Adapter's "Flags <- Enum" sync direction.
- **Result:** A clean, type-safe domain model.

---

## Beyond the Summit: The View (Milestone 8) 🔭

Once we reach the summit (Milestone 7 complete), we unlock **Milestone 8: Functional Dataflow**.

**The New Reality:**
1.  **Synchronous "Breathing" Loop:**
    - Because state is clean (no hidden flags), we can implement `State new = update(old, input)`.
2.  **Snapshotting & Replay:**
    - Clean structs = easy serialization. We can save the *exact* frame state and reload it.
    - Enables "Instant Replay" features and powerful debugging (rewind time).
3.  **Parallelism Potential:**
    - Pure functions + clean state = potential to run AI or Physics on separate threads (future).
4.  **Modding/Scripting:**
    - A clean data model is the first step towards exposing game logic to Lua/Python scripts.

**Current Focus:** finish **Base Camp** setup so we can start the ascent.

---

## Completed Milestones

### ✅ Milestone 6: Rules Engine Extraction
- Extracted outs, runs, strikes to `rules_pure/`
- Comprehensive audit completed (1 bug found/fixed)
- All game "brains" now pure and tested

### ✅ Milestone 5: Logic Purification
- Extracted physics and AI to pure functions
- Created comprehensive unit tests

---

## Technical Debt Tracker

### High Priority (Milestone 7 - Data Renaissance)
- [x] **Phase 0:** Data model audit & design
- [x] **Phase 1:** Integration test foundation
- [ ] **Phase 2:** Data structure migration (Adapter setup) **<-- DOING**
- [ ] **Phase 3:** Refactor Rendering/UI to use Enums
- [ ] **Phase 4:** Refactor Logic/AI to use Enums
- [ ] **Phase 5:** Delete old flags

### Medium Priority (Milestone 8 - Functional Dataflow)
- [ ] Global `StateInfo` dependency in update loops
- [ ] Overlay/HUD rendering extraction
- [ ] Animation state machine extraction

---

## Decision Log

### 2025-12-30: The "Climb" Strategy
**Decision:** Break migration into Rendering first, then Logic.
**Rationale:**
- Rendering is "read-only" relative to game state. Safer to migrate first.
- Visual feedback provides immediate confirmation of Adapter correctness.
- Logic migration is higher risk, so it comes second.

### 2025-12-18: Data-First Strategy
**Decision:** Don't add enums to messy structures. Clean first.

---

*For detailed milestone achievements, see `docs/MILESTONE6_COMPLETE.md`*