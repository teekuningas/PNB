# Game Loop and Structure Reference

**Quick reference for understanding PNB's game loop and data structures during refactoring.**

---

## Game Loop (mutable_world.c)

```c
void updateMutableWorld(StateInfo* stateInfo, MenuInfo* menuInfo, unsigned int* rng_seed)
{
    if (gameControl.pause == 0) {
        gameAnalysis()       // High-level monitoring, camera, flow
        actionInvocations()  // Input → action flags
        actionImplementation()  // Execute actions (pitch, bat, run)
        gameManipulation()   // Physics (ball, catching)
        Referee_Update()     // Legal state tracking
        reconcileLegalAndPhysicalState()  // React to referee decisions
        StateValidator_Check()  // Debug validation
    }
}
```

---

## Key Structures (Before Refactor)

### Legal Authority (referee.c writes only)
```c
RefereeState {
    RefereePlayerState battingPlayers[15];  // Per-player legal state
    int woundingCatchPending/Handled/Timer;
    int foulPlayActive;
    int strikesAtPitchStart;
}

GameState {
    int outs, balls, strikes;
    int runsInTheInning;
    GameEventType event;
    int outOfBounds;
    int endPeriod;
}
```

### Coordination (messy - being refactored)
```c
GameControlFlags {  // ← BEING SPLIT
    int pause;
    int waitingForBatterDecision;
    int playerArrivedToBase;     // Event!
    int firstCatchMade;          // Event!
    int freeWalkIndex;
    // ...
}

GameFlowState {  // ← BEING ELIMINATED
    int outOfBoundsCounter;      // Frame timer
    int endOfInningCounter;      // Frame timer
    int foulPlayEventFlag;       // Event!
    int ballHome;
    // ...
}

GameModeState {  // ← Keeping for now
    int runnerBatterPairCounter;
    int canMakeRunOfHonor;       // Referee decision
    int forceNextPair;
}
```

---

## Key Structures (After Phase 1 Refactor)

### Event Communication (NEW)
```c
GameEvents {
    // Transient: set this frame, cleared next frame
    int pitchStarted;
    int pitchReleased;
    int ballHitByBat;
    int catchMade;
    int playerArrivedAtBase;
    int freeWalkAccepted;
    int outOfBoundsOccurred;
    
    // Context
    int eventPlayerIndex;
    BaseID eventBase;
}
```

### Control Flow (NEW)
```c
GameControl {
    // Stateful: remains until explicitly changed
    int pause;
    int waitingForBatterDecision;
    int waitingForFreeWalkDecision;
    int freeWalkIndex;
    BaseID freeWalkBase;
    int checkForRun;
}
```

---

## Ownership Rules

### Phase 1 (Current)
| Structure | Writers | Readers |
|-----------|---------|---------|
| RefereeState | referee.c, game_analysis.c, action_implementation.c | Everyone |
| GameState | referee.c, game_analysis.c, action_implementation.c | Everyone |
| GameEvents | action_impl.c, game_manip.c | referee.c, others |
| GameControl | Various | Various |

### Phase 2 (Goal)
| Structure | Writers | Readers |
|-----------|---------|---------|
| RefereeState | **referee.c ONLY** | Everyone |
| GameState | **referee.c ONLY** | Everyone |
| GameEvents | action_impl.c, game_manip.c | **referee.c reads, never writes** |
| GameControl | Various | Various |

---

## Current Mutation Hotspots (Pre-Phase 2)

### RefereeState mutations OUTSIDE referee.c:
- **action_implementation.c**: Free walk safety grants (lines 227-283)
- **common_logic.c**: Initialization, setup (lines 493, 785, 833-834, 914, 928-930)
- **game_analysis.c**: Wounding timer, wound marking (lines 177-261)

### GameState mutations OUTSIDE referee.c:
- **action_implementation.c**: Run scoring, events (lines 233-241)
- **common_logic.c**: Initialization (lines 493-500)
- **game_analysis.c**: Strike resets, events, out of bounds (lines 158-334)

---

## Common Patterns

### Event Pattern (NEW in Phase 1)
```c
// Action system detects something happened
gameEvents.ballHitByBat = 1;
gameEvents.eventPlayerIndex = batterIndex;

// Referee reacts
if (gameEvents.ballHitByBat) {
    gameState->strikes = 0;
}

// Game loop clears
clearFrameEvents(&gameEvents);
```

### Reconciliation Pattern (Existing)
```c
// Referee makes decision
referee.battingPlayers[i].isOut = 1;

// Reconciliation reacts
if (referee.battingPlayers[i].isOut) {
    game->playerInfo[i].bTPI.state = PLAYER_STATE_OUT;
    movePlayerOut(game, i);
}
```

---

## File Responsibilities

### Core Loop
- `mutable_world.c` - Game loop coordinator

### Systems
- `game_analysis.c` - High-level monitoring, camera, flow control
- `action_invocations.c` - Input → action flags
- `action_implementation.c` - Execute actions
- `game_manipulation.c` - Physics, ball, catching
- `referee.c` - Legal state authority

### Helpers
- `common_logic.c` - Initialization, setup, utility
- `base_control.c` - Base ownership queries
- `state_validator.c` - Debug validation

---

## Northern Star Principles

1. **Referee is the authority** on legal game state
2. **Events flow one way**: Action → GameEvents → Referee → State
3. **Frame independence** is the goal
4. **Preserve working behavior** during migration
5. **Test everything**

---

**See Also:**
- `docs/REFEREE_CONSOLIDATION_PLAN.md` - Complete refactoring strategy
- `docs/ARCHITECTURE.md` - Technical architecture details
- `.dev/PLAN.md` - Development roadmap

**Last Updated:** January 10, 2026
