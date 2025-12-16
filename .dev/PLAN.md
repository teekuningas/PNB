# Refactoring Master Plan

## Immediate Goals (Milestone 5: Logic Purification)

**Goal:** Analyze the `actions_messy` and `ai_messy` modules, identify pure logic (math, rules, state-independent calculations), and move them to `src/game/actions_pure/` or `src/game/ai_pure/`.

### Step 12: Analyze & Purify Batting
- [x] Review `src/game/actions_messy/batting_system.c`.
- [x] Extract physics/math calculations (e.g., hit trajectory) to `src/game/actions_pure/batting_physics.c`.
- [x] **Create unit tests for `batting_physics.c`.**
- [ ] Isolate state modifications from logic where possible.

### Step 13: Analyze & Purify Pitching
- [x] Review `src/game/actions_messy/pitching_system.c`.
- [x] Extract physics/math calculations (e.g., hit trajectory) to `src/game/actions_pure/pitching_physics.c`.
- [x] **Create unit tests for `pitching_physics.c`.**
- [ ] Isolate state modifications from logic where possible.

## Milestone 5.5: The Vantage Point (Zen Harmony)
**Goal:** Pause, breathe, and stabilize. Ensure the "Pure vs. Messy" separation is clean, documented, and the codebase feels safe.
- [x] **Architectural Review:** Update `docs/ARCHITECTURE_MAPS.md` to reflect the new `actions_pure` and `ai_pure` structures.
- [x] **Test Suite Review:** Ensure we haven't over-tested volatile code. Verify that `actions_pure` tests are solid and fast.
- [x] **Messiness Isolation Check:** Verify that `actions_messy` files are strictly coordinators/state-mutators and contain NO complex math/rules.
- [x] **Cleanup:** Remove any temporary comments, unused includes, or transitional artifacts.
- [x] **Pitching AI Purification:** Extract the hidden AI logic from `pitching_system.c` to `ai_pure/pitching_ai_strategy.c` (caught during review).
- [x] **RNG Purification:** Refactor Random Number Generation to use explicit seed passing (`rng_seed`) throughout the game loop, removing hidden global state dependencies in `StateInfo` and ensuring deterministic replays/testing.

## Milestone 6: Rules Engine Extraction (The Judge)
**Goal:** Extract the complex rules logic (outs, runs, strikes) from `game_analysis.c` into a pure `src/game_logic/rules/` module.
- [ ] Analyze `game_analysis.c` to identify rule evaluation vs. state mutation.
- [ ] Create `src/game/rules_pure/` directory.
- [ ] Extract "Out" detection logic (e.g., `is_force_out`, `is_tag_out`).
- [ ] Extract "Scoring" logic (e.g., `calculate_runs`).
- [ ] Extract "Strike/Ball" logic.
- [ ] **Crucial:** Ensure these new functions take *only* the necessary data (structs), not the whole `StateInfo`.

## Testing Strategy
- **Pure Functions (High Priority):** Every time logic is extracted to a `_pure` module (Milestone 5+), it **must** be accompanied by unit tests. These functions take simple inputs and return outputs, making them ideal for testing.
- **Integration Tests (Medium Priority):** As the "messy" layers become thinner coordinators, we will write tests that initialize a minimal `StateInfo`, call the coordinator, and check specific state changes.
- **Regression via Serialization (Long Term):** The State Serialization tool (Future Improvements) will allow us to snapshot a game state and run logic against it to verify fixes.

## Future Improvements & Tooling
- [ ] **State Serialization (Save/Debug Dump)**: Implement a system to serialize `StateInfo` (specifically `LocalGameInfo` and `GlobalGameInfo`) to a file. This will aid in debugging bugs like the "double occupancy" issue by allowing exact state reproduction.
- [ ] **Game Loop Functional Dataflow**: Refactor the main game loop (`gameManipulation`, `actionImplementation`) to follow the Menu's "Functional Dataflow" pattern. Break `LocalGameInfo` into distinct sub-states (Physics, Rules, AI) and pass only the relevant data explicitly to update functions, removing the reliance on the global `StateInfo` god-object.
