# Refactoring Master Plan

## Immediate Goals (Milestone 5: Logic Purification)

**Goal:** Analyze the `actions_messy` and `ai_messy` modules, identify pure logic (math, rules, state-independent calculations), and move them to `src/game/actions_pure/` or `src/game/ai_pure/`.

### Step 12: Analyze & Purify Batting
- [x] Review `src/game/actions_messy/batting_system.c`.
- [ ] Extract physics/math calculations (e.g., hit trajectory) to `src/game/actions_pure/batting_physics.c`.
- [ ] **Create unit tests for `batting_physics.c`.**
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
- [ ] **Pitching AI Purification:** Extract the hidden AI logic from `pitching_system.c` to `ai_pure/pitching_ai_strategy.c` (caught during review).

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

## Completed History

### Milestone 4: The Big Split (Completed)
- [x] **Action System Scaffold**
    - [x] Created `src/game/actions_messy/` and `src/game/actions_pure/`.
- [x] **Extract Pitching System**
    - [x] Moved pitching logic to `pitching_system.c`.
- [x] **Extract Batting System**
    - [x] Moved batting logic to `batting_system.c`.
- [x] **Extract Throwing & Fielding**
    - [x] Moved throwing logic to `throwing_system.c`.
- [x] **Extract AI Logic**
    - [x] Moved catching AI to `catching_ai.c`.
    - [x] Moved batting AI to `batting_ai.c`.
    - [x] Refactored `action_implementation.c` to be a lightweight coordinator.

### Milestone 3: Renderer Decoupling (Completed Early!)
- [x] **Renderer Decoupling**
    - [x] Created `src/renderer/` module.
    - [x] Moved `player.c` rendering to `src/renderer/player_renderer.c`.
    - [x] Moved `ball.c` rendering to `src/renderer/ball_renderer.c`.
    - [x] Cleaned up OpenGL calls from `player.c` and `ball.c`.

### Milestone 2: Physics Isolation (Completed)
- [x] **Step 3-6: Physics**
    - [x] Extracted `physics/ball_physics.c` and `collision.c`.
    - [x] Integrated into `game_manipulation.c`.

### Milestone 1: Core Utilities (Completed)
- [x] **Step 0-2: Geometry & Field Layout**
    - [x] Extracted `src/core/geometry.c` and `src/core/field_layout.c`.

## Future Improvements & Tooling
- [ ] **State Serialization (Save/Debug Dump)**: Implement a system to serialize `StateInfo` (specifically `LocalGameInfo` and `GlobalGameInfo`) to a file. This will aid in debugging bugs like the "double occupancy" issue by allowing exact state reproduction.
