# Data Audit & Renaissance Plan (Milestone 7)

**Goal:** Transform "Flag Soup" and "Magic Numbers" into explicit, safe, and semantic data structures.
**Philosophy:** Functional Data Flow. Data structures should be robust, general, and self-documenting.

---

## 1. Current State Analysis

### A. The "Flag Soup" (Player Status)
Currently, a player's state is determined by a combination of boolean flags in `BattingTeamPlayerInfo`.

**Flags:**
- `isOnBase`: 1 if safe on a base, 0 if running or out.
- `out`: 1 if the player is out.
- `wounded`: 1 if the player was "wounded" (caught off base but not out).
- `takingFreeWalk`: 1 if the player is advancing freely (e.g., on a walk).
- `leading`: 1 if the player is leading off a base.

**Observed Combinations (Implicit States):**
| isOnBase | out | wounded | takingFreeWalk | implied State |
|:---:|:---:|:---:|:---:|---|
| 0 | 0 | 0 | 0 | **RUNNING** (Vulnerable) |
| 1 | 0 | 0 | 0 | **SAFE_ON_BASE** |
| 0 | 1 | 0 | 0 | **OUT** |
| 0 | 0 | 1 | 0 | **WOUNDED** (Removed from field) |
| 0 | 0 | 0 | 1 | **WALKING** (Invulnerable) |
| 1 | 0 | 0 | 0 | **LEADING** (if `leading` flag is separate) |

**Problem:** Invalid states are possible (e.g., `isOnBase=1` AND `out=1`). Logic requires checking multiple flags.

### B. Magic Numbers: `gameInfoEvent`
Used to trigger on-screen messages in `game_screen.c`.

| Value | Meaning (Text) | Context |
|:---:|---|---|
| 1 | **OUT** | Player burned at base |
| 2 | **WOUNDED** | Player caught but not out |
| 3 | **RUN** | Run scored |
| 4 | **OUT OF BOUNDS** | Ball went out |
| 5 | **STRIKE** | Strike called |
| 6 | **BALL** | Ball called |
| 7 | **HALF-INNING ENDS** | 3 outs or max runs |
| 8 | **NEXT PAIR** | Homerun contest next pair |
| 9 | **TWO RUNS** | Homerun contest scoring |

### C. Magic Numbers: `period`
Used to determine the game phase.

| Value | Meaning | Context |
|:---:|---|---|
| 0-3 | **Normal Innings** | Standard gameplay |
| >= 4 | **Homerun Contest** | "Hutunkeitto" / Tie-breaker |

### D. Ambiguous Semantics: `player.base`
- **Context 1:** Base the player is currently SAFE at.
- **Context 2:** Base the player is running FROM (during movement).
- **Context 3:** Value 4 means "Home/Scored" in some contexts, but 0 means "Home" in others.

---

## 2. Proposed Design (The Renaissance)

We will introduce strictly typed Enums to replace these magic values.

### A. `PlayerUnitState` (Replaces Flag Soup)
A single source of truth for the player's current status.

```c
typedef enum {
    // Default state
    PLAYER_STATE_IDLE = 0,
    
    // Batting states
    PLAYER_STATE_AT_BAT,
    
    // Runner states
    PLAYER_STATE_SAFE_ON_BASE,      // Replaces isOnBase=1
    PLAYER_STATE_RUNNING,           // Replaces isOnBase=0, out=0, wounded=0
    PLAYER_STATE_ADVANCING_FREELY,  // Replaces takingFreeWalk=1
    PLAYER_STATE_LEADING,           // Replaces leading=1
    
    // Terminal states (for the inning)
    PLAYER_STATE_OUT,               // Replaces out=1
    PLAYER_STATE_WOUNDED,           // Replaces wounded=1
    PLAYER_STATE_SCORED             // Explicit state for having scored
} PlayerUnitState;
```

### B. `BaseID` (Replaces `player.base` integers)
Explicit base identifiers to avoid 0 vs 4 confusion.

```c
typedef enum {
    BASE_HOME = 0,
    BASE_FIRST = 1,
    BASE_SECOND = 2,
    BASE_THIRD = 3,
    BASE_HOME_SCORED = 4, // Explicitly distinct from starting at home
    BASE_NONE = -1
} BaseID;
```

### C. `GameEventType` (Replaces `gameInfoEvent`)
```c
typedef enum {
    EVENT_NONE = 0,
    EVENT_OUT = 1,
    EVENT_WOUNDED = 2,
    EVENT_RUN_SCORED = 3,
    EVENT_OUT_OF_BOUNDS = 4,
    EVENT_STRIKE = 5,
    EVENT_BALL = 6,
    EVENT_INNING_ENDING = 7,
    EVENT_NEXT_PAIR = 8,
    EVENT_TWO_RUNS_SCORED = 9
} GameEventType;
```

### D. `GamePeriodMode` (Replaces `period >= 4`)
```c
typedef enum {
    GAME_MODE_REGULAR_INNING = 0,
    GAME_MODE_SUPER_INNING = 1,    // Potential mapping for period 2/3 if distinct
    GAME_MODE_HOMERUN_CONTEST = 2  // Replaces period >= 4
} GamePeriodMode;
```

---

## 3. Migration Strategy (Safety First)

We will use an **Adapter Pattern** to migrate without breaking the 48 existing unit tests or the game logic.

### Phase 1: Integration Safety Net
Before changing ANY data structures, we must add **Integration Tests** that verify high-level behavior.
- Test: "Runner moves from Base 1 to Base 2 -> State changes to Safe on Base 2"
- Test: "Ball caught -> Runner off base becomes Wounded"
- **Goal:** These tests must pass on the OLD code and the NEW code.

### Phase 2: Hybrid Data Structures
We will add the new Enums alongside the old flags in the structs.

```c
typedef struct {
    // ... old flags ...
    int isOnBase;
    int out;
    
    // ... NEW field ...
    PlayerUnitState state; 
} BattingTeamPlayerInfo;
```

### Phase 3: Adapter Logic
We will write helper functions to sync them.
- `update_player_state(player)`: Reads flags -> sets Enum.
- `update_player_flags(player)`: Reads Enum -> sets flags.

### Phase 4: Refactor & Cleanup
1. Switch logic to read/write Enums.
2. Use `update_player_flags` to keep legacy rendering/UI code working.
3. Eventually remove the flags once all systems are converted.

---

## 4. Immediate Next Steps (Task Agent)

1.  **Create Integration Test Harness:** Set up `tests/integration/` and a simple fixture loader.
2.  **Write Baseline Tests:** Implement 3-4 critical scenario tests using current flags.
    -   Force out at base.
    -   Scoring a run.
    -   Wounded player logic.