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
5. **Limited Scope** - Referee MUST NOT mutate other structures (e.g., `PlayerInfo`, `BallInfo`, `pRAI`, `AIState`). Its output is strictly the Legal State (`RefereeState`, `GameState`, `GameControl`). Physical enforcement is the job of `reconcile`.

Other systems:
- **Write to**: GameEvents (temporal event notifications)
- **Read from**: RefereeState, GameState (to react to legal decisions)
- **Never write to**: RefereeState, GameState

---

## Current Status (January 11, 2026)

✅ **Phase 1 Complete:** Structural Reorganization
- GameEvents and GameControl structures established.
- GameControlFlags deprecated and removed.
- Event system working (transient events cleared per frame).

✅ **Phase 2 In Progress:** Referee Consolidation
- **Strike/Ball Counting:** Migrated from `game_manipulation.c` to `referee.c` (Milestone 17).
- **Pitch Resolution:** Referee adjudicates Strike/Ball based on `ballHitGround` event. `reconcile` cleans up `pitchState` via `pitchResolutionProcessed` flag.
- **Free Walk:** Migrated safety grants and run scoring to `referee.c` (using `gameEvents.freeWalkAccepted`).
- **Next:** Wounding logic redesign.

---

## Architecture: The Reconcile Bridge

We have established a robust pattern for synchronizing the Legal State (Referee) with the Physical/Action State (Physics/Action System):

1.  **Physics/Action:** Detects physical events (e.g., "Ball Hit Ground", "Batter Arrived"). Sets **GameEvents** (transient).
2.  **Referee (`Referee_Update`):** Reads **GameEvents** + Current State. Makes legal decisions (Out, Run, Strike, Ball). Updates **GameState** and **RefereeState**. Sets **GameControl** flags if cleanup is needed (`pitchResolutionProcessed`).
3.  **Reconcile (`reconcileLegalAndPhysicalState`):** Reads **RefereeState** and **GameControl**. Updates Physical State to match Legal State (moves player if Out, resets `pitchState` if resolution processed).

This ensures:
- Referee is the **authority** on rules.
- Physics doesn't need to know rules.
- State cleanup happens safely after adjudication.

---

## Two-Phase Strategy

### Phase 1: Structural Reorganization ✅ COMPLETE
**Goal:** Create clean, semantically clear structures

*(Archived Phase 1 details removed for brevity - see git history)*

### Phase 2: Referee Consolidation (In Progress)
**Goal:** Only referee.c writes to RefereeState/GameState

#### 2.1 Simple Migrations (Low Risk)
**Target:** Event flag consumption, basic state updates

**Tasks:**
- ✅ Move strike/ball counting from `game_manipulation.c` to referee.
- ✅ Move pitch resolution logic to referee (detect `ballHitGround` + `pitchState`).
- Move strike reset to referee (detect `gameEvents.ballHitByBat`).
- Move `outOfBounds` flag management to referee.
- Move `endPeriod` flag setting to referee.
- Consolidate all `GameState.event` setting in referee.

#### 2.2 Medium Difficulty: Free Walk Refactor ✅ DONE
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

---

## Game Loop Structure (Current)

```c
void updateMutableWorld(StateInfo* state, MenuInfo* menu, unsigned int* rng) {
    if (!state->localGameInfo->gameControl.pause) {
        // 1. High-level monitoring (camera, flow)
        gameAnalysis(state, menu, rng);
        
        // 2. Convert input to action flags
        actionInvocations(state);
        
        // 3. Execute actions (pitch, bat, run)
        actionImplementation(state, rng);
        
        // 4. Physics updates (ball, catching)
        //    → Sets GameEvents (ballHitGround, catchMade, etc.)
        gameManipulation(state);
        
        // 5. Referee monitors and updates legal state
        //    → Reads GameEvents
        //    → Writes RefereeState, GameState, GameControl flags
        Referee_Update(state, &gameEvents, ...);
        
        // 6. React to referee decisions (The Bridge)
        //    → Reads RefereeState, GameControl
        //    → Updates physical world (pitchState reset, panic runs, outs)
        reconcileLegalAndPhysicalState(state);
        
        // 7. Clear transient events for next frame
        clearFrameEvents(&state->localGameInfo->gameEvents);
        
        // 8. Validate consistency (debug)
        StateValidator_Check(state);
    }
}
```

---

## Success Criteria

### Phase 2 Complete When:
- ✅ Only referee.c writes to RefereeState
- ✅ Only referee.c writes to GameState
- ✅ Referee reads GameEvents (never writes)
- ✅ Other systems write GameEvents (never write Referee/GameState)
- ✅ All tests pass
- ✅ Integration tests for wounding, free walk, outs pass

---

**Last Updated:** January 11, 2026  
**Status:** Phase 2 Active - Strike/Ball Counting Logic Consolidated