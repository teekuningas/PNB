# TODO

## 🚧 Milestone 17: Referee Consolidation - Phase 2

**Goal:** Make referee.c the sole authority on `RefereeState` and `HalfInningState`.

### Step 1: Simple Consolidations
- [x] Move strike/ball counting from `game_manipulation.c` to referee ✅ DONE
- [x] Move `strikes` and `balls` increments to `referee.c` ✅ DONE
- [ ] Move `outOfBounds` flag management to referee (currently scattered)
- [ ] Move `endPeriod` flag setting to referee (currently scattered)
- [ ] Consolidate all `HalfInningState.event` setting in referee

### Step 2: Event System Expansion
- [x] Add event clearing mechanism ✅ DONE
- [x] Audit event patterns ✅ DONE
- [ ] Document event lifecycle for each field (in `docs/GAME_LOOP_REFERENCE.md`)
- [ ] Add more events as needed (`batterOut`, `runnerOut`)

### Step 3: Free Walk Refactor
- [x] Use `freeWalkAccepted/Rejected` events ✅ DONE
- [x] Move safety grants and run scoring to referee ✅ DONE

### Step 4: Wounding Redesign (HIGH RISK)
- [ ] Design state machine (or time-based approach) to replace frame counter in `game_analysis.c`
- [ ] Move all wounding logic from `game_analysis.c:177-261` to referee
- [ ] Ensure frame independence (use `woundingCatchTimer` in RefereeState)

### Step 5: Eliminate Redundant State (batterIndex)
- [ ] Implement `get_active_batter_index()` helper function
- [ ] Update `StateValidator` to enforce `BASE_HOME` <-> `PLAYER_STATE_AT_BAT` consistency
- [ ] Replace `pII.batterIndex` reads with helper function
- [ ] Remove `pII.batterIndex` from struct and write locations

### Step 6: Final Cleanup
- [ ] Move `ballHome` to pure function or BallInfo
- [ ] Remove `GameFlowState` structure
- [ ] **Delete `game_analysis.c`** (Move initialization to `game_setup.c`)

## Test Infrastructure (Ongoing)
- [ ] Migrate remaining legacy integration tests to full-scenario tests
