# PNB Architecture

**Last updated:** 2026-01-18  
**Current Status:** Milestone 17 Phase 2B Complete ✅

## Vision: The Functional Pipeline

Transform from coordinated managers to a strict functional pipeline:

```
State_Next = Pipeline(Physics(Input(State)))
```

**Target loop:**
1. Input → Intent (e.g., INTENT_SWING_BAT)
2. Physics(State, Intent) → NewPhysicalState
3. Referee_Update(NewPhysicalState) → NewLegalState (Outs, Runs)
4. Reconcile(NewLegalState) → PhysicalReactions (Panic Runs)
5. Render(NewState)

## Milestone Progress

| # | Milestone | Status | Achievement |
|---|-----------|--------|-------------|
| 10-13 | State Consolidation | ✅ | StateInfo is single source of truth |
| 14 | The Great Decoupling V1 | ✅ | **Referee_Analyze (pure) + Referee_Apply (impure)** |
| 15 | Referee Architecture V2 | ✅ | **Referee_Update (Sequential Pipeline)** - Decisions struct eliminated |
| 16 | Event System | ✅ | GameEvents (transient) established |
| **17** | **Referee Consolidation** | **✅** | **GameControl split, wounding/foul refactored** |
| 18 | Physics/State Split | 🎯 | **Next:** Deconstruct game_manipulation into Physics + Rules |
| 19 | Action Decoupling | 🔮 | Split actions_messy/ into pure + apply |
| 20 | User Intent Layer | 🔮 | Input -> Intent -> Engine |

## Current State (Post-M17 - 2026-01-18)

### Recent Achievements ✅
- **GameControl Split:** Separated into `BetweenPitchState` (referee decisions) and `FlowControl` (user gates)
- **Wounding Logic:** Fully owned by referee, timer-based evaluation
- **Out of Bounds:** Clean pattern - referee decides, reconciliation executes
- **Pending Runs:** Ball-in-air run system with proper resolution
- **61 tests passing:** 48 unit + 13 integration (including 2 new pending run regression tests)

### Key Data Structures

```c
// Between-pitch sticky flags (reset at pitch start)
typedef struct _BetweenPitchState {
    int catchHasBeenMade;     // Fly ball was caught
    int hasBallHitGround;     // Ball has touched ground
    int outOfBounds;          // First bounce was out of bounds
    int resolutionProcessed;  // Referee adjudicated pitch
} BetweenPitchState;

// User interaction flow
typedef struct _FlowControl {
    int pause;
    int waitingForBatterDecision;
    int waitingForFreeWalkDecision;
    int freeWalkCalculationMade;  // Query result
    int freeWalkIndex;             // Who to offer
    BaseID freeWalkBase;           // From where
} FlowControl;

// Per-player rule tracking
typedef struct {
    BaseID currentSafetyBase;      // Legal safety
    BaseID baseAtPitchStart;       // Snapshot for foul reset
    int hasPendingWound;           // Catch evaluation pending
    int hasPendingRun;             // Run pending ball resolution
    int isOut;                     // Logical out
    int hasScored;                 // Logical score
    WoundingType woundingType;     // NORMAL or TUPLAHAAVA
    BaseID woundingSourceBase;     // Where to return if wounded
    // ...
} RefereePlayerState;
```

### Ownership Map

| Structure | Owner (Writer) | Readers | Lifecycle |
|-----------|----------------|---------|-----------|
| `GameEvents` | Physics/Actions | Referee | Clear every frame |
| `BetweenPitchState` | **Referee ONLY** | Everyone | Reset at pitch start |
| `FlowControl` | Game flow, UI | Referee, Actions | User-controlled |
| `RefereeState` | **Referee ONLY** | Everyone | Persistent (until specific events) |
| `HalfInningState` | **Referee ONLY** | Everyone | Persistent (inning-scoped) |

**Exception:** Initialization functions may write during setup (marked with `// REFEREE INIT`)

**Note:** This exception is temporary - next session will replace all init writes with events for true referee supremacy.

### Referee Pipeline

**File:** `src/game/referee.c` (~750 LOC)

**Flow:**
```c
void Referee_Update(StateInfo*, RefereeState*, HalfInningState*, 
                    BetweenPitchState*, PlayerCounters*, Scoreboard*)
{
    // 1. Ball location query
    int ballAtBase = get_ball_at_base_index();
    
    // 2. Foul play & wounding logic
    update_foul_play_logic();        // Out of bounds detection
    update_wounding_logic();          // Catch evaluation timer
    
    // 3. Safety pipeline
    update_safety_status();           // Grant/revoke safety
    update_force_outs_and_tuplahaava(); // Apply outs
    update_runs();                    // Award runs (or set pending)
    
    // 4. Pending run resolution
    resolve_pending_runs();           // Resolve after ball lands
    
    // 5. Strike/ball adjudication
    update_strikes();
    update_pitch_resolution();
    update_free_walk_resolution();
    
    // 6. State flags
    update_game_state_flags();        // Reset on pitch release
}
```

### The Hybrid Problem (Improving)

**Before (M16):**
- `game_analysis.c` competed with referee for authority
- Mixed physics, rules, and UI concerns everywhere

**Now (M17):**
1. **✅ Pure Core (`rules_pure/`, `referee.c`):** Clean, tested, authoritative
2. **✅ Event System (`GameEvents`):** Clear communication between layers
3. **🚧 Manipulation (`game_manipulation.c`):** Still mixes physics + state (M18 target)
4. **🚧 Analysis (`game_analysis.c`):** Slimmed down but still has flow logic

**Next Goal (M18):** Extract pure physics from `game_manipulation.c`

## Codebase Map

```
src/
├── core/ (~3.5k LOC)
│   ├── main.c                 Entry point
│   ├── state_validator.c      Runtime checks + debug dumps
│   └── ...
│
├── game/ (~9k LOC)
│   ├── referee.c ⭐            Referee_Update pipeline (750 LOC)
│   ├── mutable_world.c         Main loop, reconciliation
│   ├── game_analysis.c         Flow logic (being slimmed)
│   ├── game_manipulation.c     Physics + events (M18 target)
│   ├── common_logic.c          Shared helpers
│   │
│   ├── rules_pure/ ⭐
│   │   ├── base_logic.c       Safety helpers
│   │   ├── base_control.c     Base queries
│   │   ├── rules_outs.c       Out detection
│   │   ├── rules_runs.c       Run scoring
│   │   ├── rules_strikes.c    Strike/ball logic
│   │   └── player_utils.c     Player state queries
│   │
│   ├── actions_pure/          Pure physics (batting, pitching)
│   ├── actions_messy/         Action implementation (M19 target)
│   ├── ai_pure/               Strategy functions
│   └── ai_messy/              AI decision makers
│
├── menu/ ...
├── renderer/ ...
└── physics/                   Ball physics, collision
```

## Test Coverage

**61 tests passing** ✅

### Unit Tests (48)
- Rules: outs, runs, strikes, safety logic
- AI: catching strategy, batting strategy, pitching strategy  
- Actions: batting physics, pitching physics
- Base logic: queries, force outs, free walks

### Integration Tests (13)
- Runner scoring from third
- Forced outs at bases
- Fly ball wounding
- Chain reactions
- Tuplahaava (double wounding)
- Out of bounds reset
- Pitching: strikes & balls
- Free walk resolution
- Run of honor (kunniajuoksu)
- **Pending runs** (2 regression tests for ball-in-air scenarios)

## Next Steps

### Immediate (Next Session - ~4.5 hours)
**Goal:** Reach The Plateau - Referee Supremacy (sole writer, no exceptions)

**See:** `.dev/REFEREE_CONSOLIDATION_TODO.md` for detailed execution plan

**Key Tasks:**
1. Event-driven initialization (replace all direct writes with events)
2. Merge game_analysis → gameConsolidation
3. Clean 5-phase main loop: Inputs → Physics → Referee → Consolidation → Reset
4. All 61 tests still passing

**Result:** Perfect foundation - zero violations, zero exceptions, zero compromises

### Mid-term (Milestone 18 - Next 7-8 sessions)
**Physics/State Split:**
1. Extract pure physics from `game_manipulation.c`
2. Separate physical queries from legal state
3. Clear boundary: "what happened" vs "what it means"

*Note: PhysicsObserver not needed - physics setting GameEvents is the right pattern*

### Long-term (Milestones 19-20)
- M19: Decouple action systems (pure logic vs execution)
- M20: User Intent layer (replay foundation, networked multiplayer)

## Build & Run

```bash
# Development
make main                           # Build game
./main                              # Run
./main --debug-state crash.json     # Debug mode with state dumps

# Testing
devenv shell make test              # 48 unit tests
devenv shell make integration_test  # 13 scenario tests
devenv shell make clean             # Clean build artifacts
```

## Key Principles

1. **Referee Supremacy:** Only `referee.c` writes to `RefereeState`, `BetweenPitchState`
2. **One-Way Flow:** Physics → Events → Referee → Decisions → Reconciliation
3. **Clear Ownership:** Each struct has one writer, many readers
4. **Event-Driven:** Transient events (`GameEvents`) drive referee decisions
5. **Test Everything:** All rule changes require integration tests

---

**See Also:**
- `.dev/PLAN.md` - Detailed roadmap and task tracking
- `docs/SAANNOT.md` - Official pesäpallo rules (Finnish)