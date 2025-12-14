# Refactoring Master Plan

## Immediate Goals (Milestone 4: The Big Split)

### Step 7: Action System Scaffold (Completed)
- [x] Create directory `src/game/actions_messy/`.
- [x] Create directory `src/game/actions_pure/`.
- [x] Create `src/game/actions_messy/README.md`.

### Step 8: Extract Pitching System (Completed)
- [x] Identify pitching functions in `action_implementation.c`.
- [x] Move core functions (`startPitch`, `continuePitch`, `releasePitch`) to `src/game/actions_messy/pitching_system.c`.
- [x] Extract pitching meter logic from `updateMeters` to `pitching_system.c`.
- [x] Move AI lock constants and variables (aiActionEventLock, aiLockUpdate, aiLockTimeoutCounter) to `src/game/actions_messy/action_state.c` and .h.
- [x] Extract AI pitching variables and logic to `pitching_system.c`.
- [x] Fix includes and build.

### Step 9: Extract Batting System (Completed)
- [x] Identify batting variables and logic in `action_implementation.c`.
- [x] Create `src/game/actions_messy/batting_system.c` and `.h`.
- [x] Move batting constants/variables to `src/game/actions_messy/batting_system.c` (or `action_state.c` if shared).
- [x] Move batting functions to `batting_system.c`.
- [x] Extract batting meter logic from `updateMeters` to `batting_system.c`.

### Step 10: Extract Throwing & Fielding (Completed)
- [x] Identify throwing/fielding variables and logic in `action_implementation.c`.
- [x] Create `src/game/actions_messy/throwing_system.c` and `.h`.
- [x] Move generic movement/throwing functions (`genericThrowRelease`, `genericThrowLoad`, `genericMove`, `genericStopMove`) to the new system.
- [x] Clean up `action_implementation.c` to be a pure dispatcher.

### Step 11: Extract AI Logic (In Progress)
- [x] Create `src/game/ai_messy/catching_ai.c` and `.h`.
- [x] Move catching AI helpers (`moveControlledPlayerToLocation`, `throwBallToBase`) to `catching_ai.c`.
- [ ] Create `src/game/ai_messy/batting_ai.c` and `.h`.
- [ ] Move batting AI logic and state variables from `action_implementation.c` to `batting_ai.c`.
- [ ] Refactor `aiLogic` in `action_implementation.c` to simply call `updateCatchingAI` (or equivalent) and `updateBattingAI`.

## Cleanup Tasks (Low Priority)
- [x] **Clean `src/core/render.h`**: Remove `#include "globals.h"` from the header. Add `<GL/glew.h>`. Ensure `render.c` still includes `globals.h` for constants.

## Completed History

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
