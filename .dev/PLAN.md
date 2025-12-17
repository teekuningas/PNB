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
**Goal:** Extract the complex rules logic (outs, runs, strikes) from `game_analysis.c` into a pure `src/game/rules_pure/` module, mapping them to the official rules (`docs/SAANNOT.md`). This is the final major "Brain" extraction before we can consider larger architectural dataflow changes.

**Safety Protocol:** "One Slice at a Time." Do not attempt to refactor the entire file at once. Extract one rule type, integrate it, verify it works, and then move to the next.

- [x] **Phase 1: Outs (Pesäkilpa §33)**
    - [x] Setup: Create `src/game/rules_pure/rules_outs.h/c` and tests.
    - [x] Extract: Move "Out" detection logic to `rules_outs.c`.
    - [x] Map: Reference **§33 Pesäkilpa** in comments.
    - [x] Verify: Ensure `game_analysis.c` uses the new function and tests pass.

- [ ] **Phase 2: Runs (Juoksu §41, Kunniajuoksu §42)**
    - [ ] Setup: Create `src/game/rules_pure/rules_runs.h/c` and tests.
    - [ ] Extract: Move scoring logic to `rules_runs.c`.
    - [ ] Map: Reference **§41 Juoksu** and **§42 Kunniajuoksu**.
    - [ ] Verify: Ensure integration and tests pass.

- [ ] **Phase 3: Strikes/Balls (Syöttö §26)**
    - [ ] Setup: Create `src/game/rules_pure/rules_strikes.h/c` and tests.
    - [ ] Extract: Move strike/ball logic to `rules_strikes.c`.
    - [ ] Map: Reference **§26 Syötön tuomitseminen**.
    - [ ] Verify: Ensure integration and tests pass.

- [ ] **Final Cleanup:** Remove legacy code comments and ensure all tests pass.

## Milestone 7: Data Renaissance (Structure Shapes Logic)
**Goal:** Shift from "Code modifying flags" to "Data defining state." We cannot build a clean system on top of ambiguous data.
- [ ] **Enums over Magic Numbers:** Replace raw integers (e.g., `period >= 4`, `base == 4`) with semantic Enums (e.g., `GameMode::HOMERUN_CONTEST`, `Base::HOME`).
- [ ] **Explicit State Machines:** Replace dependent boolean flags (e.g., `isOnBase=1` && `out=0`) with single source-of-truth Enums (e.g., `PlayerState { RUNNING, SAFE, OUT }`).
- [ ] **Componentization:** Begin breaking the `StateInfo` God-object into distinct, cohesive structs (`PhysicsState`, `RulesState`, `ScoreState`).
- [ ] **Strict Data Contracts:** Ensure pure functions define their own explicit input structs (e.g., `BattingContext`) rather than accepting generic chunks of state.

## Milestone 8: Functional Dataflow & Tooling
**Goal:** The "Big Flip." Refactor the main game loop to follow a functional dataflow pattern, utilizing the clean data structures from Milestone 7.
- [ ] **State Serialization (Save/Debug Dump)**: Implement a system to serialize `StateInfo` (specifically `LocalGameInfo` and `GlobalGameInfo`) to a file. This will aid in debugging bugs like the "double occupancy" issue by allowing exact state reproduction.
- [ ] **Game Loop Functional Dataflow**: Refactor the main game loop (`gameManipulation`, `actionImplementation`) to follow the Menu's "Functional Dataflow" pattern. Break `LocalGameInfo` into distinct sub-states (Physics, Rules, AI) and pass only the relevant data explicitly to update functions, removing the reliance on the global `StateInfo` god-object.

## Testing Strategy
