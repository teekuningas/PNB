# Short-term Refactoring Plan

## Immediate Goals (Milestone 2: Physics Isolation)

We are now moving pure physics logic out of `game/game_manipulation.c`.

- [x] **Step 3: Physics Scaffold**
    - [x] Update `Makefile` to include `physics/ball_physics.o` and `physics/collision.o` in the build list.
    - [x] Create `src/physics/ball_physics.h` and `src/physics/ball_physics.c`.
    - [x] Create `src/physics/collision.h` and `src/physics/collision.c`.
    - [x] *Check:* Build (`make main`).

- [ ] **Step 4: Extract Ball Gravity & Trajectory**
    - [ ] Add `#include "globals.h"` to `src/physics/ball_physics.c` for `Vector3D` and `GRAVITY` constant.
    - [ ] Create a pure function `physics_apply_gravity(Vector3D* velocity, float dt)` in `ball_physics.c`. This function should decrease the y-component of the velocity by `GRAVITY * dt`.
    - [ ] Create a pure function `physics_apply_velocity(Vector3D* position, const Vector3D* velocity)` in `ball_physics.c`. This function should add the velocity components to the position components.
    - [ ] In `src/game/game_manipulation.c`, identify the ball movement logic (where gravity is applied to velocity and velocity to position).
    - [ ] Replace the inline gravity application in `game_manipulation.c` with a call to `physics_apply_gravity`.
    - [ ] *Check:* Build (`make main`) and Run Tests (`make test`).

- [ ] **Step 5: Extract Ground Collision Check**
    - [ ] Identify the ground collision logic (checking y < 0, etc.) in `game_manipulation.c`.
    - [ ] Create pure function `physics_check_ground_collision(float y_pos, float ground_level)` in `collision.c` that returns boolean.
    - [ ] Use this in `game_manipulation.c`.
    - [ ] *Check:* Build (`make main`).

## Completed History

- [x] **Step 0: Safety Net & Scaffold**
    - [x] Update `Makefile` to include `core/geometry.o` and `core/field_layout.o`.
    - [x] Create `src/core/geometry.h` and `src/core/geometry.c`.
    - [x] Create `src/core/field_layout.h` and `src/core/field_layout.c`.
- [x] **Step 1: Extract Geometry**
    - [x] Copy distance/angle functions to `src/core/geometry.c`.
    - [x] Update `common_logic.c` to use `geometry.h`.
- [x] **Step 2: Extract Field Layout**
    - [x] Move hardcoded positions to `src/core/field_layout.c`.
    - [x] Update `immutable_world.c` to use `field_layout.h`.

## Mid-Term Roadmap (Phase A)

- [ ] **Renderer Decoupling**
    - [ ] Create `src/renderer/` module.
    - [ ] Move OpenGL calls out of `player.c` and `ball.c`.

- [ ] **Rules Extraction**
    - [ ] Extract pure rules (outs, runs) to `src/game/rules_pure/`.