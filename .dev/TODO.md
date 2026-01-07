# TODO - Pending Tasks

## ✅ Milestone 14: The Great Decoupling (COMPLETE)

**Goal:** Split "messy" coordinators into Pure Logic (Query) and State Mutation (Apply).

### Phase 1: Game Analysis Split (COMPLETE)
- [x] Create `src/game/rules_pure/referee.h` and `.c` to house the pure `Referee_Analyze` function.
- [x] Refactor `game_analysis.c` to use `Referee_Analyze` + `Referee_Apply`.
- [x] Create `RefereeDecisions` struct in `globals.h` to capture the output of analysis (outs, runs, wounds).

### Phase 2: Action Implementation Split (PENDING)
- [ ] Audit `action_implementation.c` to identify pure physics calculations.
- [ ] Move pure physics into `src/game/actions_pure/` or `src/physics/`.
- [ ] Create `PhysicsParams` struct to pass data between pure calculations and application.

### Phase 3: Rule Logic Implementation (New Findings)
- [ ] **Implement §31 (Fielder Positioning):**
    - [ ] Create `rules_pure/positioning_rules.c`.
    - [ ] Implement `check_fielder_positions(StateInfo)` to verify bounds.
    - [ ] Hook into `pitching_system.c` to trigger Free Walk if violated.
    - [ ] Enable `tests/integration/test_scenario_fielder_positioning.c`.

### Phase 4: State Validation (COMPLETE)
- [x] Create `src/core/state_validator.c` and `.h`.
- [x] Implement `validate_game_state(StateInfo*)`.
    - [x] Check unique bases (no two players on same base).
    - [x] Check `baseControlIndex` consistency.
- [x] Implement `dump_game_state_to_json(StateInfo*, const char* filename)`.
- [x] Hook validator into `gameAnalysis` debug/test builds via `--debug-state` flag.

### Phase 5: Verification (COMPLETE)
- [x] Create `tests/test_rules_referee.c` to unit test the new `Referee_Analyze` function with mock states.
- [x] Verify that `make test` and `make main` still pass.

---

## 📅 Upcoming Milestones

### Milestone 15: The "User Intent" Phase
- **Goal:** Decouple Input from Action.
- **Concept:** Input generates an `Intent` (e.g., `INTENT_SWING_BAT`). The Engine consumes `Intent`.

### Milestone 16: Comprehensive Rule Verification
- **Goal:** 100% Audit of `docs/SAANNOT.md`.
- **Method:** Create `test_scenario_*.c` for every major rule section.