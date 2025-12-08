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
│   ├── miniaudio.h            3rd party audio library (581k lines!)
│   └── stb_image.h            3rd party image loader
│
├── core/             [~2.3k lines] Platform & utilities (11 .c files)
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
│   └── vector_math.c/h        ✨ NEW: Pure vector utilities
│
├── game/             [~6.9k lines] ★★★ THE PROBLEM ZONE (11 .c files)
│   ├── game_screen.c/h        ★ Orchestrates update/draw loop
│   ├── mutable_world.c/h      High-level coordinator
│   │
│   ├── action_implementation.c/h  ★★★ 2343 lines! User actions + AI logic
│   ├── action_invocations.c/h     Input → action flag conversion
│   │
│   ├── game_analysis.c/h      ★★ 807 lines! Pesäpallo rules logic
│   ├── game_manipulation.c/h  ★★ 884 lines! Physics + ball movement
│   ├── common_logic.c/h       ★★ 988 lines! Player movement + misc
│   │
│   ├── player.c/h             Player rendering + animations
│   ├── ball.c/h               Ball rendering
│   ├── immutable_world.c/h    Field rendering (lines, bases)
│   │
│   └── game_setup.c/h         Initialize game state from settings
│
├── menu/             [~2.7k lines] ✓ CLEAN (11 .c files)
│   ├── main_menu.c/h          Menu orchestrator
│   ├── front_menu.c/h         Title screen
│   ├── team_selection_menu.c/h
│   ├── batting_order_menu.c/h
│   ├── cup_menu.c/h
│   ├── game_over_menu.c/h
│   ├── homerun_contest_menu.c/h
│   ├── hutunkeitto_menu.c/h
│   ├── help_menu.c/h
│   ├── loading_screen_menu.c/h
│   └── menu_helpers.c/h       ✓ Shared utilities
│
└── cup/              [~483 lines] ✓ VERY CLEAN (1 .c file)
    └── cup.c/h                ✓ Self-contained tournament logic
```

---

## Current Subsystem Mapping

### 🎯 Subsystem 1: Platform/Application Core
**Location**: `src/core/` (mostly)
**Responsibility**: Window, input, rendering context, resource loading
**Quality**: ⭐⭐⭐ Good - reasonable separation
**Key Files**: main.c, input.c, render.c, platform.c

### 🎮 Subsystem 2: Game Logic (THE MESS)
**Location**: `src/game/` (everything)
**Responsibility**: ALL OF THE GAME (rules, physics, AI, rendering, state management)
**Quality**: ⭐ Poor - everything tangled together
**Problem**: 
- action_implementation.c is 2343 lines mixing:
  - Player actions (pitching, throwing, batting)
  - AI decision making
  - Physics calculations
  - Animation state management
  - Static variables for timing/state
- common_logic.c is 988 lines of unrelated functions
- game_analysis.c tries to be rules engine but mutates state directly
- game_manipulation.c does physics but also AI behavior

### 🖼️ Subsystem 3: Rendering
**Location**: Scattered! (core/render.c, game/player.c, game/ball.c, game/immutable_world.c)
**Responsibility**: Draw everything to screen
**Quality**: ⭐⭐ Mixed - OpenGL calls mixed into game logic
**Problem**: game/player.c does both logic AND rendering (450 lines of mixed concerns)

### 🎵 Subsystem 4: Menu System
**Location**: `src/menu/` (self-contained!)
**Responsibility**: All menus, team selection, game setup
**Quality**: ⭐⭐⭐⭐ Good - each screen is own file, shared helpers
**Success Pattern**: Uses menu_types.h for data, menu_helpers for utilities

### 🏆 Subsystem 5: Cup/Tournament
**Location**: `src/cup/` (perfect!)
**Responsibility**: Tournament bracket logic, match scheduling
**Quality**: ⭐⭐⭐⭐⭐ Excellent - 438 lines, clear API, testable
**Success Pattern**: Clean module, no dependencies on game internals

---

## The Core Problem Visualized

```
CURRENT DEPENDENCY DISASTER:

                    ┌──────────────┐
                    │  globals.h   │ ← Everyone includes this!
                    │ (StateInfo)  │    5000+ lines of types
                    └──────┬───────┘
                           │
         ┌─────────────────┼─────────────────┐
         │                 │                 │
    ┌────▼────┐      ┌─────▼─────┐    ┌─────▼─────┐
    │  core/  │      │   game/   │    │   menu/   │
    │ (good)  │◄─────┤  (mess)   ├───►│  (good)   │
    └─────────┘      └─────┬─────┘    └───────────┘
                           │
                    Everything calls
                    everything else!

game/action_implementation.c depends on:
- globals.h (StateInfo)
- common_logic.h (player movement)
- game_analysis.h (rules checks)
- game_manipulation.h (physics)
- render.h (OpenGL?! why?)
- Plus its own 2343 lines of code

Result: Can't change anything without risk of breaking everything.
```

---

# LEVEL 2: TARGET ARCHITECTURE (To-Be)

## Subsystem-First Design → File Structure

```
src/
├── core/                    Platform & Foundation
│   ├── main.c              ✓ App lifecycle (keep)
│   ├── platform.c/h        ✓ OS abstraction (keep)
│   ├── input_system.c/h    ⬆️ Upgraded from input.c
│   ├── resource_manager.c/h ✓ (keep)
│   ├── font.c/h            ✓ (keep)
│   └── vector_math.c/h     ✨ NEW: Math utilities
│
├── renderer/               🆕 Rendering Subsystem
│   ├── renderer.c/h        Main rendering API
│   ├── render_types.h      Renderable objects
│   ├── render_world.c/h    3D scene rendering
│   ├── render_ui.c/h       2D overlay rendering
│   └── mesh_cache.c/h      Model/texture management
│
├── physics/                🆕 Physics Subsystem
│   ├── physics.c/h         Ball trajectory, collisions
│   ├── collision.c/h       AABB, sphere tests
│   └── field_geometry.c/h  Field boundaries, bases
│
├── game_logic/             🆕 Game Logic (modular)
│   ├── rules/
│   │   ├── rules_engine.c/h      ★ Pure rules evaluation
│   │   ├── rules_outs.c/h        Out detection
│   │   ├── rules_scoring.c/h     Run scoring
│   │   └── rules_strikes.c/h     Strikes & balls
│   │
│   ├── entities/
│   │   ├── entity_manager.c/h    Player/ball management
│   │   ├── player_state.c/h      Player data
│   │   └── ball_state.c/h        Ball data
│   │
│   ├── ai/
│   │   ├── ai_controller.c/h     AI decision making
│   │   ├── ai_fielding.c/h       Defensive AI
│   │   └── ai_batting.c/h        Offensive AI
│   │
│   ├── actions/
│   │   ├── action_processor.c/h  Action handling
│   │   ├── pitching.c/h          Pitching mechanics
│   │   ├── batting.c/h           Batting mechanics
│   │   └── throwing.c/h          Throwing mechanics
│   │
│   ├── game_state.h        Read-only state snapshots
│   ├── game_events.h       Event definitions
│   └── game_coordinator.c/h  High-level game flow
│
├── menu/                   ✓ Keep as-is (it's good!)
│   └── [11 menu files]
│
├── cup/                    ✓ Keep as-is (it's perfect!)
│   └── cup.c/h
│
└── include/                Header-only definitions
    ├── types.h             Basic types (Vector3D, etc.)
    ├── constants.h         Game constants
    └── [3rd party libs]
```

---

## Target Subsystem Mapping

### 🎯 Subsystem 1: Core Platform (src/core/)
**Responsibility**: Application lifecycle, OS interaction
**Dependencies**: None (foundation layer)
**Size**: ~1500 lines (slightly smaller)

### 🎨 Subsystem 2: Renderer (src/renderer/)
**Responsibility**: ALL rendering, OpenGL encapsulation
**Dependencies**: core, types
**Size**: ~1000 lines
**Key Change**: Game logic NEVER calls OpenGL directly

### ⚛️ Subsystem 3: Physics (src/physics/)
**Responsibility**: Ball flight, collision detection, movement
**Dependencies**: types, constants
**Size**: ~500 lines
**Key Change**: Pure functions, no StateInfo

### 🎮 Subsystem 4: Game Logic (src/game_logic/)
**Responsibility**: Pesäpallo rules, AI, actions, game state
**Dependencies**: physics, types
**Size**: ~4000 lines (same total, but organized!)
**Key Change**: Modular sub-components

#### 4a: Rules Engine (game_logic/rules/)
**Responsibility**: Detect game events (outs, runs, strikes)
**Dependencies**: game_state.h (read-only)
**Size**: ~600 lines
**Key Property**: ✨ PURE FUNCTIONS - testable!

#### 4b: Entity Manager (game_logic/entities/)
**Responsibility**: Manage players and ball
**Dependencies**: types
**Size**: ~800 lines
**Key Change**: Encapsulates player array access

#### 4c: AI Controller (game_logic/ai/)
**Responsibility**: Computer player decisions
**Dependencies**: game_state.h (read-only)
**Size**: ~1000 lines
**Key Property**: ✨ PURE FUNCTIONS - testable!

#### 4d: Action Processor (game_logic/actions/)
**Responsibility**: Execute user/AI actions
**Dependencies**: entities, physics
**Size**: ~1200 lines
**Key Change**: No longer 2343-line monolith

### 🎵 Subsystem 5: Menu System (src/menu/)
**Responsibility**: All menus
**Dependencies**: renderer, types
**Size**: ~2700 lines (unchanged)
**Status**: ✓ Already good!

### 🏆 Subsystem 6: Cup/Tournament (src/cup/)
**Responsibility**: Tournament brackets
**Dependencies**: types
**Size**: ~500 lines (unchanged)
**Status**: ✓ Already perfect!

---

## The Target Dependency DAG (Directed Acyclic Graph)

```
                    ┌──────────────┐
                    │    types.h   │  ← Simple type definitions
                    │ (Vector3D,   │     No massive StateInfo!
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

## Before & After Comparison

### Key File Transformations

| Current File | Size | Problem | Target Files | Total Size |
|--------------|------|---------|--------------|------------|
| action_implementation.c | 2343 | Everything! | actions/*.c (4 files) + ai/*.c (3 files) | ~2200 |
| common_logic.c | 988 | Junk drawer | physics/*.c + entities/*.c + (deleted) | ~600 |
| game_analysis.c | 807 | Mutates state | rules/*.c (4 files) | ~700 |
| game_manipulation.c | 884 | Physics + AI | physics/*.c + ai/*.c | ~800 |
| player.c | 450 | Logic + rendering | entities/player_state.c + renderer/render_world.c | ~400 |

**Total**: ~5500 lines → ~4700 lines (cleaner, more testable)

---

## Alternative Target Goals (Speculation)

### Option A: Moderate Refactoring (Recommended)
- Keep game/ directory but split into subdirs (rules/, ai/, actions/)
- Extract physics/ and renderer/ as separate
- Keep common_logic but shrink it to just player movement helpers
- **Timeline**: 3-4 months
- **Risk**: Medium
- **Benefit**: Significant improvement, manageable scope

### Option B: Full Zen Architecture (As described above)
- Complete module separation
- Pure functions everywhere possible
- Perfect DAG structure
- **Timeline**: 5-6 months
- **Risk**: Medium-High
- **Benefit**: Maximum improvement, perfect for future development

### Option C: Minimal Refactoring (Conservative)
- Just extract vector_math, physics, and renderer
- Keep game/ as-is but document it better
- Focus on testability of new modules
- **Timeline**: 1-2 months
- **Risk**: Low
- **Benefit**: Some improvement, very safe

### Option D: Radical Rewrite (Not Recommended)
- Start from scratch using lessons learned
- Port one feature at a time
- **Timeline**: 12+ months
- **Risk**: Very High (might never finish)
- **Benefit**: Perfect code, but huge cost

---

## Key Insights

### What Makes Menu & Cup Clean?
1. **Single Responsibility**: Each file does ONE thing
2. **Clear Data Structures**: menu_types.h, cup.h define everything
3. **No Global State**: They receive what they need as parameters
4. **Testable**: Cup has unit tests because it's pure logic

### What Makes Game Messy?
1. **God Object**: StateInfo passed to 176+ functions
2. **Mixed Concerns**: action_implementation.c does 5 different jobs
3. **Static State**: Hidden state in .c files (timing, AI stages)
4. **Circular Dependencies**: Everyone includes common_logic.h
5. **Untestable**: Can't test rules without full game setup

### The Path Forward
- **Start**: Vector math extraction (✓ done!)
- **Next**: Physics module (pure functions, testable)
- **Then**: Rendering decoupling (game logic describes, renderer draws)
- **Then**: Rules engine extraction (pure functions, testable)
- **Finally**: Break up action_implementation.c (hardest!)

---

## Conclusion

**Current State**: 
- Tangled ball of dependencies
- 2343-line files mixing everything
- Hard to test, hard to modify

**Target State**:
- Clean module boundaries
- Files under 500 lines each
- Testable pure functions
- Easy to add features

**The Journey**:
- Small incremental steps
- Always working game
- Gradually untangle the mess
- Use menu/cup as role models

The refactoring is like cleaning a messy room. We can't fix everything at once, but each small improvement makes the next one easier!
