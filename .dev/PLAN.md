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

**Goal:** Replace magic numbers and flag combinations with explicit, semantic enums.

**Why now:** Logic is pure and tested. Time to clean up the STATE that logic operates on.

### Phase 1: Core Enums (High Impact)

#### A. GameMode enum
**Problem:** `period >= 4` appears 5 times, unclear meaning  
**Solution:**
```c
typedef enum {
    GAME_MODE_PERIOD_1 = 0,
    GAME_MODE_PERIOD_2 = 1,
    GAME_MODE_SUPER_INNING = 2,
    GAME_MODE_EXTRA_INNING = 3,
    GAME_MODE_HOMERUN_CONTEST = 4
} GameMode;
```

**Impact:** Replaces all `period >= 4` checks with `gameMode == GAME_MODE_HOMERUN_CONTEST`

---

#### B. BasePosition enum  
**Problem:** `player.base` means different things (0=home start, 4=home after run)  
**Solution:**
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

#### C. PlayerRunnerState enum
**Problem:** Flag soup (`isOnBase`, `out`, `wounded`, `takingFreeWalk` combinations)  
**Solution:**
```c
typedef enum {
    RUNNER_STATE_BATTING,
    RUNNER_STATE_RUNNING,
    RUNNER_STATE_SAFE_ON_BASE,
    RUNNER_STATE_LEADING,
    RUNNER_STATE_TAKING_FREE_WALK,
    RUNNER_STATE_OUT,
    RUNNER_STATE_WOUNDED,
    RUNNER_STATE_SCORED
} PlayerRunnerState;
```

**Impact:** Single source of truth, eliminates complex flag checking

---

#### D. GameEventType enum
**Problem:** `gameInfoEvent = 1/2/3/4...` magic numbers  
**Solution:**
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

### Phase 2: State Componentization (Medium Impact)

Break `StateInfo` god-object into cohesive sub-structs:
- `PhysicsState` - Ball/player positions and velocities
- `RulesState` - Outs, runs, strikes, balls
- `ScoreState` - Team scores, innings
- `AIState` - AI decision inputs

**Benefits:**
- Clear data ownership
- Easier to reason about
- Preparation for Milestone 8 dataflow

---

### Phase 3: Pure Function Contracts (Low Impact, High Value)

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

## Milestone 8: Functional Dataflow & Tooling

**Goal:** Refactor main game loop to follow functional dataflow pattern.

**Prerequisites:** Milestone 7 complete (clean data structures)

### Planned Work:
- State serialization (save/load for debugging)
- Break game loop into distinct phases (input → update → render)
- Pass only relevant sub-states to each phase
- Remove global `StateInfo` dependency from update functions

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

### High Priority (Milestone 7)
- [ ] Magic number `period >= 4` (5 occurrences)
- [ ] Flag combinations for player state
- [ ] Base position semantic confusion
- [ ] Event type magic numbers

### Medium Priority (Milestone 8)
- [ ] Global `StateInfo` dependency in update loops
- [ ] Complex boolean logic in game end detection
- [ ] Implicit state dependencies

### Low Priority (Future)
- [ ] Coordinator files still substantial (~800 lines)
- [ ] Some nested conditionals could be simplified
- [ ] Documentation could always be better

---

## Decision Log

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

### Starting a New Milestone
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
