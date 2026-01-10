# Referee Consolidation Plan
**Date:** January 2026  
**Goal:** Decouple referee logic into a frame-independent monitoring system that is the sole authority on RefereeState and GameState.

---

## The Vision

The Referee should be an **independent, decoupled entity** that:
1. **Monitors** physical and logical reality
2. **Updates** RefereeState and GameState based on observations
3. **Never gets written to** by other systems (read-only from outside)
4. **Frame-independent** - doesn't assume timing, just reacts to state

Other systems:
- **Write to**: GameEvents (temporal event notifications)
- **Read from**: RefereeState, GameState (to react to legal decisions)
- **Never write to**: RefereeState, GameState

---

## Current Problems

### 1. Fragmented Mutation Responsibility
RefereeState and GameState are mutated in **multiple places**:
- `referee.c` (correct ✓)
- `action_implementation.c` (free walk safety, run scoring - 11 locations)
- `common_logic.c` (initialization, setup - 15 locations)
- `game_analysis.c` (wounding timer, events - 27 locations)

### 2. Frame Dependency
Wounding logic in `game_analysis.c` uses frame counters:
```c
referee.woundingCatchTimer++;
if (timer > 200) { /* wound players */ }
```
This assumes 20ms updates and is tightly coupled to frame rate.

### 3. Immediate Action Pattern
Free walk acceptance immediately:
- Grants safety
- Scores runs
- Updates referee state

All before referee sees the event.

### 4. Structure Confusion
Wild west of overlapping structures:
- `GameControlFlags` - mixes events and coordination
- `GameFlowState` - dumping ground for timers and flags
- `GameModeState` - unclear ownership (referee + others)

---

## Two-Phase Strategy

### Phase 1: Structural Reorganization ⭐ START HERE
**Goal:** Create clean, semantically clear structures

**Why First?** Makes Phase 2 easier by establishing clear event communication

#### 1.1 Split GameControlFlags
```c
// BEFORE (mixed events + coordination)
typedef struct _GameControlFlags {
    int pause;
    int waitingForBatterDecision;
    int playerArrivedToBase;      // Event!
    int firstCatchMade;           // Event!
    int batterStartedRunning;     // Event!
    int freeWalkIndex;
    int checkForRun;
} GameControlFlags;

// AFTER (separated)
typedef struct _GameEvents {
    // Transient: set this frame, cleared next frame
    int pitchStarted;
    int pitchReleased;
    int ballHitByBat;
    int ballMissedByBat;
    int catchMade;
    int playerArrivedAtBase;
    int freeWalkAccepted;
    int freeWalkRejected;
    int outOfBoundsOccurred;
    
    // Context (who/where)
    int eventPlayerIndex;
    BaseID eventBase;
} GameEvents;

typedef struct _GameControl {
    // Stateful: remains until explicitly changed
    int pause;
    int waitingForBatterDecision;
    int waitingForFreeWalkDecision;
    int freeWalkIndex;
    BaseID freeWalkBase;
    int checkForRun;
} GameControl;
```

**Clearing Strategy:**
```c
void clearFrameEvents(GameEvents* events) {
    // Called at start or end of updateMutableWorld()
    events->pitchStarted = 0;
    events->ballHitByBat = 0;
    // ... etc
}
```

#### 1.2 Eliminate GameFlowState
```c
// BEFORE (dumping ground)
typedef struct _GameFlowState {
    int outOfBoundsCounter;     // Frame timer
    int endOfInningCounter;     // Frame timer
    int nextPairCounter;        // Frame timer
    int homeRunCameraCounter;   // Frame timer
    int foulPlayEventFlag;      // Event flag
    int ballHome;               // Logical state
    int closeToGround;          // Logical state
} GameFlowState;

// AFTER (eliminated)
// - Frame counters → static variables in game_analysis.c
// - Event flags → GameEvents.outOfBoundsOccurred
// - Logical state → BallInfo or pure functions
```

**Why?** Frame counters are implementation details, not shared state.

#### 1.3 Clarify GameModeState
```c
// Keep as-is for now (revisit in Phase 2)
typedef struct _GameModeState {
    int runnerBatterPairCounter;  // Super inning pair tracking
    int forceNextPair;            // Skip to next pair
    int canMakeRunOfHonor;        // Referee decision (consider moving to RefereeState later)
} GameModeState;
```

### Phase 2: Referee Consolidation (AFTER Phase 1)
**Goal:** Only referee.c writes to RefereeState/GameState

#### 2.1 Simple Migrations (Low Risk)
**Target:** Event flag consumption, basic state updates

**Tasks:**
- Move strike reset to referee (detect `gameEvents.ballHitByBat`)
- Move `outOfBounds` flag management to referee
- Move `endPeriod` flag setting to referee
- Consolidate all `GameState.event` setting in referee

**Strategy:**
```c
void Referee_Update(..., GameEvents* events) {
    // Detect hit
    if (events->ballHitByBat) {
        gameState->strikes = 0;
    }
    
    // Detect out of bounds
    if (events->outOfBoundsOccurred) {
        gameState->outOfBounds = 1;
        // Restore players...
    }
    
    // Existing safety/out/run logic...
}
```

#### 2.2 Medium Difficulty: Free Walk Refactor
**Problem:** `action_implementation.c` immediately:
- Grants safety: `referee.battingPlayers[i].currentSafetyBase = base_get_next(base)`
- Scores runs: `gameState->runsInTheInning += 1`

**Solution:**
1. Action system sets event: `gameEvents.freeWalkAccepted = 1` with context
2. Referee detects and processes:
```c
if (events->freeWalkAccepted && events->eventPlayerIndex != -1) {
    int i = events->eventPlayerIndex;
    BaseID targetBase = base_get_next(events->eventBase);
    
    // Grant safety
    referee->battingPlayers[i].currentSafetyBase = targetBase;
    referee->battingPlayers[i].baseAtPitchStart = targetBase;
    
    // Score runs if applicable
    if (targetBase == BASE_HOME_SCORED) {
        // Run scoring logic...
    }
}
```

#### 2.3 Hard: Wounding State Machine Redesign
**Problem:** Frame-dependent timer in `game_analysis.c`

**Current:**
```c
// game_analysis.c
if (referee.woundingCatchPending) {
    referee.woundingCatchTimer++;  // Frame counter
    if (referee.woundingCatchTimer > 200) {
        // Mark players for wounds
    }
}
```

**Solution Options:**

**Option A: State-based (preferred)**
```c
typedef enum {
    WOUNDING_STAGE_NONE,
    WOUNDING_STAGE_CATCH_MADE,
    WOUNDING_STAGE_TIMER_ACTIVE,
    WOUNDING_STAGE_CONFIRMED
} WoundingStage;

// Referee tracks stages, not frame counts
// Timer becomes: "has enough time passed?" check
```

**Option B: Time-based**
- Track timestamp of catch instead of frame counter
- Check elapsed time instead of frame threshold
- Still frame-independent

**Note:** This is the hardest migration, save for last.

---

## Game Loop Structure

```c
void updateMutableWorld(StateInfo* state, MenuInfo* menu, unsigned int* rng) {
    if (!state->localGameInfo->gameControl.pause) {
        // 1. High-level monitoring (camera, flow)
        gameAnalysis(state, menu, rng);
        
        // 2. Convert input to action flags
        actionInvocations(state);
        
        // 3. Execute actions (pitch, bat, run)
        //    → Sets GameEvents (pitchStarted, ballHit, etc.)
        actionImplementation(state, rng);
        
        // 4. Physics updates (ball, catching)
        //    → Sets GameEvents (catchMade, playerArrived, etc.)
        gameManipulation(state);
        
        // 5. Referee monitors and updates legal state
        //    → Reads GameEvents
        //    → Writes RefereeState, GameState
        Referee_Update(state, &gameEvents);
        
        // 6. React to referee decisions
        //    → Reads RefereeState, GameState
        //    → Updates physical world (panic runs, outs, scores)
        reconcileLegalAndPhysicalState(state);
        
        // 7. Clear transient events for next frame
        clearFrameEvents(&state->localGameInfo->gameEvents);
        
        // 8. Validate consistency (debug)
        StateValidator_Check(state);
    }
}
```

---

## Migration Order (Phase 1)

### Step 1: Create New Structures
- Add `GameEvents` to globals.h
- Add `GameControl` to globals.h
- Keep `GameControlFlags` temporarily (deprecated)
- Add clearing function

### Step 2: Add to LocalGameInfo
```c
typedef struct _LocalGameInfo {
    // ... existing ...
    GameControlFlags gameControl;  // DEPRECATED
    GameEvents gameEvents;         // NEW
    GameControl gameControlNew;    // NEW (temporary name)
} LocalGameInfo;
```

### Step 3: Migrate Subsystem by Subsystem
**Order:**
1. game_manipulation.c (sets catchMade, playerArrived)
2. action_implementation.c (sets ballHit, freeWalkAccepted)
3. game_analysis.c (reads events, less writes)
4. referee.c (reads events)

**Strategy per subsystem:**
- Identify writes to GameControlFlags
- Determine if event (transient) or control (stateful)
- Write to new structure
- Update reads to use new structure
- Test

### Step 4: Remove GameFlowState
- Move frame counters to static in game_analysis.c
- Move `foulPlayEventFlag` to GameEvents.outOfBoundsOccurred
- Move `ballHome` to BallInfo or pure function
- Remove structure definition
- Clean up references

### Step 5: Remove GameControlFlags
- Once all references migrated
- Remove deprecated structure
- Rename `gameControlNew` → `gameControl`

---

## Migration Order (Phase 2)

### Step 1: Strike Reset
- Referee detects `gameEvents.ballHitByBat`
- Sets `gameState->strikes = 0`
- Remove from game_analysis.c

### Step 2: Event Consolidation
- All `gameState->event = EVENT_*` moves to referee
- Other systems set GameEvents, referee sets GameState.event

### Step 3: Free Walk Safety
- Move safety grants from action_implementation.c to referee
- Use GameEvents.freeWalkAccepted

### Step 4: Free Walk Scoring
- Move run scoring from action_implementation.c to referee
- Part of existing update_runs() logic

### Step 5: Wounding Redesign
- Design state machine (or time-based approach)
- Move all wounding logic from game_analysis.c to referee
- This is the big one

---

## Success Criteria

### Phase 1 Complete When:
- ✅ GameEvents structure exists and is used
- ✅ GameControl structure exists and is used
- ✅ GameControlFlags removed
- ✅ GameFlowState removed
- ✅ All tests pass
- ✅ Build succeeds with no warnings

### Phase 2 Complete When:
- ✅ Only referee.c writes to RefereeState
- ✅ Only referee.c writes to GameState
- ✅ Referee reads GameEvents (never writes)
- ✅ Other systems write GameEvents (never write Referee/GameState)
- ✅ All tests pass
- ✅ Integration tests for wounding, free walk, outs pass

---

## Dependencies to Watch

### Systems that read RefereeState:
- game_analysis.c (wounding state, baseAtPitchStart)
- common_logic.c (baseAtPitchStart, currentSafetyBase)
- mutable_world.c (reconciliation logic)

### Systems that read GameState:
- UI/rendering (display outs, strikes, balls)
- game_analysis.c (flow control based on outs)
- action_implementation.c (free walk run calculations)

### Systems that will write GameEvents:
- action_implementation.c (pitch, bat, free walk decisions)
- game_manipulation.c (catch, arrival, out of bounds)
- actions_messy/* (pitching_system, batting_system, throwing_system)

---

## Testing Strategy

### Unit Tests
- Test event clearing mechanism
- Test referee event consumption
- Test that referee doesn't mutate GameEvents

### Integration Tests
- Free walk scoring (before/after migration)
- Wounding scenarios (tuplahaava, normal)
- Out of bounds restoration
- Strike reset after hit

### Regression Protection
- Capture baseline behavior before Phase 1
- Verify identical behavior after each migration step
- Integration tests are critical here

---

## Notes and Considerations

### Frame Independence
The goal is to eliminate frame counting where possible:
- Replace `counter++; if (counter > 200)` with state-based logic
- If timing is essential, use time-since instead of frame count
- Document any remaining frame dependencies

### GameModeState Future
After Phase 2, revisit:
- Should `canMakeRunOfHonor` move to RefereeState?
- Is `runnerBatterPairCounter` mode-specific enough to justify the structure?
- Could this merge into GameControl or be eliminated?

### Common Logic Exception
`common_logic.c` contains **initialization** code that sets up referee state at inning/period boundaries. This is acceptable - it's not mutation during gameplay, it's setup.

### Reconciliation Pattern
The `reconcileLegalAndPhysicalState()` function is the bridge:
- Referee says "player is out"
- Reconciliation moves player sprite off field
- This pattern is good and should be preserved

---

## Northern Star Principles

1. **Referee is the authority** on legal game state
2. **Events flow one way**: Action → GameEvents → Referee → RefereeState/GameState
3. **Frame independence** is a goal, not an absolute requirement (pragmatism over purity)
4. **Preserve working behavior** - don't break things in pursuit of perfect architecture
5. **Test everything** - integration tests are our safety net

---

## Next Steps (Immediate)

1. ✅ Document plan (this file)
2. ⬜ Update .dev/PLAN.md with Phase 1 milestone
3. ⬜ Create GameEvents and GameControl structures in globals.h
4. ⬜ Add clearing function in mutable_world.c
5. ⬜ Pick first subsystem to migrate (suggest: game_manipulation.c)
6. ⬜ Write test to verify event clearing works
7. ⬜ Begin migration

---

**Last Updated:** January 10, 2026  
**Status:** Planning Complete - Ready for Implementation
