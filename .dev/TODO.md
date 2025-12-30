# TODO - Current Tasks

- [x] Create directory tests/integration/ (commit: 8cc7fcf)
- [x] Create `tests/integration/integration_runner.c` (copy of test_runner.c adapted for integration tests) (commit: 0d430a0)
- [x] Create `tests/integration/fixtures.c` and `.h`: (commit: 937480f)
    - Implement `setup_test_state()` to allocate `StateInfo` + sub-structs
    - Mock `TeamData` (dummy players) and `FieldPositions` (standard coordinates)
    - Provide helpers like `setup_runner_at_first_base` using `initializeGameFromMenu`
- [x] Create `tests/integration/test_scenario_outs.c` to test "Runner Forced Out" behavior using current flags (commit: c1b279d)
- [ ] Create `tests/integration/test_scenario_runs.c` to test "Run Scored" behavior
- [ ] Create `tests/integration/test_scenario_wounded.c` to test "Wounded" behavior
- [ ] Add `integration_test` target to `Makefile` to run these new tests

