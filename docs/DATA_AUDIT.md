# Data Renaissance Audit (Milestone 7)

## Objective:
To identify ambiguous flags and magic numbers within `StateInfo` (specifically `LocalGameInfo` and `GlobalGameInfo`) and propose replacements with clear enums and more specific data structures.

## Initial Audit Observations & Proposals:

### 1. Game Periods (`stateInfo->globalGameInfo->period`)
- **Current Usage:** Integers (e.g., `period >= 4` for homerun contest).
- **Problem:** "Magic number" `4` is unclear without context. `period` is also overloaded (regular innings, super innings, homerun contest).
- **Proposal:** Introduce an enum `GameMode` or `GamePeriodType`.
    ```c
    typedef enum {
        NORMAL_PERIOD_1 = 0,
        NORMAL_PERIOD_2,
        SUPER_INNING,
        HOMERUN_CONTEST,
        // ... others as needed
    } GamePeriodType;
    
    // Usage:
    // stateInfo->globalGameInfo->periodType = HOMERUN_CONTEST;
    // if (stateInfo->globalGameInfo->periodType == HOMERUN_CONTEST) { ... }
    ```

### 2. Player Base (`playerInfo[index].bTPI.base`)
- **Current Usage:** Integers (e.g., `base == 0` for home, `base == 1` for first, `base == 4` for home after run).
- **Problem:** `0` for home base as start, but `4` for home base as arrival. Inconsistent and prone to errors. Magic numbers.
- **Proposal:** Introduce an enum `BasePosition`.
    ```c
    typedef enum {
        HOME_BASE = 0,
        FIRST_BASE = 1,
        SECOND_BASE = 2,
        THIRD_BASE = 3,
        HOME_PLATE_AFTER_RUN = 4, // distinct from HOME_BASE_START
        // ... potentially others like NOT_ON_BASE = -1
    } BasePosition;
    
    // Usage:
    // if (playerInfo[index].bTPI.base == HOME_BASE) { ... }
    // if (playerInfo[index].bTPI.base == HOME_PLATE_AFTER_RUN) { ... }
    ```

### 3. Player State Flags (`playerInfo[index].bTPI.isOnBase`, `playerInfo[index].bTPI.out`, `playerInfo[index].bTPI.wounded`, `playerInfo[index].bTPI.takingFreeWalk`)
- **Current Usage:** Multiple boolean flags that are often dependent or mutually exclusive.
- **Problem:** Leads to complex `if` conditions (e.g., `isOnBase == 0 && out == 0 && takingFreeWalk == 0`). A single source of truth is better.
- **Proposal:** Introduce an enum `PlayerRunnerState`.
    ```c
    typedef enum {
        PLAYER_STATE_RUNNING,
        PLAYER_STATE_SAFE_ON_BASE,
        PLAYER_STATE_OUT,
        PLAYER_STATE_WOUNDED,
        PLAYER_STATE_TAKING_FREE_WALK,
        // ... other states like BATTING, PITCHING
    } PlayerRunnerState;
    
    // Usage:
    // playerInfo[index].bTPI.runnerState = PLAYER_STATE_OUT;
    // if (playerInfo[index].bTPI.runnerState == PLAYER_STATE_SAFE_ON_BASE) { ... }
    ```

### 4. Game Info Events (`stateInfo->localGameInfo->gAI.gameInfoEvent`)
- **Current Usage:** Integers (e.g., `gameInfoEvent = 1` for out, `gameInfoEvent = 3` for run).
- **Problem:** Magic numbers for display events.
- **Proposal:** Introduce an enum `GameEventDisplayType`.
    ```c
    typedef enum {
        EVENT_NONE = 0,
        EVENT_OUT = 1,
        EVENT_WOUNDED = 2,
        EVENT_RUN_SCORED = 3,
        EVENT_FOUL_PLAY = 4,
        EVENT_STRIKE = 5,
        EVENT_BALL = 6,
        EVENT_END_OF_INNING = 7,
        EVENT_NEXT_PAIR = 8,
    } GameEventDisplayType;
    ```

## Next Steps:
This document serves as the preliminary audit for Milestone 7. The next session will involve: 
1. Integrating these enums into `globals.h` (or appropriate new header).
2. Systematically replacing magic numbers and flag combinations in core logic files. 
3. Updating affected pure functions and their tests to use the new enums. 
4. Extending this audit to other parts of `StateInfo` as needed.
