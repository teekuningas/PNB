# TODO - Pending Tasks

## 🚧 Milestone 14: The Great Decoupling

**Goal:** Split "messy" coordinators into Pure Logic (Query) and State Mutation (Apply).

### Phase 1: Game Analysis Split
- [ ] Create `src/game/rules_pure/referee.h` and `.c` to house the pure `Referee_Analyze` function.
- [ ] Refactor `game_analysis.c` to use `Referee_Analyze`.
- [ ] Create `RefereeDecisions` struct in `globals.h` to capture the output of analysis (outs, runs, wounds).

### Phase 2: Action Implementation Split
- [ ] Audit `action_implementation.c` to identify pure physics calculations.
- [ ] Move pure physics into `src/game/actions_pure/` or `src/physics/`.
- [ ] Create `PhysicsParams` struct to pass data between pure calculations and application.

### Phase 3: Rule Logic Implementation (New Findings)
- [ ] **Implement §31 (Fielder Positioning):**
    - [ ] Create `rules_pure/positioning_rules.c`.
    - [ ] Implement `check_fielder_positions(StateInfo)` to verify bounds.
    - [ ] Hook into `pitching_system.c` to trigger Free Walk if violated.
    - [ ] Enable `tests/integration/test_scenario_fielder_positioning.c`.

### Phase 4: State Validation (New)
- [ ] Create `src/core/state_validator.c` and `.h`.
- [ ] Implement `validate_game_state(StateInfo*)`.
    - [ ] Check unique bases (no two players on same base).
    - [ ] Check `playerCounters` vs actual player count.
    - [ ] Check `baseControlIndex` consistency.
- [ ] Implement `dump_game_state_to_json(StateInfo*, const char* filename)`.
- [ ] Hook validator into `gameAnalysis` debug/test builds.

### Phase 5: Verification
- [ ] Create `tests/test_referee.c` to unit test the new `Referee_Analyze` function with mock states.
- [ ] Verify that `make test` and `make main` still pass.
