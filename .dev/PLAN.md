# Refactoring Master Plan

## Immediate Goals (Milestone 4: The Big Split)

### Step 7: Action System Scaffold (Completed)
- [x] Create directory `src/game/actions_messy/`.
- [x] Create directory `src/game/actions_pure/`.
- [x] Create `src/game/actions_messy/README.md`.

### Step 8: Extract Pitching System (Active)
- [x] Identify pitching functions in `action_implementation.c`.
- [x] Move core functions (`startPitch`, `continuePitch`, `releasePitch`) to `src/game/actions_messy/pitching_system.c`.
- [ ] Extract pitching meter logic from `updateMeters` to `pitching_system.c`.
- [x] Move AI lock constants and variables (aiActionEventLock, aiLockUpdate, aiLockTimeoutCounter) to `src/game/actions_messy/action_state.c` and .h.
- [x] Extract AI pitching variables and logic to `pitching_system.c`.
- [x] Fix includes and build.

## Cleanup Tasks (Low Priority)
- [ ] **Clean `src/core/render.h`**: Remove `#include "globals.h"` from the header. Add `<GL/glew.h>`. Ensure `render.c` still includes `globals.h` for constants.

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