# TODO - Current Tasks

## Milestone 7.5: Data Structure Cleanup (CURRENT)

**Philosophy:** Data shapes architecture. Clean the foundation before building the Referee.

**Goal:** Separate domain state from control state in our data structures.

---

### Phase 0: Data Structure Audit (START HERE - 1 day)

- [ ] **Audit GameAnalysisInfo**
  - [ ] List all 40+ fields
  - [ ] Document purpose of each field
  - [ ] Classify each: Domain, Control, Camera, or Eliminable?
  - [ ] Draw dependency diagram
  - [ ] Create `docs/DATA_STRUCTURE_AUDIT.md`

**Expected Discovery:** Some flags can be eliminated entirely, not just moved!

---

### Phase 1: Extract PlayerRuntimeState (2 days)

- [ ] **Create new struct**
  - [ ] Define `PlayerRuntimeState` in globals.h
  - [ ] Add `PlayerRuntimeState runtime[24]` to LocalGameInfo
  
- [ ] **Migrate control flags from BattingTeamPlayerInfo**
  - [ ] Move `arrivedToBase` (simple optimization flag)
  - [ ] Move `woundedApply` (deferred execution queue)
  - [ ] Move `passedPathPoint` (state machine variable)
  - [ ] Move `goingForward` (direction tracking)
  - [ ] Move `hasMadeRunOnThirdBase` (guard flag)
  - [ ] Run tests after EACH migration

**Result:** Clean BattingTeamPlayerInfo with domain state only

---

### Phase 2: Split GameAnalysisInfo God Object (3-4 days)

- [ ] **Create focused structs**
  - [ ] Define `GameState` (outs, strikes, balls, runs)
  - [ ] Define `GameControlFlags` (pause, initLocals, etc.)
  - [ ] Define `WoundingState` (wounding system state)
  - [ ] Define `CameraState` (camera/UI state)
  - [ ] Define `PlayerCounters` (player tracking)

- [ ] **Migrate fields group by group**
  - [ ] Migrate game state counters
  - [ ] Migrate control flags
  - [ ] Migrate wounding state
  - [ ] Migrate camera state
  - [ ] Migrate player counters
  - [ ] Update ALL references (mechanical but large)

- [ ] **Test continuously**
  - [ ] Run unit tests after each group
  - [ ] Run integration tests after each group
  - [ ] Manual playtest at end

**Result:** Focused, single-responsibility structs

---

### Phase 3: Stabilize & Document (1-2 days)

- [ ] **Update documentation**
  - [ ] Update `docs/DATA_STRUCTURE_AUDIT.md` with new structure
  - [ ] Draw new hierarchy diagram
  - [ ] Document each struct's purpose
  - [ ] Add usage examples

- [ ] **Testing & verification**
  - [ ] Full unit test suite
  - [ ] Full integration test suite
  - [ ] Manual playthrough (full game)
  - [ ] Verify save/load still works

- [ ] **Celebrate!**
  - [ ] Create `docs/MILESTONE7.5_COMPLETE.md`
  - [ ] Update PLAN.md
  - [ ] Commit all changes

**Result:** Clean, documented, tested foundation for Milestone 8

---

## Next Up: Milestone 8 - The Referee Architecture

Will be done AFTER data cleanup is complete. The Referee pattern will emerge naturally from clean data structures.

---

## Completed ✅

### Milestone 7: Data Renaissance (Complete 2026-01-01)
- [x] Eliminated ALL legacy state flags
- [x] Type-safe enums (PlayerUnitState, BaseID, GameEventType)
- [x] Deleted state_adapter.c/h
- [x] All 51 unit + 5 integration tests passing
