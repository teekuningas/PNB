# PNB Refactoring Strategy: Isolate Messiness, Extract Purity

## Philosophy: Pragmatic Progression with Milestones

Your vision is excellent and very practical! Let me formalize it:

### Core Principles

1. **Hierarchical is OK** - We're building an architecture, not microservices
2. **Dispatcher Pattern** - Coordinators delegate to subsystems → pure functions
3. **Data flows in/out** - Not sideways between subsystems
4. **Messiness isolation** - Keep mess contained in "bubbles"
5. **Two-phase approach**:
   - Phase A: Isolate messiness + Extract purity (THE LONG PHASE)
   - Phase B: Decouple messiness (only if we survive Phase A!)

### Target Architecture (Refined)

```
┌─────────────────────────────────────────────────────────┐
│                    main.c                               │
│              (Application Dispatcher)                    │
└───────────┬─────────────────────────┬───────────────────┘
            │                         │
    ┌───────▼────────┐        ┌───────▼────────┐
    │  main_menu.c   │        │game_coordinator│  ← PUPPET MASTERS
    │  (delegates)   │        │     .c         │     (concise!)
    └───────┬────────┘        └───────┬────────┘
            │                         │
    ┌───────▼────────┐    ┌───────────┴──────────────┐
    │ Menu Subsystems│    │   Game Subsystems        │
    │ (well-defined) │    │   (isolated bubbles)     │
    └───────┬────────┘    └───────┬──────────────────┘
            │                     │
            └─────────┬───────────┘
                      │
              ┌───────▼────────┐
              │  Pure Functions │  ← THE LEAVES
              │ (math, physics) │     (beautiful!)
              └─────────────────┘

Key: Hierarchy flows DOWN. Data flows IN/OUT. No sideways coupling!
```

---

## PHASE A: Isolate Messiness + Extract Purity

**Duration**: 3-4 months
**Goal**: Reorganize without fundamentally changing logic
**Result**: Messy systems are contained, pure logic is extracted

### The Strategy

#### 1. Create "Messiness Bubbles"
Keep complex, stateful, coupled code together - but CONTAINED.

Example:
```
src/game/
├── game_coordinator.c          ← Clean dispatcher (NEW)
├── messy_action_system/        ← BUBBLE: Contains the mess
│   ├── action_state.c          ← All the static variables
│   ├── pitching_impl.c         ← Complex pitching logic
│   ├── batting_impl.c          ← Complex batting logic
│   └── ai_impl.c               ← AI decision making
├── messy_rules_system/         ← BUBBLE: Rules + state mutation
│   ├── rules_state.c           ← Counters, flags
│   ├── outs_check.c            ← Check outs + mutate state
│   └── scoring_check.c         ← Check scoring + mutate state
└── pure/                       ← CLEAN: Pure extracted logic
    ├── ball_physics.c          ← Pure trajectory calculations
    ├── distance_checks.c       ← Pure geometry
    └── field_geometry.c        ← Pure field constants
```

**Benefit**: We know EXACTLY where the mess lives. Future devs see "messy_*" and understand.

#### 2. Extract Pure Functions Aggressively
Whenever we find a pure function in messy code, extract it immediately.

**Pure function criteria**:
- No static variables
- No StateInfo parameter
- Deterministic (same input → same output)
- No side effects

---

## Milestone-Driven Roadmap

### 🏁 Milestone 1: "The Foundation" (2-3 weeks)
**Goal**: Extract all pure math/utility functions

**Candidates** (from common_logic.c and game_manipulation.c):
1. ✅ Vector math (DONE!)
2. Distance calculations
3. Angle/orientation calculations
4. Boundary checking (out of bounds)
5. Base position geometry

**Files created**:
- `src/core/vector_math.c` ✅
- `src/core/geometry.c` (distance, angles)
- `src/core/field_layout.c` (base positions, boundaries)

**Success Metric**: 
- All pure math in `src/core/`
- `common_logic.c` shrinks to <600 lines
- Zero behavior changes

**Joy Factor**: 🎉 We have testable utilities! Can write unit tests!

---

### 🏁 Milestone 2: "Physics Isolation" (2-3 weeks)
**Goal**: Extract ball physics into pure module

**What gets extracted** (from game_manipulation.c):
- Ball trajectory calculation
- Gravity application
- Collision detection
- Bounce physics
- Out-of-bounds detection

**Files created**:
- `src/physics/ball_physics.h`
- `src/physics/ball_physics.c`
- `src/physics/collision.c`

**Messiness kept separate**:
- Player catching AI (still messy, stays in game/)
- Ball state updates (mutation, stays in game/)

**Success Metric**:
- `physics/` module compiles standalone
- Can test "if ball at X with velocity Y, where will it be in 5 frames?"
- `game_manipulation.c` shrinks to <500 lines

**Joy Factor**: 🎉 Physics is a black box! Easy to tweak and test!

---

### 🏁 Milestone 3: "Rendering Decoupling" (3-4 weeks)
**Goal**: Game logic describes, renderer draws

**What changes**:
- Create `RenderList` data structure
- Move OpenGL calls from `player.c`, `ball.c` to new `renderer/`
- `game_screen.c` builds RenderList from game state

**Files created**:
- `src/renderer/render_types.h`
- `src/renderer/renderer.c`
- `src/renderer/world_renderer.c`

**Messiness kept separate**:
- Player animation state (complex, stays in game/)
- Model selection logic (stays in game/)

**Success Metric**:
- NO `#include "render.h"` in `src/game/*.c` (except game_screen.c)
- Game looks identical
- Renderer could be swapped (Vulkan, DirectX - theoretically!)

**Joy Factor**: 🎉 Could test game logic without GPU! Rendering is a service!

---

### 🏁 Milestone 4: "The Big Split" (4-5 weeks)
**Goal**: Split action_implementation.c into manageable pieces

**Current**: action_implementation.c (2343 lines mixing everything)

**Target structure**:
```
src/game/
├── game_coordinator.c          ← 150 lines (dispatcher)
├── action_dispatcher.c         ← 200 lines (routes actions)
├── actions_messy/              ← BUBBLE (still coupled, but organized)
│   ├── pitching_system.c       ← ~400 lines (pitching + animation)
│   ├── batting_system.c        ← ~500 lines (batting + animation)
│   ├── throwing_system.c       ← ~300 lines (throwing + animation)
│   ├── fielding_system.c       ← ~300 lines (fielding + AI)
│   └── action_state.c          ← ~200 lines (static variables)
└── actions_pure/               ← Pure extracted parts
    ├── pitch_trajectory.c      ← ~100 lines (pure math)
    ├── bat_collision.c         ← ~100 lines (pure physics)
    └── throw_calculation.c     ← ~100 lines (pure math)
```

**Success Metric**:
- No single file >500 lines
- Clear naming: *_system = messy, *_calculation = pure
- action_implementation.c DELETED
- All features still work

**Joy Factor**: 🎉🎉 The monolith is CONQUERED! Files are understandable!

---

### 🏁 Milestone 5: "Rules Extraction" (3-4 weeks)
**Goal**: Make pesäpallo rules queryable and testable

**Current**: game_analysis.c (807 lines, mutates state)

**Target**:
```
src/game/
├── rules_coordinator.c         ← Calls checks, applies events
├── rules_messy/                ← BUBBLE (state-dependent)
│   ├── rules_state.c           ← Timers, flags, counters
│   ├── outs_detector.c         ← Check outs (reads full state)
│   ├── scoring_detector.c      ← Check runs (reads full state)
│   └── strikes_detector.c      ← Strikes/balls (reads state)
└── rules_pure/                 ← Pure rule logic
    ├── out_conditions.c        ← "Is this an out?" (pure)
    ├── run_conditions.c        ← "Is this a run?" (pure)
    └── strike_conditions.c     ← "Strike or ball?" (pure)
```

**Success Metric**:
- Can write tests: "Player at base 1, ball at base 1 → should be out"
- rules_pure/ has NO StateInfo dependency
- game_analysis.c shrinks or disappears

**Joy Factor**: 🎉 Can test pesäpallo rules in isolation! New features are safer!

---

### 🏁 Milestone 6: "AI Extraction" (2-3 weeks)
**Goal**: Separate AI decision-making from action execution

**Current**: AI logic scattered in action_implementation.c

**Target**:
```
src/game/
├── ai_coordinator.c            ← Routes AI queries
├── ai_messy/                   ← BUBBLE (complex heuristics)
│   ├── ai_state.c              ← AI timers, stages
│   ├── fielding_ai.c           ← Defensive decisions
│   ├── batting_ai.c            ← Offensive decisions
│   └── pitcher_ai.c            ← Pitching decisions
└── ai_pure/                    ← Pure AI helpers
    ├── field_evaluation.c      ← "Best player to catch?" (pure)
    ├── threat_assessment.c     ← "How dangerous?" (pure)
    └── target_selection.c      ← "Where to throw?" (pure)
```

**Success Metric**:
- AI code not mixed with player action code
- Could write AI tests with mock game states
- Easier to tune AI difficulty

**Joy Factor**: 🎉 AI is a module! Could make AI settings menu!

---

## Phase A Summary: What We've Achieved

After Milestone 6 (16-20 weeks total):

### File Structure
```
src/
├── core/                       ← Pure utilities (testable!)
│   ├── vector_math.c
│   ├── geometry.c
│   └── field_layout.c
├── physics/                    ← Pure physics (testable!)
│   ├── ball_physics.c
│   └── collision.c
├── renderer/                   ← Rendering service
│   ├── renderer.c
│   └── world_renderer.c
├── game/                       ← Game logic (organized!)
│   ├── game_coordinator.c      ← 150 lines (clean dispatcher)
│   ├── action_dispatcher.c     
│   ├── rules_coordinator.c     
│   ├── ai_coordinator.c        
│   │
│   ├── actions_messy/          ← BUBBLE 1
│   ├── actions_pure/           ← Pure action helpers
│   │
│   ├── rules_messy/            ← BUBBLE 2
│   ├── rules_pure/             ← Pure rule logic
│   │
│   ├── ai_messy/               ← BUBBLE 3
│   └── ai_pure/                ← Pure AI helpers
├── menu/                       ← (unchanged)
└── cup/                        ← (unchanged)
```

### Metrics
- **Largest file**: <500 lines (was 2343!)
- **Pure functions extracted**: ~30-40 functions
- **Testable modules**: 5+ (core, physics, rules_pure, ai_pure, actions_pure)
- **Messiness**: Isolated in 3 bubbles, clearly labeled
- **Dependencies**: Much cleaner, coordinators at top

### Benefits
✅ New features easier to add (know where code goes)
✅ Bugs easier to find (smaller files, clear responsibilities)
✅ Testing possible (pure functions can be tested)
✅ Onboarding easier (clear structure, labeled mess)
✅ Refactoring safer (mess is contained)

**Big Win**: Even if we STOP here, we've won! Game is maintainable!

---

## PHASE B: Decouple Messiness (Optional Future)

**Only start if**:
1. Phase A is complete
2. Team still has energy
3. Clear use case for decoupling (e.g., adding multiplayer)

**What it involves**:
- Breaking up messy_action_system into cleaner subsystems
- Making rules engine truly pure (pure functions only)
- Eliminating static variables in AI
- Cleaner state management (not entity-component, just cleaner)

**Duration**: 3-4 months
**Risk**: High (touching complex logic)
**Benefit**: Perfect architecture

**But honestly**: Phase A might be enough! Don't let perfect be enemy of good!

---

## Current Status (Updated 2025-12-18)

### ✅ Completed (Milestones 1-6)
- Pure utilities extracted to `src/core/` (vector_math, geometry, field_layout)
- Ball physics isolated in `src/physics/`
- Entity rendering decoupled in `src/renderer/`
- Actions split: `actions_messy/` (coordinators) + `actions_pure/` (testable)
- AI split: `ai_messy/` (coordinators) + `ai_pure/` (testable)
- Rules extracted: `rules_pure/` (testable)
- **48 unit tests passing**
- **Foundation quality: 8.5/10 - Production Ready**

### 🎯 In Progress (Milestone 7 - Data Renaissance)
- Data-first approach: redesign structures BEFORE adding enums
- Multi-layer testing for safe migration
- Integration tests for behavior validation
- Adapter pattern for safe migration

### 📋 Planned (Milestone 8+)
- Functional dataflow refactor (synchronous, NOT event-driven)
- Overlay/HUD rendering extraction
- Animation state machine extraction
- Optional: Coordinator renames

---

## Practical First Steps (Next 2 Weeks)

### Step 1: Create Directory Structure
```bash
mkdir -p src/core
mkdir -p src/physics
mkdir -p src/renderer
mkdir -p src/game/actions_messy
mkdir -p src/game/actions_pure
mkdir -p src/game/rules_messy
mkdir -p src/game/rules_pure
mkdir -p src/game/ai_messy
mkdir -p src/game/ai_pure
```

Add .gitkeep files to mark structure.

### Step 2: Extract Geometry Functions (Milestone 1)

**From common_logic.c**, extract to `src/core/geometry.c`:
```c
// Pure distance/angle functions
float geometry_distance_3d(Vector3D* a, Vector3D* b);
float geometry_distance_2d_xz(Vector3D* a, Vector3D* b);
float geometry_angle_between(Vector3D* from, Vector3D* to);
int geometry_is_within_radius(Vector3D* pos, Vector3D* center, float radius);
```

**From immutable_world.c**, extract to `src/core/field_layout.c`:
```c
// Pure field geometry
Vector3D field_get_base_position(int base_index);
Vector3D field_get_pitcher_position(void);
int field_is_out_of_bounds(Vector3D* position);
float field_get_base_radius(int base_index);
```

### Step 3: Document Messiness

Create `src/game/actions_messy/README.md`:
```markdown
# Actions Messy System

⚠️ WARNING: This directory contains complex, stateful code.

## What's here
- Pitching, batting, throwing implementation
- Animation state management
- Timing-dependent logic
- Static variables for meters, counters

## Why it's messy
- Tight coupling to StateInfo
- Static state for timing
- Mix of physics and animation

## Future work
Gradually extract pure functions to actions_pure/
Eventually refactor to event-driven system
```

**Benefit**: Future you (or others) know what they're getting into!

---

## Decision Points & Trade-offs

### Should we create game_coordinator.c now?
**Option A**: Wait until Milestone 4 (when we split action_implementation.c)
**Option B**: Create skeleton now, gradually move logic there

**Recommendation**: Wait. Don't add indirection until we know what to dispatch.

### Should we rename common_logic.c?
**Option A**: Keep name, shrink it by extracting pure functions
**Option B**: Rename to player_movement.c (its remaining purpose)

**Recommendation**: Wait until Milestone 2, then rename to match contents.

### Should we keep mutable_world.c?
**Option A**: Keep as high-level coordinator
**Option B**: Merge into game_coordinator.c

**Recommendation**: Keep for now, decide at Milestone 4.

---

## How to Stay Motivated (Milestone Celebrations!)

### After Each Milestone:
1. **Run the game** - Verify it works
2. **Look at metrics** - Count lines saved, functions extracted
3. **Write tests** - For newly extracted pure functions
4. **Update progress** - Update REFACTORING_LOG.md
5. **Commit** - With message like "🎉 Milestone 2: Physics Isolated!"
6. **Take a break** - Seriously, refactoring is marathon not sprint

### Visual Progress Tracker
```
Milestone 1: Foundation        [################____] 80%
Milestone 2: Physics Isolation [####____________] 25%
Milestone 3: Rendering         [____________________]  0%
Milestone 4: Big Split         [____________________]  0%
Milestone 5: Rules             [____________________]  0%
Milestone 6: AI                [____________________]  0%

Overall Phase A Progress: [###_________________] 15%
```

---

## Success Criteria

### Phase A is complete when:
✅ No file >500 lines
✅ All math/physics in pure modules
✅ Messiness is in labeled directories
✅ Can write unit tests for 20+ functions
✅ New dev can understand structure in 1 hour
✅ Game works perfectly

### We know it's working if:
✅ Adding new feature takes less time than before
✅ Bugs are found faster
✅ Code reviews are easier
✅ Team morale is up (clean code is satisfying!)

---

## Conclusion: Your Vision is Spot-On!

Your two-phase strategy is perfect:

**Phase A**: Isolate messiness + Extract purity
- Achievable (16-20 weeks)
- Low risk (small, safe steps)
- High value (huge improvement)
- **Can stop here and still win!**

**Phase B**: Decouple messiness
- Optional
- Higher risk
- Incremental benefit
- Only if we want "perfect"

The hierarchical dispatcher pattern is exactly right:
- main.c → coordinators → subsystems → pure functions
- Data flows in/out, never sideways
- Messiness is isolated in bubbles
- Pure logic is extracted to leaves

**Let's do this!** 🚀

Start with geometry extraction (next 2 weeks). Get that first milestone. Feel the joy of clean code!
