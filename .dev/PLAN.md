# PNB Development Plan

## Current Phase: Referee Consolidation - Final Cleanup (Phase 2B)

**Status:** Core refactoring COMPLETE ✅ | Out of bounds bug FIXED ✅ | Cleanup & struct split next 🔧

**Immediate Goal:** Complete Milestone 17 cleanup before moving to Milestone 18 (Physics/State Split). This means: (1) Split GameControl into PitchState and FlowControl, (2) Remove all dead code, (3) Consolidate timers in referee, (4) Document patterns, (5) Make referee.c crystal clear.

**The Golden Rule:**
1.  **One Way Flow:** Physics → Events → Referee → Legal State → Reconciliation → Physics
2.  **Limited Scope:** Referee MUST NOT mutate other structures (e.g., `PlayerInfo`, `BallInfo`, `pRAI`, `AIState`). Its output is strictly the Legal State (`RefereeState`, `HalfInningState`, `PitchState`).

### Ownership Map (After Struct Split)

| Structure | Owner (Writer) | Readers | Lifecycle |
| :--- | :--- | :--- | :--- |
| `GameEvents` | Physics/Actions | Referee | Clear every frame |
| **`PitchState`** | **Referee ONLY** | Everyone | Reset at pitch start |
| `FlowControl` | Menus/Input | Game loop, Referee | User-controlled |
| `RefereeState` | **referee.c ONLY** | Reconcile, UI, AI | Persistent |
| `HalfInningState` | **referee.c ONLY** | Everyone | Persistent (inning-scoped) |

---

## 📅 Immediate Priorities (Next 2-3 Sessions)

### 🎯 SESSION GOAL: Complete Phase 2B Cleanup + Struct Split

**Estimated Time:** 5-6 hours over 2-3 sessions
**Prerequisites:** All tests passing (✅ Currently: 51 unit, 11 integration)
**Objective:** Zero technical debt + clean struct separation before Milestone 18

---

## 🔧 Phase 2B: Cleanup Tasks (Prioritized)

### **Priority 0: Split GameControl → PitchState + FlowControl** ⏱️ 5 hours

**Status:** Out of bounds bug revealed mixed responsibilities in GameControl ✅

**Problem:** `GameControl` mixes three concerns:
1. Referee decisions (pitch-scoped: `catchHasBeenMade`, `outOfBounds`)
2. Flow control (user gates: `pause`, `waitingForBatterDecision`)
3. Coordination (consumed by reconcile: `pitchResolutionProcessed`)

**Solution:** Split into two focused structs with clear ownership.

#### Step 0.1: Preparation - Add Prefixes (30 min)

**Before the split, prefix all GameControl fields for clarity:**

```bash
# In GameControl struct definition (globals.h)
pause                      → flow_pause
waitingForBatterDecision   → flow_waitingForBatterDecision
waitingForFreeWalkDecision → flow_waitingForFreeWalkDecision
catchHasBeenMade          → pitch_catchHasBeenMade
hasBallHitGround          → pitch_hasBallHitGround
outOfBounds               → pitch_outOfBounds
pitchResolutionProcessed  → pitch_resolutionProcessed
freeWalkCalculationMade   → pitch_freeWalkCalculationMade
freeWalkIndex             → pitch_freeWalkIndex
freeWalkBase              → pitch_freeWalkBase
```

**Commands:**
```bash
# Dry run first to see what would change
sed -n 's/gameControl\.pause/gameControl.flow_pause/gp' src/game/*.c

# Then apply (be careful with partial matches!)
find src/game -name "*.c" -exec sed -i 's/gameControl\.pause\b/gameControl.flow_pause/g' {} \;
find src/game -name "*.c" -exec sed -i 's/gameControl\.waitingForBatterDecision\b/gameControl.flow_waitingForBatterDecision/g' {} \;
# ... (repeat for all fields)

# Update struct definition in globals.h manually
# Update struct initialization in common_logic.c manually
```

**Note:** Manual review required after sed! Some fields like `pause` might have false positives.

**Checklist:**
- [ ] Add prefixes to all GameControl fields
- [ ] Update struct definition in globals.h
- [ ] Update initialization in common_logic.c
- [ ] Compile and run tests
- [ ] Commit: "Add prefixes to GameControl fields (prep for split)"

#### Step 0.2: Create New Structs (1 hour)

**Add to `globals.h`:**

```c
// Pitch-scoped state (reset at pitch start, written by referee)
typedef struct _PitchState {
    // Referee decisions about THIS pitch
    int catchHasBeenMade;     // Fly ball was caught
    int hasBallHitGround;     // Ball has touched ground (first bounce tracking)
    int outOfBounds;          // First bounce was out of bounds (foul play)
    int resolutionProcessed;  // Referee has adjudicated strike/ball
    
    // Free walk decision data
    int freeWalkCalculationMade;
    int freeWalkIndex;        // Which player offered free walk (-1 = none)
    BaseID freeWalkBase;      // From which base (BASE_NONE = none)
} PitchState;

// User interaction flow control (not pitch-scoped, not referee's concern)
typedef struct _FlowControl {
    int pause;
    int waitingForBatterDecision;
    int waitingForFreeWalkDecision;
} FlowControl;
```

**Add to `MatchSession`:**
```c
struct _MatchSession {
    // ... existing fields ...
    PitchState pitchState;
    FlowControl flowControl;
    // GameControl gameControl;  // DELETE this line
};
```

**Initialize in `common_logic.c`:**

```c
// In initializeTemporaryGameAnalysisInfo() - Reset PitchState at pitch start
void initializeTemporaryGameAnalysisInfo(MatchSession* match) {
    // ... existing code ...
    
    // Reset PitchState for new pitch
    match->pitchState.catchHasBeenMade = 0;
    match->pitchState.hasBallHitGround = 0;
    match->pitchState.outOfBounds = 0;
    match->pitchState.resolutionProcessed = 0;
    match->pitchState.freeWalkCalculationMade = 1; // Default: not calculated yet
    match->pitchState.freeWalkIndex = -1;
    match->pitchState.freeWalkBase = BASE_NONE;
}

// In game setup - Initialize FlowControl (once at game start)
void initializeFlowControl(FlowControl* flow) {
    flow->pause = 0;
    flow->waitingForBatterDecision = 0;
    flow->waitingForFreeWalkDecision = 0;
}
```

**Checklist:**
- [ ] Add `PitchState` struct to globals.h
- [ ] Add `FlowControl` struct to globals.h
- [ ] Add fields to `MatchSession`
- [ ] Initialize `PitchState` in `initializeTemporaryGameAnalysisInfo()`
- [ ] Initialize `FlowControl` in game setup
- [ ] Commit: "Add PitchState and FlowControl structs"

#### Step 0.3: Migrate All References (2 hours)

**Strategy:** Use sed for bulk replace, then manual verification.

```bash
# Replace gameControl.pitch_* → pitchState.*
find src -name "*.c" -exec sed -i 's/gameControl\.pitch_catchHasBeenMade\b/pitchState.catchHasBeenMade/g' {} \;
find src -name "*.c" -exec sed -i 's/gameControl\.pitch_hasBallHitGround\b/pitchState.hasBallHitGround/g' {} \;
find src -name "*.c" -exec sed -i 's/gameControl\.pitch_outOfBounds\b/pitchState.outOfBounds/g' {} \;
find src -name "*.c" -exec sed -i 's/gameControl\.pitch_resolutionProcessed\b/pitchState.resolutionProcessed/g' {} \;
find src -name "*.c" -exec sed -i 's/gameControl\.pitch_freeWalkCalculationMade\b/pitchState.freeWalkCalculationMade/g' {} \;
find src -name "*.c" -exec sed -i 's/gameControl\.pitch_freeWalkIndex\b/pitchState.freeWalkIndex/g' {} \;
find src -name "*.c" -exec sed -i 's/gameControl\.pitch_freeWalkBase\b/pitchState.freeWalkBase/g' {} \;

# Replace gameControl.flow_* → flowControl.*
find src -name "*.c" -exec sed -i 's/gameControl\.flow_pause\b/flowControl.pause/g' {} \;
find src -name "*.c" -exec sed -i 's/gameControl\.flow_waitingForBatterDecision\b/flowControl.waitingForBatterDecision/g' {} \;
find src -name "*.c" -exec sed -i 's/gameControl\.flow_waitingForFreeWalkDecision\b/flowControl.waitingForFreeWalkDecision/g' {} \;

# Also need to update function signatures that take GameControl*
# This requires manual work: change to PitchState* or FlowControl*
```

**Critical Manual Steps:**
- Update function signatures in `referee.c`
- Update function signatures in `mutable_world.c`
- Update all struct access patterns
- Update state validator to log both structs

**Checklist:**
- [ ] Run sed commands for bulk replacement
- [ ] Update `referee.c` function signatures (use `PitchState*`)
- [ ] Update `mutable_world.c` function signatures
- [ ] Update `game_analysis.c` flow control references
- [ ] Update menu code to use `FlowControl*`
- [ ] Compile and fix errors
- [ ] Commit: "Migrate GameControl → PitchState + FlowControl"

#### Step 0.4: Remove Old GameControl (30 min)

```bash
# Search for any remaining references
grep -rn "GameControl" src/

# Delete struct definition from globals.h
# Delete field from MatchSession
```

**Checklist:**
- [ ] Verify no remaining `GameControl` references
- [ ] Delete `GameControl` typedef from globals.h
- [ ] Delete `gameControl` field from MatchSession
- [ ] Run all tests
- [ ] Commit: "Remove old GameControl struct"

#### Step 0.5: Add State Validator Logging (30 min)

**Update `state_validator.c` to log new structs:**

```c
// Add to dumpState()
fprintf(f, "%s\"pitchState\": {\n", sp);
fprintf(f, "%s  \"catchHasBeenMade\": %d,\n", sp, game->pitchState.catchHasBeenMade);
fprintf(f, "%s  \"hasBallHitGround\": %d,\n", sp, game->pitchState.hasBallHitGround);
fprintf(f, "%s  \"outOfBounds\": %d,\n", sp, game->pitchState.outOfBounds);
fprintf(f, "%s  \"resolutionProcessed\": %d,\n", sp, game->pitchState.resolutionProcessed);
fprintf(f, "%s  \"freeWalkCalculationMade\": %d,\n", sp, game->pitchState.freeWalkCalculationMade);
fprintf(f, "%s  \"freeWalkIndex\": %d,\n", sp, game->pitchState.freeWalkIndex);
fprintf(f, "%s  \"freeWalkBase\": %d\n", sp, game->pitchState.freeWalkBase);
fprintf(f, "%s},\n", sp);

fprintf(f, "%s\"flowControl\": {\n", sp);
fprintf(f, "%s  \"pause\": %d,\n", sp, game->flowControl.pause);
fprintf(f, "%s  \"waitingForBatterDecision\": %d,\n", sp, game->flowControl.waitingForBatterDecision);
fprintf(f, "%s  \"waitingForFreeWalkDecision\": %d\n", sp, game->flowControl.waitingForFreeWalkDecision);
fprintf(f, "%s},\n", sp);
```

**Checklist:**
- [ ] Add `pitchState` logging to state validator
- [ ] Add `flowControl` logging to state validator
- [ ] Test debug log output
- [ ] Commit: "Add PitchState and FlowControl to debug logging"

#### Step 0.6: Add Reset Validation Tests (1 hour)

**Goal:** Ensure `PitchState` is reset at pitch start and `GameEvents` are cleared every frame.

**Challenge:** C doesn't have reflection, so we can't dynamically iterate struct fields.

**Solution:** Use X-Macros pattern for testability!

**In `globals.h` (or new `pitch_state.h`):**

```c
// Define all PitchState fields as X-Macro
#define PITCH_STATE_FIELDS \
    X(int, catchHasBeenMade, 0) \
    X(int, hasBallHitGround, 0) \
    X(int, outOfBounds, 0) \
    X(int, resolutionProcessed, 0) \
    X(int, freeWalkCalculationMade, 1) \
    X(int, freeWalkIndex, -1) \
    X(BaseID, freeWalkBase, BASE_NONE)

// Generate struct definition
typedef struct _PitchState {
#define X(type, name, default_val) type name;
    PITCH_STATE_FIELDS
#undef X
} PitchState;

// Generate reset function
static inline void resetPitchState(PitchState* state) {
#define X(type, name, default_val) state->name = default_val;
    PITCH_STATE_FIELDS
#undef X
}
```

**Test in `tests/test_pitch_state_reset.c`:**

```c
int test_pitch_state_reset_after_pitch(void) {
    MatchSession match;
    memset(&match, 0xFF, sizeof(match)); // Fill with garbage
    
    // Manually dirty the state
    match.pitchState.catchHasBeenMade = 1;
    match.pitchState.hasBallHitGround = 1;
    match.pitchState.outOfBounds = 1;
    
    // Call initialization
    initializeTemporaryGameAnalysisInfo(&match);
    
    // Verify all fields are reset using X-Macro
#define X(type, name, expected) \
    ASSERT_EQ(expected, match.pitchState.name, "pitchState." #name " not reset");
    PITCH_STATE_FIELDS
#undef X
    
    return TEST_PASSED;
}

int test_game_events_cleared_every_frame(void) {
    GameEvents events;
    
    // Set all events to 1
    events.catchMade = 1;
    events.ballHitGround = 1;
    events.ballHitByBat = 1;
    // ... all others
    
    // Clear them
    clearFrameEvents(&events);
    
    // Verify all are 0
    ASSERT_EQ(0, events.catchMade, "catchMade not cleared");
    ASSERT_EQ(0, events.ballHitGround, "ballHitGround not cleared");
    // ... test all fields
    
    return TEST_PASSED;
}
```

**Alternative (simpler but manual):**

Just write explicit tests for each field without X-Macros. Less elegant but easier to understand.

**Checklist:**
- [ ] Consider X-Macro pattern (optional - advanced)
- [ ] Create `tests/test_pitch_state_reset.c`
- [ ] Test `initializeTemporaryGameAnalysisInfo()` resets all fields
- [ ] Test `clearFrameEvents()` clears all fields
- [ ] Add to test runner
- [ ] Run tests
- [ ] Commit: "Add PitchState reset validation tests"

#### Summary: Priority 0 Benefits

✅ **Clear ownership:** Referee owns PitchState, never touches FlowControl  
✅ **Clear lifecycle:** PitchState = pitch-scoped, FlowControl = user-controlled  
✅ **Better naming:** `pitchState.outOfBounds` is self-documenting  
✅ **Validation:** Tests ensure reset logic works  
✅ **Phase 2B ready:** Clean foundation for remaining cleanup

---

### **Priority 1: Remove Dead Code** ⏱️ 30 minutes

#### A. Delete Unused Pure Functions (10 functions, ~115 lines)
**Location:** `src/game/rules_pure/`, `src/game/actions_pure/`, `src/game/ai_pure/`

**Delete These:**
```
1. calculate_batted_ball_velocity (batting_physics.c) - 20 lines, 0 calls
2. should_ai_drop_ball (catching_ai_strategy.c) - 13 lines, 0 calls
3. determine_pitch_result (rules_strikes.c) - 12 lines, 0 calls
4. base_get_prev (base_logic.c) - 6 lines, 0 calls
5. base_is_safe_haven (base_logic.c) - 2 lines, 0 calls
6. base_can_advance (base_logic.c) - 2 lines, 0 calls
7. base_is_at_least (base_logic.c) - 2 lines, 0 calls
8. base_cmp (base_logic.c) - 4 lines, 0 calls
```

**Also Delete Corresponding Tests:**
- `test_batted_ball_velocity` in `test_batting_physics.c`
- `test_should_ai_drop_ball_scenario` in `test_catching_ai_strategy.c`

**Why:** These functions are completely unused (verified by grep). Removing reduces maintenance burden with zero coverage loss.

**Checklist:**
- [ ] Delete function implementations
- [ ] Delete function declarations from headers
- [ ] Delete test cases
- [ ] Run tests to verify (expect 49 passing)
- [ ] Commit: "Remove 10 unused pure functions"

---

### **Priority 2: Remove Low-Value Test Files** ⏱️ 15 minutes

**Delete These Test Files:**

1. **`tests/test_debug_logging.c`** (58 lines, 6 assertions)
   - Tests StateValidator infrastructure, not game logic
   - Integration tests provide implicit coverage

2. **`tests/test_rules_referee.c`** (107 lines, 0 test framework assertions!)
   - Uses `assert()` directly instead of test macros
   - Duplicates integration test coverage
   - 11 integration scenarios thoroughly test referee

**Why:** Reduces test maintenance without losing coverage. Integration tests are better for referee behavior.

**Checklist:**
- [ ] Delete `tests/test_debug_logging.c`
- [ ] Delete `tests/test_rules_referee.c`
- [ ] Update `tests/test_runner.c` to remove includes
- [ ] Run tests to verify (expect 47 passing)
- [ ] Commit: "Remove redundant test files"

---

### **Priority 3: Consolidate Timers in Referee** ⏱️ 45 minutes

**Problem:** Referee still references `gameFlowState.endOfInningCounter`
**Solution:** Use existing `referee.endInningTimer` and `referee.nextPairTimer`

**Files to Modify:**

1. **`src/game/referee.c:331`**
   ```c
   // Change:
   game->gameFlowState.endOfInningCounter == -1
   // To:
   referee->endInningTimer == -1
   ```

2. **`src/game/game_analysis.c`** - Update `checkIfEndOfInning()` and `checkIfNextPair()`
   - Read `referee->endInningTimer` instead of `gameFlowState.endOfInningCounter`
   - Read `referee->nextPairTimer` instead of `gameFlowState.nextPairCounter`
   - These timers should be READ ONLY by game_analysis (not set)

**Decision Needed:** Should end-of-inning/next-pair **detection** move to referee?
- **Current:** game_analysis detects conditions, sets timer
- **Proposed:** Referee detects, sets timer; game_analysis reads and executes transitions
- **Defer to:** Next session discussion (not blocking)

**Checklist:**
- [ ] Update referee.c to use referee->endInningTimer
- [ ] Update game_analysis.c to read from referee timers
- [ ] Run integration tests (especially end-of-inning scenarios)
- [ ] Commit: "Consolidate flow timers in referee state"

---

### **Priority 4: Clean Up Batting System Wounding Cancellation** ⏱️ 30 minutes

**Location:** `src/game/actions_messy/batting_system.c:436-437`

**Current (Pre-Referee Code Writes to Referee):**
```c
stateInfo->match->referee.woundingEvaluationActive = 0;
stateInfo->match->referee.woundingEvaluationTimer = -1;
```

**Problem:** Pre-referee code clearing referee state

**Options:**
- A) Keep it (it's reacting to physical event: player reached base)
- B) Let referee monitor this automatically (check if vulnerable player reached base safely)

**Recommended:** Option B - Referee should detect when evaluation should end

**Implementation:**
```c
// In referee.c update_wounding_logic():
// Add check: If marked player reached base safely, cancel evaluation
if (referee->woundingEvaluationActive) {
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        if (referee->woundingPlayersMarked[i]) {
            // If this player is now safe on a base, cancel evaluation
            if (game->playerInfo[i].bTPI.state == PLAYER_STATE_ON_BASE) {
                referee->woundingEvaluationActive = 0;
                referee->woundingEvaluationTimer = -1;
                // Clear all markings
                for (int j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
                    referee->woundingPlayersMarked[j] = 0;
                }
                break;
            }
        }
    }
}
```

**Checklist:**
- [ ] Add safety check to referee wounding logic
- [ ] Remove lines from batting_system.c
- [ ] Test wounding scenarios (integration tests)
- [ ] Commit: "Move wounding cancellation to referee"

---

### **Priority 5: Document Referee Structure** ⏱️ 30 minutes

**Goal:** Make referee.c self-documenting with clear sections

**Add Section Comments to `src/game/referee.c`:**

```c
// ============================================================================
// SECTION 1: HELPER UTILITIES (Lines 1-130)
// ============================================================================
// - update_foul_play_logic()
// - update_wounding_logic()
// - Helper functions for rule checks

// ============================================================================
// SECTION 2: SAFETY PIPELINE (Lines 130-320)
// ============================================================================
// - update_safety_status()
// - update_force_outs_and_tuplahaava()
// - Safety gain/loss logic (§33-§37)

// ============================================================================
// SECTION 3: SCORING LOGIC (Lines 320-450)
// ============================================================================
// - update_runs()
// - Regular runs (§41) and Run of Honor (§42)

// ============================================================================
// SECTION 4: STRIKE/PITCH RESOLUTION (Lines 450-550)
// ============================================================================
// - update_strikes()
// - Pitch result determination
// - Free walk detection

// ============================================================================
// SECTION 5: MAIN UPDATE & INITIALIZATION (Lines 550-642)
// ============================================================================
// - Referee_Update() - Main entry point
// - initializeRefereeState()
```

**Checklist:**
- [ ] Add section headers to referee.c
- [ ] Add brief docstrings to each helper function
- [ ] Document timer ownership rules in comments
- [ ] Commit: "Document referee.c structure"

---

### **Priority 6: Inline Trivial Functions** ⏱️ 20 minutes

**Optional but Recommended**

**Inline These (1 call site each):**

1. **`should_change_batter_on_strikes()`** → `game_analysis.c:116`
   ```c
   // Replace:
   if (should_change_batter_on_strikes(&(stateInfo->match->halfInningState)))
   // With:
   if (stateInfo->match->halfInningState.strikes >= 3)
   ```

2. **`player_is_protected()`** → `base_logic.c:70` (used internally)
   ```c
   // Replace:
   if (player_is_protected(state))
   // With:
   if (state == PLAYER_STATE_ON_BASE || state == PLAYER_STATE_AT_BAT)
   ```

**Why:** Single call site + trivial logic = unnecessary abstraction

**Checklist:**
- [ ] Inline both functions
- [ ] Remove from source files
- [ ] Remove from headers
- [ ] Remove tests
- [ ] Run tests (expect 45 passing)
- [ ] Commit: "Inline trivial helper functions"

---

### **Priority 7: Create Pattern Documentation** ⏱️ 30 minutes

**Create:** `docs/REFEREE_PATTERN.md`

**Contents:**
1. **Event → Referee → State Flow**
   - Diagram showing data flow
   - Event emission rules (pre-referee)
   - Decision making (referee)
   - Action execution (post-referee)

2. **Timer Ownership Rules**
   - When timer lives in referee (pending decision)
   - When timer lives in reconciliation (delayed action)
   - Examples: wounding vs out of bounds

3. **Initialization Phase Exceptions**
   - Document legitimate pre-referee writes
   - Pitch release snapshots (pitching_system.c)
   - Batter initialization (batting_system.c)
   - Setup functions (common_logic.c)

4. **Examples of Proper Usage**
   - Good: Event emission
   - Good: Referee decision
   - Good: Post-referee action
   - Bad: Pre-referee writes to referee state

**Checklist:**
- [ ] Create docs/REFEREE_PATTERN.md
- [ ] Add diagrams (ASCII art is fine)
- [ ] Document both timer patterns
- [ ] Link from PLAN.md
- [ ] Commit: "Document Referee Supremacy pattern"

---

## ✅ Phase 2B Success Criteria

- [ ] No `gameFlowState` timer references in `referee.c`
- [ ] No pre-referee writes to referee state (except documented initialization)
- [ ] Zero unused functions in `rules_pure/`, `actions_pure/`, `ai_pure/`
- [ ] `referee.c` has clear section organization with comments
- [ ] Referee pattern documented in `docs/REFEREE_PATTERN.md`
- [ ] All tests passing (expect ~45 unit tests after cleanup)
- [ ] Git history shows incremental, well-documented commits

**Final Metrics:**
- Code reduction: ~280 lines
- Test reduction: ~165 lines  
- Coverage loss: 0%
- Maintenance burden: -20%
- Code clarity: +40%

---

## 📅 Roadmap & Long-Term Vision

### ✅ Milestone 16: Structural Reorganization (Completed Jan 2026)
- **Completed:**
    - Split `GameControlFlags` into `GameEvents` (transient) + `GameControl` (stateful).
    - Established strict Event -> Referee -> State pipeline.
    - Verified event system with 60+ passing tests.

### ✅ Milestone 17: Referee Consolidation (95% Complete - Cleanup Remaining)

**Goal:** Complete the transition of rule logic to `referee.c` and establish clean patterns.

#### Phase 2A: Core Rule Migration ✅ COMPLETE
- ✅ **Strike/Ball Counting:** Moved to `referee.c`
- ✅ **Free Walk Logic:** Moved to `referee.c`
- ✅ **Out of Bounds:** Fully refactored to Referee Supremacy pattern
  - Decision immediate → Timer in reconciliation (mutable_world.c)
  - Removed `outOfBoundsCounter`, `foulPlayEventFlag`
- ✅ **Wounding Logic:** Fully refactored to Referee Supremacy pattern
  - Decision pending → Timer in referee (monitors ball drop)
  - Moved from game_analysis.c to referee.c
- ✅ **Pattern Established:** Physics → Events → Referee → Decisions → Reconciliation
- ✅ **Tests:** All 51 unit + 11 integration tests passing

**Code Changes (Phase 2A):**
- Removed ~250 lines from game_analysis.c
- Added ~150 lines to referee.c
- Net: 17 files changed, 282 insertions(+), 319 deletions(-)

#### Phase 2B: Cleanup & Consolidation 🔧 IN PROGRESS (See Section Above)
**Status:** Core refactoring complete, cleanup tasks identified and documented

**Remaining Work:**
1. Remove dead code (10 unused functions)
2. Delete redundant tests (2 test files)
3. Consolidate gameFlowState timers → referee timers
4. Clean up wounding cancellation in batting_system.c
5. Document referee structure with section comments
6. Create REFEREE_PATTERN.md documentation
7. (Optional) Inline 2 trivial functions

**Timeline:** 1-2 sessions, ~3 hours total
**Blocking:** None - can proceed immediately

#### Phase 2C: Final Migration (Future - Lower Priority)
**After Phase 2B completion, consider:**
1. Move end-of-inning/next-pair **detection** to referee
   - Currently: game_analysis.c detects → sets timer
   - Proposed: referee.c detects → sets timer; game_analysis.c reads timer → executes state reset
2. Move camera counter logic to game_screen.c or camera module
3. Minimize game_analysis.c to pure flow/menu transitions

**Decision Point:** Do we need Phase 2C before Milestone 18?
- **Pro:** Complete logical separation before physics refactor
- **Con:** Adds delay, current state is functional
- **Recommendation:** DEFER - Current cleanup (Phase 2B) is sufficient for Milestone 18

---

## 🔮 Mid-Term Goals (Milestone 18-19)

### 🚧 Milestone 18: The Physics/State Split (Next Major Phase)

**Why This Matters:** Currently `game_manipulation.c` (~1500 lines) mixes:
- Pure physics integration (ball movement, collision)
- Game state mutations (baseId, hasBallIndex)
- Event emission (ballHitGround, catchMade)
- Geometry checks (out of bounds)

**Goal:** Separate physics engine from state management

**Benefits:**
1. **Testability:** Pure physics functions can be unit tested
2. **Replay/Determinism:** Physics separate from side effects
3. **Performance:** Can optimize physics without touching state
4. **Clarity:** Clear boundary between "what happened" and "what it means"

**High-Level Plan:**

1. **Create PhysicsEngine Module** (~2 sessions)
   - Input: `State + dt` (delta time)
   - Output: `NewPositions + PhysicsEvents`
   - Pure functions: `updateBallPhysics()`, `updatePlayerPhysics()`, `detectCollisions()`

2. **Separate Physical Safety from Legal Safety** (~2 sessions)
   - Physical: "Player is touching base geometry"
   - Legal: "Referee says player has safety"
   - Currently mixed in game_manipulation.c

3. **Extract Event Emission** (~1 session)
   - Create `PhysicsObserver` that watches physics and emits events
   - Replaces scattered event setting in game_manipulation.c

4. **Refactor game_manipulation.c** (~2 sessions)
   - Becomes: Physics → Observer → State Update → Referee
   - Much cleaner, ~500 lines instead of 1500

**Estimated Total:** 7-8 sessions, 12-15 hours

**Prerequisites:**
- ✅ Milestone 17 Phase 2B complete (referee clean)
- ✅ Pure physics tests in place (we have these!)
- ✅ Integration tests as safety net (we have 11)

**Success Criteria:**
- Physics engine is pure (no side effects)
- All physics covered by unit tests
- Integration tests still pass
- State updates are explicit, not scattered

---

### 🔮 Milestone 19: Action System Decoupling (Later)

**Goal:** Pure/Impure separation for action systems

**Current State:** `batting_system.c`, `pitching_system.c`, `throwing_system.c` mix:
- Input handling
- State queries  
- State mutations
- Physics triggers

**Target:** Pure action logic separated from state management

**Estimated:** 5-6 sessions after Milestone 18

---

### 🔮 Milestone 20: The "User Intent" Phase (Much Later)

**Goal:** Decouple Input from Action (`Input` → `Intent` → `Engine`)

**Benefits:** Replay, AI testing, networked multiplayer foundation

**Estimated:** 8-10 sessions after Milestone 19

---

## 📊 Timeline Summary

| Milestone | Status | Time Remaining | Blocking |
|-----------|--------|----------------|----------|
| M17 Phase 2B | 🔧 In Progress | 3 hours (1-2 sessions) | None |
| M17 Phase 2C | 💤 Deferred | 4 hours (optional) | None |
| **M18 Physics Split** | 📋 Planned | 12-15 hours (7-8 sessions) | M17 Phase 2B |
| M19 Action Decouple | 🔮 Future | ~20 hours | M18 |
| M20 User Intent | 🔮 Far Future | ~30 hours | M19 |

**Current Focus:** Complete M17 Phase 2B (next 1-2 sessions)
**Next Major Push:** M18 Physics/State Split (biggest refactor remaining)

---

---

## 🧹 Technical Debt & Cleanup Status

### ✅ Recently Cleaned (Jan 2026)
- ✅ Removed ~250 lines from game_analysis.c (wounding/foul play)
- ✅ Consolidated referee state (removed 5+ redundant flags)
- ✅ Established clean event flow pattern
- ✅ All 62 tests passing (51 unit + 11 integration)

### 🔧 Pending Cleanup (Phase 2B - Next Session)
- ⏳ 10 unused pure functions (~115 lines) - See `.dev/PURE_FUNCTIONS_AUDIT.md`
- ⏳ 2 redundant test files (~165 lines) - See `.dev/UNIT_TEST_ANALYSIS.md`
- ⏳ gameFlowState timer consolidation
- ⏳ Referee structure documentation

**Impact After Cleanup:**
- Code reduction: ~280 lines
- Test reduction: ~165 lines
- Maintenance: -20%
- Clarity: +40%

### 📋 Long-Term Technical Debt
- **Globals:** `src/include/globals.h` (700+ LOC) - Split into focused headers during M18
- **game_manipulation.c:** Mixed physics/state (~1500 lines) - Address in M18
- **Test Suite:** 
  - Unit tests focus on pure functions ✅ (Good!)
  - Integration tests provide safety net ✅ (Good!)
  - Some brittle mocking in action tests (Clean during M19)

### 🎯 Debt Priority
1. **HIGH:** Phase 2B cleanup (blocks nothing, improves everything)
2. **MEDIUM:** globals.h split (do during M18 naturally)
3. **LOW:** Brittle test cleanup (wait for M19 action refactor)

---

## 📚 Reference Documents

### Created This Session:
- `.dev/SESSION_SUMMARY_2026_01_15.md` - Today's refactoring achievements
- `.dev/UNIT_TEST_ANALYSIS.md` - Complete test suite audit
- `.dev/PURE_FUNCTIONS_AUDIT.md` - Function-by-function analysis
- `.dev/PLAN.md` - This file (updated)

### Key Documents:
- `.dev/ARCHITECT_AGENT.md` - High-level planning protocol
- `.dev/TASK_AGENT.md` - Implementation protocol
- `.dev/GENERAL_AGENT.md` - User interaction protocol

### To Be Created:
- `docs/REFEREE_PATTERN.md` - Referee Supremacy pattern guide (Phase 2B)
- `docs/PHYSICS_SPLIT.md` - M18 planning document (future)

---

## 🎯 Quick Start for Next Session

**Goal:** Complete Phase 2B Cleanup (3 hours)

**Step 1:** Review this PLAN.md section "🔧 Phase 2B: Cleanup Tasks"
**Step 2:** Start with Priority 1 (Remove dead code - easiest win)
**Step 3:** Work through priorities 2-7 in order
**Step 4:** Verify all tests pass after each priority
**Step 5:** Create incremental commits with clear messages

**Success:** All checkboxes ticked, referee.c is crystal clear, ready for M18!

**Resources:**
- Detailed cleanup steps: See "Priority 1-7" above
- Function deletion list: `.dev/PURE_FUNCTIONS_AUDIT.md`
- Test deletion list: `.dev/UNIT_TEST_ANALYSIS.md`
- Current status: All tests passing ✅

---

## Technical Debt / Cleanup
- **Globals:** `src/include/globals.h` (700+ LOC) - Split into focused headers.
- **Test Suite Modernization:** 
    - **Unit Tests:** Many are brittle because they mock complex "messy" state. *Action:* Delete/rewrite brittle tests during refactoring. Focus unit tests on pure logic (`rules_pure/`).
    - **Integration Tests:** Consolidate scenario tests to serve as the primary regression net.
