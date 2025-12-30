# TODO - Current Tasks

### Phase 1: Integration Safety Net (Completed)
- [x] Create directory tests/integration/ (commit: 8cc7fcf)
- [x] Create `tests/integration/integration_runner.c` (commit: 0d430a0)
- [x] Create `tests/integration/fixtures.c` and `.h` (commit: 937480f)
- [x] Create `tests/integration/test_scenario_outs.c` (commit: c1b279d)
- [x] Create `tests/integration/test_scenario_runs.c` (commit: 22c8606)
- [x] Create `tests/integration/test_scenario_wounded.c` (commit: 8804b65, fixed in recent commits)
- [x] Add `integration_test` target to `Makefile` (commit: 85122e2)

### Phase 2: Data Structure Migration (Next)
- [x] Define `PlayerUnitState` and `BaseID` enums in `src/include/globals.h` (alongside existing structs)
- [x] Add `PlayerUnitState state` and `BaseID baseId` fields to `BattingTeamPlayerInfo` struct in `globals.h`
- [x] Create `src/game/state_adapter.c` and `.h`:
    - Implement `update_player_state_from_flags(PlayerInfo* p)` (Flags -> Enum)
    - Implement `update_player_flags_from_state(PlayerInfo* p)` (Enum -> Flags)
- [ ] Hook up `state_adapter` in `src/game/game_analysis.c`:
    - Add `#include "state_adapter.h"` at the top
    - In `gameAnalysis` function, at the very beginning (after `initLocals` check), add a loop to sync Enum from Flags:
      ```c
      for (int i = 0; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
          update_player_state_from_flags(&stateInfo->localGameInfo->playerInfo[i]);
      }
      ```
    - In `gameAnalysis` function, at the very end, add a loop to sync Flags from Enum:
      ```c
      for (int i = 0; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
          update_player_flags_from_state(&stateInfo->localGameInfo->playerInfo[i]);
      }
      ```
- [ ] Update `Makefile` to include `game/state_adapter.o`:
    - Add `game/state_adapter.o` to `_OBJ` (for the main app)
    - Add `game/state_adapter.o` to `_INT_TEST_LOGIC_OBJ` (for logic tests)
    - Add `game/state_adapter.o` to `_INT_TEST_OBJ` (for integration tests)
- [ ] Run `devenv shell make integration_test` and verify all tests still pass
