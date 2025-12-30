# TODO - Current Tasks

- [x] Create directory tests/integration/ (commit: 8cc7fcf)
- [x] Create `tests/integration/integration_runner.c` (copy of test_runner.c adapted for integration tests) (commit: 0d430a0)
- [x] Create `tests/integration/fixtures.c` and `.h`: (commit: 937480f)
    - Implement `setup_test_state()` to allocate `StateInfo` + sub-structs
    - Mock `TeamData` (dummy players) and `FieldPositions` (standard coordinates)
    - Provide helpers like `setup_runner_at_first_base` using `initializeGameFromMenu`
- [x] Create `tests/integration/test_scenario_outs.c` to test "Runner Forced Out" behavior using current flags (commit: c1b279d)
- [x] Create `tests/integration/test_scenario_runs.c` to test "Run Scored" behavior (commit: 22c8606)
- [x] Fix failing `test_runner_wounded_if_off_base_when_ball_caught` in `tests/integration/test_scenario_wounded.c` (commit: bb8bf86):
    - Set `state->localGameInfo->gAI.woundingCatch = 1`
    - Set `state->localGameInfo->gAI.woundingCatchHandled = 0`
    - Set `state->localGameInfo->pII.hasBallIndex = 14` (simulates fielder holding the ball)
    - Ensure `runner->bTPI.isOnBase = 0` and `runner->bTPI.base` is NOT `runner->bTPI.originalBase` (or `isOnBase = 0`)
    - Call `gameAnalysis(state, NULL, &seed)` in a loop (at least 60 times) to pass the `WOUNDING_CATCH_THRESHOLD`
    - Verify `bTPI.wounded == 1` at the end
- [x] Add `integration_test` target to `Makefile` to run these new tests (commit: 85122e2)
