# PNB Architecture: Current State vs Target State

## Two-Level Understanding

This document maps:
1. **Current Reality** - Files → Subsystems (what we have now)
2. **Target Vision** - Subsystems → Files (what we want)

---

# LEVEL 1: CURRENT ARCHITECTURE (As-Is)

## File Structure → Functional Subsystems

```
src/
├── include/          [~102k lines] Shared type definitions (no .c files)
│   ├── globals.h              ★ THE GOD HEADER - everything includes this
│   ├── menu_types.h           Clean menu data structures
│   ├── fixtures.h             Test scenario definitions
│   ├── miniaudio.h            3rd party audio library
│   └── stb_image.h            3rd party image loader
│
├── core/             [~2.3k lines] Platform & utilities
│   ├── main.c                 ★ Application loop, coordinates everything
│   ├── input.c/h              Keyboard/controller reading
│   ├── render.c/h             OpenGL setup, 2D/3D contexts
│   ├── font.c/h               Text rendering
│   ├── sound.c/h              Audio playback
│   ├── resource_manager.c/h   Texture/asset loading
│   ├── loadobj.c/h            3D model (.obj) loading
│   ├── platform.c/h           OS-specific code
│   ├── fill_player_data.c/h   XML team data parsing
│   ├── fixtures.c/h           Test fixture setup
│   ├── vector_math.c/h        Pure vector utilities
│   ├── geometry.c/h           Pure geometric calculations
│   └── field_layout.c/h       Pure field layout definitions
│
├── renderer/         [~800 lines] 🆕 Decoupled Rendering
│   ├── player_renderer.c/h    Player model rendering
│   └── ball_renderer.c/h      Ball model rendering
│
├── physics/          [~400 lines] 🆕 Decoupled Physics
│   ├── ball_physics.c/h       Pure ball trajectory math
│   └── collision.c/h          Pure collision math
│
├── game/             [~6.9k lines] ★ THE GAME LOGIC
│   ├── game_screen.c/h        ★ Orchestrates update/draw loop
│   ├── mutable_world.c/h      High-level coordinator
│   ├── immutable_world.c/h    Field rendering (lines, bases)
│   ├── game_setup.c/h         Initialize game state from settings
│   │
│   ├── actions_messy/         ⚠️ Coordinators (Logic + State Mutation)
│   │   ├── pitching_system.c/h
│   │   ├── batting_system.c/h
│   │   ├── throwing_system.c/h
│   │   └── action_state.h     Shared action state
│   │
│   ├── actions_pure/          ✅ Pure Logic (Testable)
│   │   ├── pitching_physics.c/h  Trajectory, meter math
│   │   └── batting_physics.c/h   Trajectory, meter math
│   │
│   ├── ai_messy/              ⚠️ Coordinators (Logic + State Mutation)
│   │   ├── batting_ai.c/h
│   │   └── catching_ai.c/h
│   │
│   ├── ai_pure/               ✅ Pure Logic (Testable)
│   │   ├── batting_ai_strategy.c/h  Decision trees
│   │   ├── catching_ai_strategy.c/h Movement & decisions
│   │   └── pitching_ai_strategy.c/h Target calculation strategies
│   │
│   ├── action_implementation.c/h  ⬇️ Shrinking coordinator
│   ├── action_invocations.c/h     Input → action flag conversion
│   ├── game_analysis.c/h          ⬇️ Shrinking coordinator (Rules delegation)
│   ├── game_manipulation.c/h      Physics integration
│   ├── common_logic.c/h           Misc logic
│   ├── player.c/h                 Legacy player logic
│   ├── ball.c/h                   Legacy ball logic
│   └── rules_pure/              ✅ Pure Logic (Testable)
│       ├── rules_outs.c/h         Out determination (§33 Pesäkilpa)
│       ├── rules_runs.c/h         Run calculation (§41 Juoksu, §42 Kunniajuoksu)
│       └── rules_strikes.c/h      Strike/Ball logic (§26 Syötön tuomitseminen)
│
├── menu/             [~2.7k lines] ✓ CLEAN
│   └── [11 menu files] ...
│
└── cup/              [~483 lines] ✓ VERY CLEAN
    └── cup.c/h                ✓ Self-contained tournament logic
```

---

## Current Subsystem Mapping

### 🎯 Subsystem 1: Platform/Application Core
**Location**: `src/core/`
**Responsibility**: Window, input, audio, resource loading
**Quality**: ⭐⭐⭐ Good - Solid foundation

### 🎮 Subsystem 2: Game Logic (Refactoring in Progress)
**Location**: `src/game/`
**Responsibility**: Rules, AI, Actions, Physics
**Quality**: ⭐⭐ Improving - "Messy" vs "Pure" separation is visible
**Progress**:
- **Actions**: Split into `actions_messy` (coordinators) and `actions_pure` (math/physics).
- **AI**: Split into `ai_messy` (state mutators) and `ai_pure` (decision strategies).
- **Rules**: Split into `game_analysis.c` (coordinator) and `rules_pure` (pure rule evaluations).
- **Physics**: Core ball physics extracted to `src/physics/`.
- **Renderer**: Entity rendering moved to `src/renderer/`.

### 🖼️ Subsystem 3: Rendering
**Location**: `src/renderer/` + `src/core/render.c`
**Responsibility**: Draw everything to screen
**Quality**: ⭐⭐⭐ Improving - Player and Ball rendering separated from logic.

### 🎵 Subsystem 4: Menu System
**Location**: `src/menu/`
**Responsibility**: All menus
**Quality**: ⭐⭐⭐⭐ Good

### 🏆 Subsystem 5: Cup/Tournament
**Location**: `src/cup/`
**Responsibility**: Tournament bracket logic
**Quality**: ⭐⭐⭐⭐⭐ Excellent

---

## The Core Problem Visualized (Updated)

```
STILL DEPENDENCY HEAVY, BUT LAYERS EMERGING:

                    ┌──────────────┐
                    │  globals.h   │
                    │ (StateInfo)  │
                    └──────┬───────┘
                           │
         ┌─────────────────┼─────────────────┐
         │                 │                 │
    ┌────▼────┐      ┌─────▼─────┐    ┌─────▼─────┐
    │  core/  │      │   game/   │    │   menu/   │
    │ (good)  │◄─────┤ (improving)├──►│  (good)   │
    └─────────┘      └─────┬─────┘    └───────────┘
                           │
                ┌──────────┴──────────┐
                │                     │
        ┌───────▼──────┐       ┌──────▼──────┐       ┌───────▼──────┐
        │ actions_pure │       │   ai_pure   │       │  rules_pure  │
        │ (NO STATE!)  │       │ (NO STATE!) │       │ (NO STATE!)  │
        └──────────────┘       └─────────────┘       └──────────────┘

Progress:
- `actions_pure`, `ai_pure`, and `rules_pure` do NOT depend on `globals.h` (StateInfo).
- `actions_messy`, `ai_messy`, and `game_analysis.c` depend on their respective pure modules and `globals.h`.
- This creates a clean "Leaf Node" layer of pure logic.
```

---

# LEVEL 2: TARGET ARCHITECTURE (To-Be)

## The Target Dependency DAG (Directed Acyclic Graph)

```
                    ┌──────────────┐
                    │    types.h   │
                    │ (Vector3D,   │
                    │  constants)  │
                    └───────┬──────┘
                            │
              ┌─────────────┼─────────────┐
              │             │             │
         ┌────▼────┐   ┌────▼────┐   ┌───▼────┐
         │  core   │   │ physics │   │renderer│
         │(platform│   │ (pure)  │   │        │
         └────┬────┘   └────┬────┘   └───┬────┘
              │             │            │
              └──────┬──────┴────────────┘
                     │
              ┌──────▼───────┐
              │ game_logic   │
              │ (modular)    │
              └──────┬───────┘
                     │
        ┌────────────┼────────────┐
        │            │            │
    ┌───▼───┐   ┌────▼────┐   ┌──▼──┐
    │ menu  │   │   cup   │   │ ... │
    └───────┘   └─────────┘   └─────┘

Rules: Dependencies only flow downward!
- renderer never calls game_logic
- physics never calls renderer
- rules engine never mutates state
- No circular dependencies
```

---

## The Game Loop: Functional Dataflow (NOT Event-Driven!)

**CRITICAL:** We do NOT want event buses, message queues, or async patterns!

### What We Want: Synchronous Function Call Pipeline

```
main.c (The Orchestrator)
  │
  ├─> InputState input = poll_input()
  │       └─> Read keyboard/controller state
  │
  ├─> StateInfo new_state = update_game(current_state, input)
  │       │
  │       ├─> update_physics(state, input)
  │       │     └─> Ball trajectories, collisions (pure math)
  │       │
  │       ├─> update_movement(state, input)
  │       │     └─> Player positions, animations
  │       │
  │       ├─> update_rules(state)
  │       │     └─> Check outs, runs, strikes (pure logic)
  │       │
  │       ├─> update_ai(state)
  │       │     └─> AI decisions (pure strategy)
  │       │
  │       └─> return modified_state
  │
  ├─> render_game(new_state)
  │       ├─> render_3d_world(new_state)
  │       └─> render_overlay(new_state)
  │
  └─> current_state = new_state
  │
  └─> Loop repeats (60 FPS)
```

### Key Principles

✅ **Explicit function calls** - main → coordinators → subsystems → pure  
✅ **Synchronous execution** - Easy to debug, single call stack  
✅ **Data flows in/out** - Like breathing (in: state+input, out: new state)  
✅ **Top-down control** - No sideways messaging between subsystems  
✅ **Pure functions dominate** - 80% of code is pure, 20% is coordination  

### What We DON'T Want

❌ **Event bus** - No message queue, no pub/sub  
❌ **Observer pattern** - No listeners, no callbacks  
❌ **Async messaging** - No "fire and forget" events  
❌ **Sideways coupling** - Subsystems don't talk to each other  

**Why:** Simplicity, debuggability, and understandability trump architectural flexibility.

---