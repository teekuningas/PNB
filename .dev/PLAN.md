# Refactoring Master Plan

## Current Status: Milestone 6 Complete ✅

**Foundation Quality:** 8.5/10 - Production Ready  
**Next Milestone:** 7 (Data Renaissance)

---

## Completed Milestones

### ✅ Milestone 5: Logic Purification (Actions & AI)
- Extracted all physics calculations to `actions_pure/`
- Extracted all AI decisions to `ai_pure/`
- Created comprehensive unit tests
- **Result:** Clean separation of logic from state

### ✅ Milestone 5.5: The Vantage Point
- Architectural review and documentation
- RNG purification (explicit seed passing)
- Test suite review
- **Result:** Stable baseline, ready for rules extraction

### ✅ Milestone 6: Rules Engine Extraction
- Extracted outs detection to `rules_pure/rules_outs.c` (§33 Pesäkilpa)
- Extracted run scoring to `rules_pure/rules_runs.c` (§41 Juoksu, §42 Kunniajuoksu)
- Extracted strike/ball logic to `rules_pure/rules_strikes.c` (§26 Syöttö)
- **Comprehensive audit completed:**
  - 15 of 18 functions verified across all `*_pure/` modules
  - 1 critical bug found and fixed (baseIndex parameter)
  - All HIGH and MEDIUM risk extractions verified correct
- **Result:** All game "brains" now pure and tested (48 tests passing)

**See `docs/MILESTONE6_COMPLETE.md` for detailed summary.**

---

## Key Semantic Discoveries (from Milestone 6 audit)

**Critical for Milestone 7 enum design:**

1. **`player.base`** = "Where player IS or running FROM" (NOT running TO)
   - Stays at origin during run, updates on arrival
   - This semantic confusion caused the baseIndex bug!

2. **`isOnBase`** = Safety flag (0=running/vulnerable, 1=safe)

3. **`baseIndex = i - 1`** = "Previous base" pattern
   - Used to check if player racing FROM base i-1 TO base i

4. **Magic number `period >= 4`** = Homerun contest detection (5 occurrences)

5. **Flag combinations** = Multiple booleans used to represent single state
   - Example: `isOnBase=0 && out=0 && takingFreeWalk=0` → should be ONE enum

**These discoveries directly inform our enum design below.**

---

## Milestone 7: Data Renaissance (Structure Shapes Logic) 🎯 NEXT

**Goal:** Redesign state data structures FIRST, then add semantic enums to the clean model.

**Philosophy:** Don't add enums to messy structures. Clean the structure, THEN enumerate.

**Why now:** Logic is pure and tested. Time to clean up the STATE that logic operates on.

### Phase 0: Data Model Audit & Design (2 weeks) 🔍 START HERE

**Goal:** Map current data mess, design cleaner model WITHOUT changing code yet.

**Tasks:**
1. **Map flag combinations** - Document all boolean flag usages:
   - `isOnBase` + `out` + `wounded` + `takingFreeWalk` combinations
   - Which combinations exist? Which are invalid?
   - Draw state machine diagram
   
2. **Identify semantic states** - What do combinations MEAN?
   - `isOnBase=0 && out=0 && takingFreeWalk=0` → "RUNNING"
   - `isOnBase=1 && out=0` → "SAFE_ON_BASE"
   - Document all states in DATA_AUDIT.md

3. **Design new model** - Propose cleaner structures:
   - Single `PlayerState` enum instead of 4 flags
   - Explicit `BasePosition` with clear semantics
   - Document in DATA_AUDIT.md

4. **Plan migration strategy** - How to migrate safely:
   - Integration test fixtures (behavior tests)
   - Adapter pattern (old/new coexist temporarily)
   - Incremental rollout plan

**Deliverable:** `docs/DATA_AUDIT.md` with proposed new model + migration plan

---

### Phase 1: Integration Test Foundation (1 week) 🧪

**Goal:** Add high-level behavior tests that survive data structure changes.

**Why:** Our 48 unit tests check pure functions (good!) but are implementation-specific.
Integration tests check BEHAVIOR independent of data structure.

**Tasks:**
1. Create `tests/integration/` directory
2. Add fixture system (load game scenarios from JSON/simple format)
3. Write 10-15 integration tests for critical scenarios:
   - Runner forced out at base
   - Run scoring
   - Strike/ball detection
   - Homerun contest mode
   
**Example:**
```c
TEST(runner_forced_out_scenario) {
    GameState* game = load_fixture("runner_at_home_ball_at_first.json");
    assert_player_is_out(game, 0);
    assert_outs_count(game, 1);
}
```

**Safety:** These tests will pass BEFORE and AFTER data migration!

---

### Phase 2: Data Structure Migration (3-4 weeks) 🔧

**Goal:** Implement new data model using adapter pattern for safety.

#### Step A: Add New Fields (Keep Old)
```c
struct PlayerInfo {
    // OLD (keep for now):
    int base;
    int isOnBase;
    int out;
    int wounded;
    int takingFreeWalk;
    
    // NEW (add alongside):
    PlayerState state;     // Single enum replacing flags
    BasePosition basePos;  // Clearer semantics
};
```

#### Step B: Create Adapter Functions
```c
// Adapters read NEW fields, provide OLD interface
int player_is_on_base(const PlayerInfo* p) {
    return (p->state == PLAYER_STATE_SAFE_ON_BASE);
}
```

#### Step C: Update Coordinators to Use Adapters
- Replace direct field access with adapter calls
- Run tests → should pass
- Both old and new fields coexist

#### Step D: Remove Old Fields
- Once all code uses adapters, remove old fields
- Update adapters to direct accessors
- Tests still pass!

**Safety:** At each step, ALL tests (unit + integration) must pass.

---

### Phase 3: Core Enums (High Impact) ✨ NOW WE ADD ENUMS

#### A. PlayerState enum (CLEAN MODEL)
**Problem:** Flag soup (`isOnBase`, `out`, `wounded`, `takingFreeWalk` combinations)  
**Solution:** Single enum after data migration
```c
typedef enum {
    PLAYER_STATE_BATTING,
    PLAYER_STATE_RUNNING,
    PLAYER_STATE_SAFE_ON_BASE,
    PLAYER_STATE_LEADING,
    PLAYER_STATE_TAKING_FREE_WALK,
    PLAYER_STATE_OUT,
    PLAYER_STATE_WOUNDED,
    PLAYER_STATE_SCORED
} PlayerState;
```

**Impact:** Single source of truth, eliminates complex flag checking

---

#### B. BasePosition enum (CLEAN MODEL)
**Problem:** `player.base` means different things (0=home start, 4=home after run)  
**Solution:** Explicit semantics after data migration
```c
typedef enum {
    BASE_HOME = 0,
    BASE_FIRST = 1,
    BASE_SECOND = 2,
    BASE_THIRD = 3,
    BASE_HOME_SCORED = 4,  // Distinct from BASE_HOME
    BASE_NONE = -1
} BasePosition;
```

**Impact:** Makes base semantics explicit, prevents future bugs like baseIndex issue

---

#### C. GamePeriodType enum (SIMPLE ADDITION)
**Problem:** `period >= 4` appears 5 times, unclear meaning  
**Solution:** Can add immediately (doesn't require data migration)
```c
typedef enum {
    PERIOD_NORMAL_1 = 0,
    PERIOD_NORMAL_2 = 1,
    PERIOD_SUPER_INNING = 2,
    PERIOD_EXTRA_INNING = 3,
    PERIOD_HOMERUN_CONTEST = 4
} GamePeriodType;
```

**Impact:** Replaces all `period >= 4` checks with `period == PERIOD_HOMERUN_CONTEST`

---

#### D. GameEventType enum (SIMPLE ADDITION)
**Problem:** `gameInfoEvent = 1/2/3/4...` magic numbers  
**Solution:** Can add immediately (doesn't require data migration)
```c
typedef enum {
    EVENT_NONE = 0,
    EVENT_OUT = 1,
    EVENT_WOUNDED = 2,
    EVENT_RUN_SCORED = 3,
    EVENT_FOUL_PLAY = 4,
    // ... others
} GameEventType;
```

**Impact:** Self-documenting event system

---

### Phase 4: State Componentization (Optional, Medium Impact)

Break `StateInfo` god-object into cohesive sub-structs:
- `PhysicsState` - Ball/player positions and velocities
- `RulesState` - Outs, runs, strikes, balls
- `ScoreState` - Team scores, innings
- `AIState` - AI decision inputs

**Benefits:**
- Clear data ownership
- Easier to reason about
- Preparation for Milestone 8 dataflow

**Note:** This is optional. May defer to Milestone 8 if data migration is complex enough.

---

### Phase 5: Pure Function Contracts (Low Impact, High Value)

Define explicit input structs for pure functions:
```c
typedef struct {
    int powerCount;
    int angleCount;
    float ballVelocityY;
    int swingMax;
    int loadMax;
} BattingContext;

float calculate_batting_vertical_angle(const BattingContext* ctx);
```

**Benefits:**
- Self-documenting
- Type-safe
- Easier to extend

---

## Milestone 7 Testing Strategy

### Safety Net (Multi-Layer)

1. **Integration Tests** (Behavior, data-independent)
   - 10-15 high-level scenario tests
   - Check WHAT happens, not HOW
   - Survive data structure changes

2. **Unit Tests** (Existing 48 tests)
   - Test pure function contracts
   - Keep contracts stable during migration
   - Update only after migration complete

3. **Adapter Pattern** (Temporary bridge)
   - Old and new fields coexist
   - All tests pass at each step
   - Remove adapters when migration done

4. **Manual Gameplay** (Final validation)
   - Play through game after each phase
   - Verify no behavior changes
   - Essential for catching integration bugs

**Golden Rule:** ALL tests (unit + integration) must pass after EACH commit.

---

## Milestone 8: Functional Dataflow (NOT Event-Driven!)

**Goal:** Refactor main game loop to follow **synchronous functional dataflow** pattern.

**CRITICAL:** We do NOT want event buses, message passing, or async callbacks!

### What We Want: The Breathing Pattern

```c
// main.c - The Orchestrator
while (running) {
    InputState input = poll_input();           // IN: Read keyboard
    
    StateInfo new_state = update_game(         // BREATHE IN: Function calls flow down
        current_state,
        input
    );
    // Inside update_game:
    //   - update_physics(state, input)
    //   - update_rules(state)
    //   - update_ai(state)
    //   All explicit function calls, synchronous!
    
    render_game(new_state);                     // BREATHE OUT: Render from state
    
    current_state = new_state;
}
```

### What We DON'T Want:
❌ Event bus / message queue  
❌ Observer pattern  
❌ Publish/subscribe  
❌ Async callbacks  
❌ "Fire events" that systems listen to  

### What We DO Want:
✅ Explicit function calls (main → coordinators → subsystems → pure)  
✅ Synchronous execution (easy to debug, single call stack)  
✅ Data flows IN (state, input) and OUT (new state)  
✅ Top-down control flow (no sideways messaging)  
✅ Pure functions dominate (80% of code)  

**Prerequisites:** Milestone 7 complete (clean data structures)

### Planned Work:
- State serialization (save/load for debugging)
- Break game loop into distinct phases (input → update → render)
- Pass only relevant sub-states to each phase
- Remove global `StateInfo` dependency from update functions
- **Overlay/HUD rendering extraction** (from game_screen.c)
- **Animation state machine extraction** (from common_logic.c)

**Status:** Planning phase, awaiting Milestone 7 completion

---

## Testing Strategy

### Current Coverage (Milestone 6)
- ✅ 48 unit tests for pure functions
- ✅ All pass, good edge case coverage
- ❌ Gap: Integration tests (baseIndex bug exposed this)

### Milestone 7 Testing
- Add integration tests for enum conversions
- Test flag-to-enum migration logic
- Ensure no behavioral changes during refactor

---

## Technical Debt Tracker

### High Priority (Milestone 7 - Data Renaissance)
- [ ] **Phase 0:** Data model audit (map flag combinations, design clean model)
- [ ] **Phase 1:** Integration test foundation (behavior tests)
- [ ] **Phase 2:** Data structure migration (adapter pattern)
- [ ] **Phase 3:** Add enums to clean model
- [ ] Magic number `period >= 4` (5 occurrences) → `GamePeriodType` enum
- [ ] Flag combinations for player state → `PlayerState` enum
- [ ] Base position semantic confusion → `BasePosition` enum
- [ ] Event type magic numbers → `GameEventType` enum

### Medium Priority (Milestone 8 - Functional Dataflow)
- [ ] Global `StateInfo` dependency in update loops
- [ ] Complex boolean logic in game end detection
- [ ] Implicit state dependencies
- [ ] **Overlay/HUD rendering extraction** (from game_screen.c)
- [ ] **Animation state machine extraction** (from common_logic.c)

### Low Priority (Future)
- [ ] Coordinator files still substantial (~800 lines)
- [ ] Some nested conditionals could be simplified
- [ ] Documentation could always be better
- [ ] Coordinator file renames (semantic names)

---

## Decision Log

### 2025-12-18: Milestone 7 Strategy Update
**Decision:** Data-first approach with multi-layer testing  
**Rationale:**
- Don't add enums to messy structures ("lipstick on a pig")
- Redesign state model FIRST, then enumerate clean model
- Integration tests ensure behavior preservation during migration
- Adapter pattern allows safe incremental migration
- Functional dataflow confirmed (NOT event-driven, synchronous calls only)

### 2025-12-17: Milestone 6 Audit Results
**Decision:** Proceed to Milestone 7 (Data Renaissance)  
**Rationale:** 
- Foundation verified solid (8.5/10)
- One bug found and fixed
- Semantic insights gained during audit directly inform enum design
- All HIGH and MEDIUM risk code verified correct

### 2025-11-XX: Vertical-then-Horizontal Methodology
**Decision:** Extract logic first (vertical), audit thoroughly second (horizontal)  
**Rationale:**
- Allows fast progress without getting stuck
- Comprehensive audit catches issues tests miss
- Proven effective (caught critical baseIndex bug)

---

## Notes for Future Sessions

### Starting Milestone 7 (Data Renaissance)
1. Read this PLAN.md (Phase 0 starts with data audit)
2. Read `docs/MILESTONE6_COMPLETE.md` for semantic insights
3. Read `docs/DATA_AUDIT.md` for existing proposals
4. **Start with Phase 0:** Map current data, design clean model
5. **Then Phase 1:** Add integration tests for safety
6. **Then Phase 2:** Migrate data structures with adapters
7. Update TODO.md with atomic tasks

### Rendering System Refactors (Deferred to Milestone 8+)
**In-game rendering lives in:**
- `src/game/game_screen.c` (590 lines) - Overlay/HUD drawing (drawStatistics)
- `src/game/common_logic.c` (996 lines) - Player animation state machines
- `src/renderer/` - Already clean! (player_renderer.c, ball_renderer.c)

**What needs love (later):**
- Overlay/HUD: Hardcoded positions, magic numbers, mixed with 3D camera
- Animation: State machines mixed with movement logic
- gameInfoEvent: Magic numbers (1=OUT, 2=WOUNDED, 3=RUN)

**Defer to:** Milestone 8+ (after data structures are clean)

### Using the Archive
1. Read this PLAN.md
2. Read `docs/MILESTONE6_COMPLETE.md` for context
3. Check `docs/DATA_AUDIT.md` for Milestone 7 specifics
4. Update TODO.md with atomic tasks

### Using the Archive
- Detailed audit reports in `docs/archive/`
- Reference if you need to understand HOW we verified something
- Most day-to-day work only needs PLAN.md and MILESTONE6_COMPLETE.md

### The Rhythm
```
Vertical Phase:
  - Move fast, extract logic
  - Get tests passing
  - Don't over-analyze

Horizontal Phase:
  - Systematic audit
  - Compare original vs extracted
  - Manual testing
  - Fix any bugs found
  - Document learnings

Then: Next milestone!
```

---

*For detailed milestone achievements, see `docs/MILESTONE6_COMPLETE.md`*  
*For audit methodology and findings, see `docs/archive/`*
