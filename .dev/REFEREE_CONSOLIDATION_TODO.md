# Referee Pattern Consolidation Plan
**Date:** 2026-01-18 (Updated: 2026-01-19)
**Goal:** **Referee_Update is the SOLE writer** - Zero exceptions!

## ✅ COMPLETED: Status Enum Migration (2026-01-19)

**Achievement:** Successfully migrated from flag-based status tracking to enum-based state machine!

**What was removed:**
- `int isOut` → replaced by `status == PLAYER_STATUS_OUT`
- `PendingWoundState pendingWoundState` → replaced by status enum
- `WoundingType woundingType` → replaced by status enum
- `BaseID woundingSourceBase` → use baseAtPitchStart instead
- `int woundingPlayersMarked[]` → replaced by status checks

**What was added:**
- `RefereePlayerStatus` enum with 7 states (ACTIVE, WOUND_MARKED, WOUND_MARKED_DOUBLE, WOUND_PENDING, WOUND_PENDING_DOUBLE, WOUNDED, OUT)
- Single `status` field in RefereePlayerState
- Cleaner, more explicit state transitions

**Results:**
- ✅ All 48 unit tests passing
- ✅ All 15 integration tests passing
- ✅ Net -50 lines of code (simpler!)
- ✅ Simplified game_manipulation.c (removed wound type branching)
- ✅ Single source of truth for player status

**Key files changed:**
- src/include/globals.h (new enum, removed old fields)
- src/game/referee.c (~35 read/write sites updated)
- src/game/game_manipulation.c (major simplification!)
- src/game/mutable_world.c (status checks)
- src/game/game_setup.c (initialization)
- tests/ (5 files - only debugging output, no assertions changed)

**Documentation:** REFEREE_STATUS_ENUM_PLAN.md completed and removed.

---

## 🎯 NEXT UP: Remaining Consolidation Tasks

### Priority 1: Out of Bounds Logic (Still Outside Referee)

**Problem:** Out of bounds detection and reset logic is scattered:
- Detection: Possibly in ball_physics.c or game_manipulation.c
- Timer: applyFoulPlayReset() in mutable_world.c
- State writes: game_setup.c writes to referee state during reset

**Should be:** 
- Referee detects out of bounds via events
- Referee owns the `betweenPitchState.outOfBounds` flag
- Foul reset uses event-driven pattern (see Phase 1.3 below)

### Priority 2: Wounding Execution Completion

**File:** game_manipulation.c line 339 (now removed during status migration)

**Status:** ✅ May already be fixed! Need to verify game_manipulation.c no longer writes to referee state for wounding completion.

### Priority 3: Foul Reset Event-Driven Pattern

**See Phase 1.3 below** - Use `foulResetCompleted` event instead of direct writes.

### Priority 4: Main Loop Reordering

**Issue:** gameAnalysis runs BEFORE Referee_Update, sees stale balls count (1 frame delay on free walks).

**Fix:** Move gameAnalysis after Referee_Update (see Session 2 below).

---

---

## 🎯 Vision: Referee Supremacy (No Exceptions!)

**Rule:** ONLY `Referee_Update` writes to RefereeState and BetweenPitchState. Period.

**No "initialization exceptions"** - Use events instead:
- `batterEntered` → Referee sets baseAtPitchStart
- `pitchStarted` → Referee snapshots positions
- `foulResetCompleted` → Referee restores state
- `gameStarted` → Referee initializes

**Result:** Clean, testable, no special cases.

---

## 🔍 ALL Violations Found

### Writes to RefereeState Outside referee.c

**common_logic.c:**
- Line 463: `currentSafetyBase = BASE_HOME` (batter initialization)
- Line 756: `baseAtPitchStart = BASE_NONE` (initialization loop)
- Line 909: `baseAtPitchStart = BASE_HOME` (homerun contest setup)
- Lines 920-921: Setting baseAtPitchStart, currentSafetyBase (homerun runner setup)

**game_manipulation.c:**
- Line 339: `hasPendingWound = 0` (wounding execution)

**game_setup.c:**
- Lines 104, 107-108, 130-131: Foul reset state restoration

**Status:** ⚠️ ALL must be replaced with events!

---

## 🎨 Revised Main Loop Order

**Current (messy):**
```
gameAnalysis → actions → physics → referee → reconciliation
```

**Your Vision (clean):**
```
1. Inputs (actionInvocations)
2. Physics / Experience (actionImplementation, gameManipulation)
3. Referee (Referee_Update - reads events, writes legal state)
4. Consolidation (merge game_analysis + reconciliation)
5. Frame resets (clearFrameEvents)
```

**Key insight:** game_analysis IS reconciliation! Both react to referee's decisions.

---

## 🔧 Consolidation Plan

### Phase 1: Replace Init Writes with Events (2 hours)

**1.1: Batter Entered Event (30 min) - ✅ DONE**

**Add to GameEvents:**
```c
typedef struct _GameEvents {
    ...
    int batterEntered;
    // batterEnteredIndex not needed, referee finds AT_BAT player
} GameEvents;
```

**Emit in common_logic.c:463:**
```c
// NEW:
match->gameEvents.batterEntered = 1;
```

**Handle in referee.c:**
```c
if (events->batterEntered) {
    // Find AT_BAT player and set safety
}
```

**1.2: Pitch Started Event (30 min) - ✅ DONE**

**Status:** Used existing `pitchReleased` event instead of creating `pitchStarted`.

**Emit in pitching_system.c (releasePitch):**
```c
// Existing event emission:
stateInfo->match->gameEvents.pitchReleased = 1;
// Removed direct writes to referee state
```

**Handle in referee.c:**
```c
if (events->pitchReleased) {
    // Snapshot all players' positions
    // Reset between-pitch state
    // Reset wounding evaluation
}
```

**1.3: Foul Reset Event (30 min)**

**Add to GameEvents:**
```c
int foulResetCompleted;
```

**Emit in game_setup.c (applyFoulPlayReset):**
```c
// After physical teleports:
stateInfo->match->gameEvents.foulResetCompleted = 1;

// DELETE all referee state writes (lines 104, 107-108, 130-131)
```

**Handle in referee.c:**
```c
if (events->foulResetCompleted) {
    // Restore state from snapshot
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        BaseID restore = referee->battingPlayers[i].baseAtPitchStart;
        if (restore != BASE_NONE) {
            referee->battingPlayers[i].currentSafetyBase = restore;
        }
        referee->battingPlayers[i].hasPendingWound = 0;
        referee->battingPlayers[i].woundingType = WOUNDING_TYPE_NONE;
    }
    betweenPitchState->outOfBounds = 0;
}
```

**1.4: Wounding Execution (30 min)**

**Handle in referee.c:**
```c
// Detect when wounded player reached destination
for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
    if (referee->battingPlayers[i].hasPendingWound) {
        BaseID dest = referee->battingPlayers[i].woundingSourceBase;
        if (game->playerInfo[i].bTPI.currentBase == dest &&
            game->playerInfo[i].bTPI.state == PLAYER_STATE_ON_BASE) {
            // Complete
            referee->battingPlayers[i].hasPendingWound = 0;
            referee->battingPlayers[i].woundingType = WOUNDING_TYPE_NONE;
            referee->battingPlayers[i].woundingSourceBase = BASE_NONE;
        }
    }
}
```

**Delete from game_manipulation.c:339**

### Phase 2: Merge game_analysis into Consolidation (1 hour)

**Goal:** Rename and merge reconciliation functions.

**Current files:**
- `game_analysis.c` - Flow decisions, free walk calc, timers
- `mutable_world.c:reconcileLegalAndPhysicalState()` - React to referee

**New structure:**
- `game_consolidation.c` - All post-referee reactions

**Functions to merge:**
```c
// From game_analysis.c:
- checkIfNextBatterDecision()
- strikesAndBalls() (free walk calculation)
- checkIfEndOfInning()
- checkIfNextPair()

// From mutable_world.c:
- reconcileLegalAndPhysicalState()
- applyFoulPlayReset() (timer logic)

// New unified function:
void gameConsolidation(StateInfo*, MenuInfo*, unsigned int* rng_seed) {
    // 1. React to referee decisions (old reconciliation)
    // 2. Calculate flow decisions (old game_analysis)
    // 3. Manage timers
}
```

**New main loop:**
```c
void updateMutableWorld() {
    // 1. Inputs
    actionInvocations();
    
    // 2. Physics / Experience
    actionImplementation();
    gameManipulation();  // Emits events
    
    // 3. Referee (SOLE WRITER)
    Referee_Update();  // Reads events, writes legal state
    
    // 4. Consolidation (reacts to referee)
    gameConsolidation();  // Flow, timers, reactions
    
    // 5. Frame reset
    clearFrameEvents();
}
```

### Phase 3: Clean Up & Test (1 hour)

**3.1: Remove ALL old initialization code (15 min)**
- Delete direct writes from common_logic.c
- Delete direct writes from game_setup.c
- Delete direct writes from game_manipulation.c

**3.2: Test event-driven init (30 min)**
- Test new batter entering
- Test pitch start
- Test foul reset
- Test wounding

**3.3: Verify all tests pass (15 min)**
- Run 61 tests
- Debug any failures

### Phase 4: Documentation (30 min)

**4.1: Update ARCHITECTURE.md**
- Document event-driven initialization pattern
- Show new main loop order
- Explain consolidation phase

**4.2: Add section markers to referee.c**
```c
// ============================================================================
// EVENT HANDLING & INITIALIZATION
// ============================================================================
// - batterEntered, pitchStarted, foulResetCompleted

// ============================================================================
// FOUL PLAY & WOUNDING LOGIC  
// ============================================================================

// ... etc
```

**4.3: Create game_consolidation.c header**
- Document what consolidation does
- Clear separation from referee

---

## ✅ Success Criteria

**Zero exceptions:**
- ✅ ONLY Referee_Update writes to RefereeState
- ✅ ONLY Referee_Update writes to BetweenPitchState (except reconciliation consuming flags)
- ✅ All initialization via events

**Clean architecture:**
- ✅ Main loop order: Inputs → Physics → Referee → Consolidation → Reset
- ✅ game_analysis merged into gameConsolidation
- ✅ Clear phase separation

**All tests passing:**
- ✅ 61 tests (48 unit + 13 integration)
- ✅ No regressions

**Documentation current:**
- ✅ Event-driven patterns documented
- ✅ Main loop order clear
- ✅ Consolidation phase explained

---

## 💡 Key Insights

1. **No initialization exceptions** - Events are cleaner and testable
2. **game_analysis IS consolidation** - They're the same phase
3. **Referee is pure** - Events in, legal state out
4. **Main loop has 5 clear phases** - Easy to understand
5. **Ready for M18** - Clean foundation for physics extraction

---

## 📝 Timeline

- **Event-driven init:** 2 hours
- **Merge consolidation:** 1 hour
- **Testing:** 1 hour
- **Documentation:** 30 min

**Total: ~4.5 hours**

**Result: THE PERFECT PLATEAU** 🏔️

No compromises. No exceptions. Pure referee supremacy!

---

## 📋 Current Main Loop Order

```c
void updateMutableWorld() {
    1. gameAnalysis()           // Flow decisions, free walk calculation
    2. actionInvocations()      // Input → Intent
    3. actionImplementation()   // Execute actions
    4. gameManipulation()       // Physics + Events
    5. Referee_Update()         // Events → Legal State
    6. applyFoulPlayReset()     // Timer + teleport (if outOfBounds)
    7. reconcileLegalAndPhysicalState()  // React to referee decisions
    8. clearFrameEvents()       // Cleanup
}
```

**Flow:** Input → Actions → Physics → Events → Referee → Reconciliation

---

## 📊 What's in game_analysis.c? (370 LOC)

**Current Functions:**
1. `checkIfNextBatterDecision()` - Detect when to prompt for next batter
2. `strikesAndBalls()` - **FREE WALK CALCULATION** using balls count
3. `checkIfEndOfInning()` - Detect inning end, start timer
4. `checkIfNextPair()` - Homerun contest pair completion

**Key Finding:** 
- `strikesAndBalls()` checks `balls >= 3` + runner situation
- Sets `flowControl.freeWalkIndex` and `flowControl.freeWalkBase`
- This is **QUERY** logic ("who is eligible?"), not DECISION logic ("grant free walk")
- Referee makes the actual decision when `freeWalkAccepted` event happens

**Verdict:** game_analysis.c is mostly **FLOW CONTROL**:
- Detects when UI prompts needed
- Calculates free walk offers (query, not decision)
- Manages transition timers

---

## ⚠️ Problems Identified

### Problem 1: Wounding Execution Violation ⚠️
**File:** game_manipulation.c:339

**Current:**
```c
// After physical teleport of wounded player:
stateInfo->match->referee.battingPlayers[index].hasPendingWound = 0;
```

**Should be:** Referee detects player reached wounding destination and clears flag internally.

**Fix:**
```c
// In referee.c, add to Referee_Update:
for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
    if (referee->battingPlayers[i].hasPendingWound) {
        BaseID dest = referee->battingPlayers[i].woundingSourceBase;
        if (game->playerInfo[i].bTPI.currentBase == dest &&
            game->playerInfo[i].bTPI.state == PLAYER_STATE_ON_BASE) {
            // Wounding execution complete
            referee->battingPlayers[i].hasPendingWound = 0;
            referee->battingPlayers[i].woundingType = WOUNDING_TYPE_NONE;
            referee->battingPlayers[i].woundingSourceBase = BASE_NONE;
        }
    }
}
```

### Problem 2: Foul Reset Writes to Referee ⚠️
**File:** game_setup.c:104-131

**Current Pattern:**
1. Referee decides `betweenPitchState.outOfBounds = 1`
2. mutable_world.c waits 2 seconds (timer)
3. applyFoulPlayReset() teleports players physically
4. applyFoulPlayReset() **writes to referee state** (restores baseAtPitchStart → currentSafetyBase)

**Problem:** Step 4 violates ownership - only referee should write referee state.

**Solution: Event-Driven Approach**
```c
// Add new event:
typedef struct _GameEvents {
    ...
    int foulResetCompleted;  // Physical teleport finished
} GameEvents;

// In applyFoulPlayReset() - after teleporting:
stateInfo->match->gameEvents.foulResetCompleted = 1;

// In referee.c - respond to event:
if (events->foulResetCompleted) {
    // Restore referee's own state
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        BaseID restore = referee->battingPlayers[i].baseAtPitchStart;
        if (restore != BASE_NONE) {
            referee->battingPlayers[i].currentSafetyBase = restore;
        }
        referee->battingPlayers[i].hasPendingWound = 0;
        referee->battingPlayers[i].woundingType = WOUNDING_TYPE_NONE;
    }
    betweenPitchState->outOfBounds = 0;
}
```

### Problem 3: Main Loop Order ⚠️
**Current:** gameAnalysis runs BEFORE Referee_Update

**Issue:**
- gameAnalysis calculates free walks using `halfInningState.balls`
- Referee_Update increments `halfInningState.balls`
- **Bug:** Free walk calculation uses STALE balls count from previous frame!

**Example:**
- Frame N: balls=2, gameAnalysis sees balls=2 (no free walk)
- Frame N: Referee increments to balls=3
- Frame N+1: gameAnalysis finally sees balls=3 (1 frame late!)

**Solution:** Move gameAnalysis AFTER Referee_Update

**New Order:**
```c
1. actionInvocations      // Input → Intent
2. actionImplementation   // Execute actions  
3. gameManipulation       // Physics → Events
4. Referee_Update         // Events → Legal State (updates balls count)
5. gameAnalysis          // Flow (sees CURRENT balls count)
6. reconciliation        // React to legal state
7. clearFrameEvents      // Cleanup
```

---

## 🎯 Tomorrow's Consolidation Plan

### Session 1: Fix Critical Violations (2 hours)

**Task 1.1: Wounding Execution (30 min)**
- Add wounding completion detection to referee.c
- Remove line from game_manipulation.c:339
- Test wounding scenarios

**Task 1.2: Foul Reset Event-Driven (1 hour)**
- Add `foulResetCompleted` event to GameEvents
- Emit event in applyFoulPlayReset() after teleport
- Handle event in referee.c (restore state)
- Remove writes from game_setup.c:104-131
- Test out of bounds scenarios

**Task 1.3: Verify Tests (30 min)**
- Run all 61 tests
- Debug any failures
- Ensure no regressions

### Session 2: Main Loop Reordering (1 hour)

**Task 2.1: Move gameAnalysis After Referee (30 min)**
- Change order in mutable_world.c:
  ```c
  actionInvocations();
  actionImplementation();
  gameManipulation();
  Referee_Update();
  gameAnalysis();  // <-- MOVED HERE
  reconciliation();
  clearFrameEvents();
  ```
- Test that free walk still works correctly

**Task 2.2: Verify No Issues (30 min)**
- Run all tests
- Play test game manually
- Check free walk offers appear at right time

### Session 3: Documentation (30 min)

**Task 3.1: Mark Initialization Functions**
- Add `// REFEREE INIT` comments to:
  - common_logic.c:463, 756, 909, 920-921
  - Any other init functions that write referee state

**Task 3.2: Add Section Markers to referee.c**
```c
// ============================================================================
// Foul Play & Wounding Logic
// ============================================================================

// ============================================================================
// Safety Management
// ============================================================================

// ============================================================================
// Force Outs & Tuplahaava
// ============================================================================

// ============================================================================
// Run Scoring & Pending Runs
// ============================================================================

// ============================================================================
// Strike/Ball Adjudication
// ============================================================================

// ============================================================================
// Main Update & Initialization
// ============================================================================
```

**Task 3.3: Update ARCHITECTURE.md**
- Document the event-driven foul reset pattern
- Document initialization exceptions
- Update main loop order diagram

---

## ✅ Success Criteria

After tomorrow's session:

1. **Zero Violations:**
   - ✅ Only referee.c writes to RefereeState (except marked init functions)
   - ✅ Only referee.c writes to BetweenPitchState (except reconciliation consuming flags)

2. **Correct Main Loop Order:**
   - ✅ gameAnalysis runs after Referee_Update (sees current state)
   - ✅ No stale-data bugs

3. **Event-Driven Patterns:**
   - ✅ Foul reset uses events, not direct writes
   - ✅ Wounding execution detected by referee

4. **All Tests Passing:**
   - ✅ 61 tests (48 unit + 13 integration)
   - ✅ No regressions

5. **Documentation Current:**
   - ✅ Initialization functions marked
   - ✅ Section markers in referee.c
   - ✅ ARCHITECTURE.md updated

**Then: THE PLATEAU** 🏔️
- Clean referee pattern established
- Zero technical debt
- Ready for M18 physics extraction

---

## 💡 Key Insights

1. **You're absolutely right about PhysicsObserver:**
   - NOT needed! Physics setting GameEvents is perfect.
   - They're the outputs of physics. Keep it simple.

2. **Referee pattern is 95% there:**
   - Just 2 violations remain (wounding, foul reset)
   - Both are fixable with event-driven approach

3. **game_analysis.c is actually OK:**
   - It's flow/query logic, not rules
   - Moving it after referee fixes the stale-data bug
   - No need to move free walk calculation to referee

4. **Main loop order really matters:**
   - Subtle bug: free walk uses old balls count
   - Easy fix: reorder gameAnalysis

5. **Initialization exceptions are fine:**
   - Just need clear `// REFEREE INIT` comments
   - Document in ARCHITECTURE.md

**Bottom line:** ~3.5 hours of work tomorrow gets us to the plateau. Then M18 with confidence!

---

## 📝 Estimated Timeline

- **Wounding fix:** 30 min
- **Foul reset event:** 1 hour  
- **Testing violations:** 30 min
- **Loop reordering:** 30 min
- **Testing reordering:** 30 min
- **Documentation:** 30 min

**Total: ~3.5 hours**

**After this:** Ready for Milestone 18 with zero regrets!
