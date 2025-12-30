# TODO - Current Tasks

- [x] Create directory tests/integration/ (commit: 8cc7fcf)
- [ ] Create `tests/integration/integration_runner.c` (copy of test_runner.c adapted for integration tests)
- [ ] Create `tests/integration/fixtures.c` and `.h` to help setup specific `StateInfo` scenarios (e.g. `setup_runner_at_first_base`)
- [ ] Create `tests/integration/test_scenario_outs.c` to test "Runner Forced Out" behavior using current flags
- [ ] Create `tests/integration/test_scenario_runs.c` to test "Run Scored" behavior
- [ ] Create `tests/integration/test_scenario_wounded.c` to test "Wounded" behavior
- [ ] Add `integration_test` target to `Makefile` to run these new tests

