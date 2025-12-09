# Refactoring Master Plan

## Immediate Goals (Milestone 4: The Big Split)

### Step 7: Action System Scaffold (Active)
- [ ] Create directory `src/game/actions_messy/`.
- [ ] Create directory `src/game/actions_pure/`.
- [ ] Create `src/game/actions_messy/README.md`.

### Step 8: Extract Pitching System (Milestone 4 Start)
- [ ] Identify pitching functions in `action_implementation.c`.
- [ ] Move them to `src/game/actions_messy/pitching_system.c` and `.h`.
- [ ] Fix includes and build.

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
