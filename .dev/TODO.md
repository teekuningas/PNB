# TODO - Current Tasks

## Milestone 7.5: Data Structure Cleanup (CURRENT)

**Philosophy:** Data shapes architecture. Clean the foundation before building the Referee.

**Goal:** Separate domain state from control state in our data structures.

---

### Phase 0: Data Structure Audit (COMPLETE ✅)

- [x] **Audit GameAnalysisInfo** (Done 2026-01-02)
  - [x] List all 40+ fields
  - [x] Document purpose of each field
  - [x] Classify each: Domain, Control, Camera, or Eliminable?
  - [x] Draw dependency diagram
  - [x] Create `docs/DATA_STRUCTURE_AUDIT.md`

---

### Phase 1: Extract PlayerRuntimeState (COMPLETE ✅)

- [x] Create `PlayerRuntimeState` struct in `src/include/globals.h`, add `playerRuntime` array to `LocalGameInfo`, and initialize it in `src/game/common_logic.c`

- [x] **Migrate control flags (One by one)**
  - [x] **Migrate `arrivedToBase`** (Optimization flag)
  - [x] **Migrate `woundedApply`** (Deferred execution)
  - [x] **Migrate `passedPathPoint`** (State machine)
  - [x] **Migrate `goingForward`** (Direction)
  - [x] **Migrate `hasMadeRunOnThirdBase`** (Guard)

**Result:** `BattingTeamPlayerInfo` contains ONLY domain state (serializable "truth").

---

### Phase 2: Split GameAnalysisInfo God Object (COMPLETE ✅)

- [x] **Create focused structs in `globals.h`** (Done 2026-01-03)
  - [x] `GameState` (outs, strikes, balls, runs, event)
  - [x] `GameControlFlags` (pause, initLocals, etc.)
  - [x] `WoundingState` (woundingCatch, handled)
  - [x] `CameraState` (camera flags, targetPoint)
  - [x] `PlayerCounters` (battingTeamPlayers, nonJokers, jokers)
  - [x] `GameModeState` (pairCounter, runOfHonor, forceNextPair)

- [x] **Add new structs to `LocalGameInfo`**

- [x] **Migrate fields group by group**
  - [x] Migrate `GameState` fields
  - [x] Migrate `GameControlFlags`
  - [x] Migrate `WoundingState`
  - [x] Migrate `CameraState`
  - [x] Migrate `PlayerCounters`
  - [x] Migrate `GameModeState`

- [x] **Remove `GameAnalysisInfo` struct**

- [x] **Test continuously** (All 51 tests passing)

**Result:** Focused, single-responsibility structs.

---

### Phase 3: Stabilize & Document (CURRENT 🎯)

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
