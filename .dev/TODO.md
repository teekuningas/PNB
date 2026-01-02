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

### Phase 1: Extract PlayerRuntimeState (NEXT UP)

- [ ] **Create new struct**
  - [ ] Define `PlayerRuntimeState` in `src/include/globals.h`
    ```c
    typedef struct _PlayerRuntimeState {
        int arrivedToBase;
        int woundedApply;
        int passedPathPoint;
        int goingForward;
        int hasMadeRunOnThirdBase;
    } PlayerRuntimeState;
    ```
  - [ ] Add `PlayerRuntimeState playerRuntime[2*PLAYERS_IN_TEAM + JOKER_COUNT];` to `LocalGameInfo` in `globals.h` (parallel to playerInfo)
  - [ ] Initialize `playerRuntime` (likely in `game_setup.c` or where `playerInfo` is cleared)
  
- [ ] **Migrate control flags (One by one)**
  - [ ] **Migrate `arrivedToBase`** (Optimization flag)
    - [ ] Replace `bTPI.arrivedToBase` with `playerRuntime[i].arrivedToBase`
    - [ ] Remove from `BattingTeamPlayerInfo`
    - [ ] Verify build & tests
  - [ ] **Migrate `woundedApply`** (Deferred execution)
    - [ ] Replace `bTPI.woundedApply` with `playerRuntime[i].woundedApply`
    - [ ] Remove from `BattingTeamPlayerInfo`
    - [ ] Verify build & tests
  - [ ] **Migrate `passedPathPoint`** (State machine)
    - [ ] Replace `bTPI.passedPathPoint` with `playerRuntime[i].passedPathPoint`
    - [ ] Remove from `BattingTeamPlayerInfo`
    - [ ] Verify build & tests
  - [ ] **Migrate `goingForward`** (Direction)
    - [ ] Replace `bTPI.goingForward` with `playerRuntime[i].goingForward`
    - [ ] Remove from `BattingTeamPlayerInfo`
    - [ ] Verify build & tests
  - [ ] **Migrate `hasMadeRunOnThirdBase`** (Guard)
    - [ ] Replace `bTPI.hasMadeRunOnThirdBase` with `playerRuntime[i].hasMadeRunOnThirdBase`
    - [ ] Remove from `BattingTeamPlayerInfo`
    - [ ] Verify build & tests

**Result:** `BattingTeamPlayerInfo` contains ONLY domain state (serializable "truth").

---

### Phase 2: Split GameAnalysisInfo God Object (Planned)

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

### Phase 3: Stabilize & Document (Planned)

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