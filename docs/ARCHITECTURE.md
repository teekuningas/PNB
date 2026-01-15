# PNB Architecture

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
| 16 | Centralized Mutation | ✅ | Initial move of rule writes to Referee |
| 17 | Kill The Analyst | 🚧 | **Delete game_analysis.c**; Complete Referee Authority |
| 18 | Physics/State Split | 🔮 | **The Big One:** Deconstruct game_manipulation into Physics + Rules |
| 19 | Action Decoupling | 🔮 | Split actions_messy/ into pure + apply |
| 20 | User Intent Layer | 🔮 | Input -> Intent -> Engine |

## Current State (Post-M16)

### Key Achievements
- `Referee_Update` is a functional pipeline.
- `GameEvents` structure exists for communication.

### The Hybrid Problem
We currently have a "Hybrid" architecture:
1.  **Pure Core (`rules_pure/`):** Nice, clean, testable.
2.  **Legacy Managers (`game_analysis.c`, `game_manipulation.c`):** Old-style "Manager" classes that do too much (Physics + Rules + UI).

**Immediate Goal:** Eliminate `game_analysis.c`. It competes with `referee.c` for authority.
**Next Major Goal:** Tackle `game_manipulation.c`. This is the "God Object" of the system.

### Key Data Structures

```c
// Per-player rule tracking (in RefereeState)
typedef struct {
    BaseID currentSafetyBase;      // Replaces old baseControlIndex
    int hasPendingWound;
    WoundingType woundingType;
    BaseID baseAtPitchStart;       // Foul play snapshot
    int isOut;                     // Logical out
    int hasScored;                 // Logical score
    // ...
} RefereePlayerState;

// Note: RefereeDecisions struct is GONE.
```

### Referee Pipeline

The `Referee_Update` function runs a sequence of logic updates directly on the state:
1. `get_ball_at_base_index` (Query)
2. `update_safety_status` (Grants/Removes safety based on location)
3. `update_force_outs_and_tuplahaava` (Applies Outs, Events)
4. `update_runs` (Calculates runs, updates scores)

### State Validator

**File:** `src/core/state_validator.c` (199 LOC)

**Checks:**
1. No two players on same base
2. Safety consistency (if claims Base X, must be there)
3. Reverse control (if physically safe, must claim it)

**Usage:** `./main --debug-state crash.json` → dumps JSON on violation

## Codebase Map

### Directory Structure

```
src/
├── core/ (~3.5k LOC)
│   ├── main.c                 Entry point
│   ├── state_validator.c      Runtime checks ⭐
│   └── ...
│
├── game/ (~8.5k LOC)
│   ├── game_screen.c          Main game loop
│   ├── game_analysis.c        Legacy coordinator (slimmed down)
│   ├── game_manipulation.c    Player movement + ballHome logic
│   ├── common_logic.c         Shared helpers
│   ├── mutable_world.c        Main Update Loop (calls Referee_Update)
│   │
│   ├── rules_pure/ ⭐ REF CORE
│   │   ├── referee.c          Referee_Update pipeline
│   │   ├── base_logic.c       Safety helpers
│   │   ├── base_control.c     get_base_controller(), get_ball_at_base()
│   │   ├── rules_outs.c       Out detection
│   │   ├── rules_runs.c       Run scoring
│   │   └── rules_strikes.c    Strike/ball logic
│   │
│   ├── actions_pure/ ...
│   ├── actions_messy/ ...
│   └── ai_messy/ ...
│
├── menu/ ...
├── cup/ ...
├── renderer/ ...
└── physics/ ...

include/
└── globals.h - All types
```

### Next Refactoring Targets (M16)

**Centralized Mutation**
- Move `foulPlay` restoration logic to Referee.
- Move `EVENT_STRIKE/BALL` logic to Referee.
- Ensure `game_manipulation` only handles physics, not rules.

**Game Manipulation**
- Break down into `updatePhysics`, `updateAI`, `updateReactions`.

## Test Coverage

**67 tests passing**

### Unit Tests (53)
- `test_rules_referee.c` - Verified with Referee_Update
- `test_rules_outs.c`, `test_rules_runs.c`, `test_rules_strikes.c`
- `test_base_logic.c`
- ...

### Integration Tests (14)
- **Full Scenario:** `test_full_scenarios.c` (Force Out, Scoring)
- **Legacy Scenarios:** Chain reactions, Tuplahaava, Foul Play, etc.

## Build & Run

```bash
make main                          # Build game
./main                             # Run
./main --debug-state crash.json    # Debug mode

devenv shell make test             # 53 unit tests
devenv shell make integration_test # 14 scenario tests
```

---

**Last updated:** 2026-01-08
**Current:** Milestone 15 complete, starting M16