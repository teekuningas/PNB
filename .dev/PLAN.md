## Refactoring Master Plan

## Current Status: Milestone 7 - Phase 5 (Write-Both Pattern Complete) 🎉

**Foundation Quality:** 10.0/10 - Excellent (Write-Both Pattern Implemented)
**Current Task:** Migrate reads from legacy flags to new enum-based fields.

---

## The Write-Both Pattern Strategy (NEW APPROACH)

**Decision (2025-12-31):** Abandoned the scary bidirectional adapter pattern in favor of a simple, safe **write-both pattern**.

### What We Did
- **Every write location** now updates BOTH the legacy field AND the new enum field
- **No adapter sync calls needed** - data stays consistent by construction
- **Safe migration path** - reads can be migrated independently without risk

### Current State
- ✅ **All writes** update both old and new fields
- ✅ **Game works perfectly** - no broken functionality
- ⚠️ **Reads are mixed** - some code reads old fields, some reads new
- 🎯 **Next step** - gradually migrate reads to use new fields

---

## The Mountain Climb: Migration Roadmap 🏔️

We are currently at **Phase 5: Migrate Reads** (After Summit).

### Base Camp: Dual State (Completed ✅)
- **Goal:** Both old flags and new Enums exist side-by-side.
- **Status:** ✅ Complete. Both systems present.

### Ascent Stage 1: Write-Both Pattern (Completed ✅)
- **Goal:** Every write updates BOTH old and new fields.
- **Status:** ✅ Complete. All writes keep both fields in sync.
- **Safety:** Data consistency guaranteed by construction.

### Ascent Stage 2: Migrate Reads (IN PROGRESS 🔄)
- **Goal:** Change read sites from old fields to new enum fields.
- **Status:** 🔄 In Progress
  - Player states: Mix of old and new reads
  - Base location: Mostly old `base`, some new `baseId` (game_screen.c)
  - Events: New `event` field in game_screen.c, old elsewhere
- **Safety:** Safe to migrate one read at a time - writes keep both in sync.

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