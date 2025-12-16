- [x] Remove commented-out "Original logic" code from `src/game/actions_pure/batting_physics.c` to keep the file clean. (commit: 9b7bb73)
- [x] Create `src/game/ai_pure/pitching_ai_strategy.h` and `src/game/ai_pure/pitching_ai_strategy.c`. (commit: e9499ef)
    - Extract `calculate_ai_pitch_targets` (calculates limits based on randomness and game state).
    - Ensure it is a pure function taking random values as inputs (no `rand()` inside).
- [x] Create `tests/test_pitching_ai_strategy.c` and register it in `tests/test_runner.c`. (commit: e9499ef)
- [x] Refactor `src/game/actions_messy/pitching_system.c` to use `pitching_ai_strategy.h`. (commit: e9499ef)
- [x] Modify `src/game/actions_messy/pitching_system.c` to use `seeded_rand` for generating random values `rand1`, `rand2`, and `rand3` before passing them to `calculate_ai_pitch_targets`. (commit: d9b2c8f)

### RNG Refactoring (Explicit Seed Passing)
- [ ] Refactor RNG to pass `rng_seed` explicitly instead of storing it in `StateInfo`.
    - [x] **Step 1: Header Changes**: Remove `unsigned int rng_seed;` from `LocalGameInfo` in `src/include/globals.h`. (commit: b998fba)
    - [x] **Step 2: Main & Game Loop**: Update `src/core/main.c` to initialize `rng_seed` locally and pass `&rng_seed` to `updateGameScreen`. (commit: 41e2583)
    - [x] **Step 3: Game Screen**: Update `src/game/game_screen.h/.c` `updateGameScreen` to take `unsigned int* rng_seed` and pass to `actionImplementation`. (commit: 702a602)
    - [x] **Step 4: Action Implementation**: Update `src/game/action_implementation.h/.c` `actionImplementation` and `aiLogic` to take `rng_seed` and pass to AI updates. (commit: 0d8e958)
    - [x] **Step 5: AI Updates**: Update `src/game/ai_messy/catching_ai.h/.c` and `src/game/ai_messy/batting_ai.h/.c` to take `rng_seed` and use `seeded_rand`. (commit: 8cb4391)
    - [x] **Step 6: Pitching System**: Update `src/game/actions_messy/pitching_system.h/.c` `updateAIPitching` to take `rng_seed` (remove `stateInfo` reliance). (commit: 6232221)
    - [x] **Step 7: Game Setup**: Update `src/game/game_setup.h/.c` `initializeGameFromMenu` to take `rng_seed`. Remove `stateInfo->localGameInfo->rng_seed` assignment. Update `loadMutableWorldSettings` to pass seed. (commit: 1e8c496)
    - [x] **Step 8: Common Logic**: Update `src/game/common_logic.h/.c` `initializeSpatialPlayerInformation` to take `rng_seed` and use `seeded_rand` instead of `rand()`. (commit: 1e8c496)
    - [x] **Step 9: Menu**: Update `src/menu/main_menu.c` to pass `rng_seed` to `initializeGameFromMenu`. (commit: b42088d)
