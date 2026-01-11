# TODO

## ✅ Milestone 16: Structural Reorganization & Event System (Completed Jan 10, 2026)

**Phase 1 Complete!** Successfully split `GameControlFlags` into `GameEvents` + `GameControl` and fixed critical bugs.

- [x] Create `GameEvents` and `GameControl` structures
- [x] Migrate all 10 fields to new structures
- [x] Update all references (14 files)
- [x] Remove deprecated `GameControlFlags`
- [x] Add `clearFrameEvents()` mechanism
- [x] Fix critical ball physics bugs from "Organize structs" commit
- [x] Fix event system: `gameEvents.catchMade` → `gameControl.catchHasBeenMade`
- [x] Remove unused `batterStartedRunning` event
- [x] Comprehensive audit of all event patterns
- [x] Verify all tests pass (54 unit + 6 integration)

**Event System Status:** ✅ PERFECT - All patterns verified and working correctly.

## 🚧 Milestone 17: Referee Consolidation - Phase 2 (Next Steps)

**Goal:** Make referee.c the sole authority on `RefereeState` and `GameState`.

**Reference:** See `docs/REFEREE_CONSOLIDATION_PLAN.md` section "Phase 2: Referee Consolidation"

### Step 1: Simple Consolidations (Low Risk - Start Here)
- [ ] Move strike/ball counting from `game_manipulation.c` to referee (lines 116-120)
- [ ] Move `outOfBounds` flag management to referee  
- [ ] Move `endPeriod` flag setting to referee
- [ ] Consolidate all `GameState.event` setting in referee

### Step 2: Event System Expansion (Medium Risk)
- [x] Add event clearing mechanism ✅ DONE (`clearFrameEvents` in mutable_world.c)
- [x] Audit event patterns ✅ DONE (all verified correct)
- [ ] Document event lifecycle for each field
- [ ] Add more events as needed (e.g., `batterOut`, `runnerOut`)

### Step 3: Free Walk Refactor (Medium Risk)
- [x] Use `freeWalkAccepted/Rejected` events (already defined!)
- [x] Move safety grants from `action_implementation.c:227-283` to referee
- [x] Move run scoring from `action_implementation.c:233-241` to referee

### Step 4: Wounding Redesign (HIGH RISK - Last)
- [ ] Design state machine (or time-based approach) to replace frame counter
- [ ] Move all wounding logic from `game_analysis.c:177-261` to referee
- [ ] Ensure frame independence

### Step 5: GameFlowState Elimination (After wounding redesign)
- [ ] Move frame counters to static variables in game_analysis.c
- [ ] Move `ballHome` to pure function or BallInfo
- [ ] Remove GameFlowState structure

## Completed (Recent)

- [x] **Milestone 16 (Jan 10):** Structural reorganization & event system fixes
  - Split GameControlFlags into GameEvents + GameControl
  - Fixed critical ball physics bugs (missing brackets, comment-code merge)
  - Fixed event system pattern (transient vs persistent)
  - Removed unused batterStartedRunning
  - All 60 tests passing (54 unit + 6 integration)
- [x] **Milestone 15 (Jan 2026):** Referee Architecture V2 - Sequential update pipeline
- [x] **Milestone 14 (Jan 2026):** The Great Decoupling V1 - Pure/impure separation
- [x] Fix vapaataival (free walk) bug - Updated get_base_controller() logic
- [x] Create comprehensive refactoring documentation (REFEREE_CONSOLIDATION_PLAN.md)
- [x] Referee Refactor: Remove `RefereeDecisions` struct and use `Referee_Update` pipeline
- [x] Migrate `ballHome` logic out of Referee to `game_manipulation.c`
- [x] Fix referee system integration: Add `reconcile` logic to `mutable_world.c` after `Referee_Update`
- [x] Eliminate `baseControlIndex` array
- [x] Create `Referee_Analyze` (pure) and `Referee_Apply` (impure)
- [x] Fix "frame-off" bug where player runs automatically due to delayed safety grant

## Test Infrastructure (Ongoing)

- [ ] Migrate remaining legacy integration tests to full-scenario tests