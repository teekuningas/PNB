## Refactoring Master Plan

## Current Status: Milestone 7 - Phase 4 (Player State Reads Migrated) 🎉

**Foundation Quality:** 10.0/10 - Excellent (Write-Both Pattern & Read Migration for States Complete)
**Current Task:** Migrate base location reads and then delete legacy fields.

---

## The Write-Both Pattern Strategy (NEW APPROACH)

**Decision (2025-12-31):** Abandoned the scary bidirectional adapter pattern in favor of a simple, safe **write-both pattern**.

### What We Did
- **Every write location** updates BOTH the legacy field AND the new enum field
- **Read migration complete** for all player state flags (isOnBase, out, wounded, leading, takingFreeWalk)
- **Safe migration path** - reads are now using the new type-safe enums

### Current State
- ✅ **All writes** update both old and new fields
- ✅ **Reads migrated** for all player state flags
- ✅ **Game works perfectly** - verified with 51 tests (48 unit + 3 integration)
- 🎯 **Next step** - migrate base location reads (base -> baseId)

---

## The Mountain Climb: Migration Roadmap 🏔️

We are currently at **Phase 4: Migrate Reads (Base Locations)**.

### Base Camp: Dual State (Completed ✅)
- **Goal:** Both old flags and new Enums exist side-by-side.
- **Status:** ✅ Complete.

### Ascent Stage 1: Write-Both Pattern (Completed ✅)
- **Goal:** Every write updates BOTH old and new fields.
- **Status:** ✅ Complete.

### Ascent Stage 2: Migrate Reads (IN PROGRESS 🔄)
- **Goal:** Change read sites from old fields to new enum fields.
- **Status:** 🔄 In Progress
  - Player states: ✅ COMPLETE (All files migrated)
  - Base location: 🔄 In Progress (Audit & migration needed)
  - Events: ✅ COMPLETE (game_screen.c uses new event field)

### The Summit: Delete Legacy Fields (Next 🚩)
- **Goal:** Remove old flag fields once all reads migrated.
- **Task:** Delete `out`, `wounded`, `isOnBase`, `leading`, `takingFreeWalk`, `base`, `gameInfoEvent`.
- **Result:** Clean, type-safe domain model with only enum fields.

---

## Beyond the Summit: The View (Milestone 8) 🔭

Once we reach the summit (Milestone 7 complete), we unlock **Milestone 8: The Referee Architecture**.

**The New Reality:**
1.  **The Referee Layer:**
    - A dedicated system that analyzes the physical world and outputs **Abstract State** (Outs, Runs) and **Permissions** ("Can Pitch", "Can Bat").
2.  **Explicit Permissions:**
    - Player code no longer calculates *if* it can do something. It simply checks the Referee's permissions.
3.  **Synchronous "Breathing" Loop:**
    - Input -> Physics -> Referee (Analysis) -> Logic/AI -> Render.
4.  **Snapshotting & Replay:**
    - Clean state enables instant replay and rewinding time.

**Current Focus:** Finish **The Summit** (Milestone 7) so we can build the Referee.

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
- [x] **Phase 2:** Data structure migration (Add new enum fields)
- [x] **Phase 3:** Write-Both Pattern (All writes update both fields)
- [ ] **Phase 4:** Migrate Reads (Change reads to use new fields) **<-- DOING**
- [ ] **Phase 5:** Delete legacy fields **<-- NEXT**

### Medium Priority (Milestone 8 - Functional Dataflow)
- [ ] Global `StateInfo` dependency in update loops
- [ ] Overlay/HUD rendering extraction
- [ ] Animation state machine extraction

---

## Decision Log

### 2025-12-31: The "Write-Both" Pattern
**Decision:** Abandoned bidirectional adapter pattern in favor of write-both.
**Rationale:**
- Bidirectional adapter was scary - easy to overwrite data accidentally
- Write-both is simple, obvious, and safe
- Local reasoning at each write site
- No timing dependencies or order-sensitive sync calls
- Safe migration of reads can happen gradually

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