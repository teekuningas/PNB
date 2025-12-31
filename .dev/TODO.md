# TODO - Current Tasks

## Milestone 7: Data Renaissance - Phase 4 (Migrate Reads)

**Strategy:** Write-Both Pattern (All writes update both old and new fields)
**Current:** Migrating read sites from legacy to new enum fields
**Next:** Delete legacy fields once all reads migrated

### Phase 4: Migrate Reads to Use New Enum Fields (In Progress)
- [ ] **Player State Reads:** Migrate from legacy flags to `state` enum
  - [ ] Audit all reads of `isOnBase`, `out`, `wounded`, `leading`, `takingFreeWalk`
  - [ ] Replace with reads of `state` enum and switch statements
  - [ ] Test after each migration
- [ ] **Base Location Reads:** Migrate from `base` to `baseId`
  - [ ] Audit all reads of `base` field
  - [ ] Replace with reads of `baseId` enum
  - [ ] Test after each migration
- [ ] **Event Reads:** Already mostly done! ✅
  - [x] `game_screen.c` uses `event` enum
  - [ ] Check if any other files read `gameInfoEvent`

### Phase 5: Delete Legacy Fields (Next)
- [ ] Remove legacy field definitions from `BattingTeamPlayerInfo`:
  - [ ] Delete `isOnBase`, `out`, `wounded`, `leading`, `takingFreeWalk`
  - [ ] Delete `base` field (keep only `baseId`)
  - [ ] Delete `gameInfoEvent` (keep only `event`)
- [ ] Remove adapter functions (unused):
  - [ ] Delete `update_player_state_from_flags()`
  - [ ] Delete `update_player_flags_from_state()`
  - [ ] Remove `state_adapter.h` and `state_adapter.c`
  - [ ] Remove includes
- [ ] Final verification
