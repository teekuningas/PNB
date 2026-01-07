# PNB Architecture

## Vision: The Functional Pipeline

Transform from coordinated managers to a strict functional pipeline:

```
State_Next = Apply(Referee(Physics(Intent(State, Input))))
```

**Target loop:**
1. Input → Intent (e.g., INTENT_SWING_BAT)
2. Physics(State, Intent) → PhysicsResult
3. Referee(PhysicsResult) → Decisions (outs, runs, wounds)
4. Apply(Decisions) → NewState
5. Render(NewState)

## Milestone Progress

| # | Milestone | Status | Achievement |
|---|-----------|--------|-------------|
| 10-13 | State Consolidation | ✅ | StateInfo is single source of truth |
| 14 | The Great Decoupling | ✅ | **Referee_Analyze (pure) + Referee_Apply (impure)** |
| 15 | Action Decoupling | 🚧 | Split actions_messy/ into pure + apply |
| 16 | Intent Phase | 🔮 | Decouple input from execution |
| 17 | Full Pipeline | 🔮 | Linear game loop |

## Current State (M14 Complete)

### What Changed in M14

**Before:** `game_analysis.c` mixed rule logic with state mutation (~923 LOC)

**After:**
- `rules_pure/referee.c` (310 LOC) - Pure analysis, returns `RefereeDecisions`
- `referee_apply.c` (149 LOC) - Applies decisions to state
- `game_analysis.c` (613 LOC) - Coordinator calling analyze → apply

### Key Data Structures

```c
// Per-player rule tracking
typedef struct {
    BaseID currentSafetyBase;      // Replaces old baseControlIndex
    int hasPendingWound;
    WoundingType woundingType;
    BaseID baseAtPitchStart;       // Foul play snapshot
    // ...
} RefereePlayerState;

// Referee analysis output
typedef struct {
    PlayerDecision playerDecisions[24];  // Per-player: isOut, isWounded, etc
    int ballHome;
    int eventOut, eventRun;
    int canMakeRunOfHonor;
    int isPeriodEnd;
} RefereeDecisions;
```

### State Validator

**File:** `src/core/state_validator.c` (199 LOC)

**Checks:**
1. No two players on same base
2. Safety consistency (if claims Base X, must be there)
3. Reverse control (if physically safe, must claim it)

**Usage:** `./main --debug-state crash.json` → dumps JSON on violation

### Eliminated: baseControlIndex

**Old:** `int baseControlIndex[4]` - cached array  
**New:** `referee.battingPlayers[i].currentSafetyBase`  
**Query:** `get_base_controller(game, BASE_X)` iterates

## Codebase Map

### Directory Structure (15k LOC)

```
src/
├── core/ (~3.5k LOC)
│   ├── main.c                 Entry point
│   ├── state_validator.c      Runtime checks ⭐
│   ├── render.c               OpenGL
│   ├── input.c, sound.c, resource_manager.c
│   └── [vector_math, geometry, field_layout]
│
├── game/ (~8.5k LOC)
│   ├── game_screen.c (519)    Main game loop
│   ├── game_analysis.c (613)  Rule coordinator
│   ├── game_manipulation.c (1010)  Player movement
│   ├── common_logic.c (1032)  Shared helpers
│   │
│   ├── rules_pure/ ⭐ PURE (Query)
│   │   ├── referee.c (310)       Analysis engine
│   │   ├── base_logic.c          Safety helpers
│   │   ├── base_control.c        get_base_controller()
│   │   ├── rules_outs.c          Out detection
│   │   ├── rules_runs.c          Run scoring
│   │   └── rules_strikes.c       Strike/ball logic
│   │
│   ├── referee_apply.c (149) ⭐ IMPURE (Apply)
│   │
│   ├── actions_pure/
│   │   ├── batting_physics.c
│   │   └── pitching_physics.c
│   │
│   ├── actions_messy/ ⚠️ TO REFACTOR
│   │   ├── batting_system.c (473)
│   │   ├── pitching_system.c (385)
│   │   └── throwing_system.c (250)
│   │
│   ├── ai_pure/
│   │   ├── batting_ai_strategy.c
│   │   ├── catching_ai_strategy.c
│   │   └── pitching_ai_strategy.c
│   │
│   └── ai_messy/ ⚠️ TO REFACTOR
│       ├── batting_ai.c (407)
│       └── catching_ai.c (247)
│
├── menu/ (~2.8k LOC)
├── cup/ (~450 LOC)
├── renderer/ (~600 LOC)
└── physics/ (~400 LOC)

include/
└── globals.h (773 LOC) - All types, 38 structs
```

### Next Refactoring Targets (M15)

**actions_messy/** (~1,108 LOC)
- Split into `Action_Analyze()` + `Action_Apply()`
- Pattern: Same as referee (pure analysis → mutation)

**ai_messy/** (~654 LOC)
- Move pure logic to ai_pure/
- Leave only state mutation

**common_logic.c** (1,032 LOC)
- Audit: separate queries from mutators

## Test Coverage

**67 tests passing**

### Unit Tests (53)
- `test_rules_referee.c` - Pure referee
- `test_rules_outs.c`, `test_rules_runs.c`, `test_rules_strikes.c`
- `test_base_logic.c`
- `test_batting_physics.c`, `test_pitching_physics.c`
- `test_*_ai_strategy.c` - AI decision logic
- `test_cup_logic.c`

### Integration Tests (14)
- Force outs, runs, wounding
- Tuplahaava (§36 collision rules)
- Foul play, overtaking (§42 Run of Honor)
- Chain reactions, force play
- Fielder positioning (§31 placeholder)

## Implementation Patterns

### The Query/Apply Split

**Goal:** Separate "What should happen?" from "Make it happen"

```c
// QUERY (Pure function)
RefereeDecisions Referee_Analyze(const StateInfo* state) {
    RefereeDecisions decisions = {0};
    // Read state, detect outs/runs/wounds
    // NO STATE MUTATION
    return decisions;
}

// APPLY (Impure function)
void Referee_Apply(StateInfo* state, const RefereeDecisions* decisions) {
    // Mutate state based on decisions
    // Remove players, update scores, etc
}

// COORDINATOR
void gameAnalysis(StateInfo* state) {
    RefereeDecisions decisions = Referee_Analyze(state);
    Referee_Apply(state, &decisions);
}
```

**Why?**
- Pure functions are testable without game loop
- Clear separation of concerns
- Enables future replay/undo systems

### State Validation Pattern

```c
// After critical mutations
if (debugMode) {
    if (!validate_game_state(state)) {
        dump_game_state_to_json(state, "crash.json");
        exit(1);
    }
}
```

**Catches:**
- Ghost runners (player claims base but isn't there)
- Duplicate safety (two players on same base)
- Control inconsistency

## Technical Debt

1. **actions_messy/** - Mix of input, timing, physics, mutation (M15 target)
2. **ai_messy/** - Mix of decisions and mutation
3. **common_logic.c** - 1k LOC of mixed concerns
4. **globals.h** - 773 LOC "God header" (consider splitting later)

## Build & Run

```bash
make main                          # Build game
./main                             # Run
./main --debug-state crash.json    # Debug mode

devenv shell make test             # 53 unit tests
devenv shell make integration_test # 14 scenario tests
```

## Key Principles

1. **Data shapes architecture** - Clean data → obvious design
2. **Pure functions first** - Extract logic before refactoring
3. **Small safe steps** - Test each change
4. **Foundation before skyscraper** - Clean data before complex patterns

---

**Last updated:** 2026-01-07  
**Current:** Milestone 14 complete, starting M15
