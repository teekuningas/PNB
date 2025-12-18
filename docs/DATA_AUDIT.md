# Data Renaissance Audit (Milestone 7)

## ⚠️ IMPORTANT: Data-First Approach

**Updated 2025-12-18:** This document proposes enums, but Milestone 7 now follows a **data-first approach**:

1. **Phase 0:** Audit current data mess, design CLEAN model (without enums yet)
2. **Phase 1:** Add integration tests (behavior validation)
3. **Phase 2:** Migrate data structures using adapter pattern
4. **Phase 3:** Add enums to the CLEAN model (proposals below)

**See `.dev/PLAN.md` for full Milestone 7 strategy.**

---

## Objective

Identify ambiguous flags and magic numbers within `StateInfo` and propose cleaner data structures with semantic enums.

## Enum Proposals (To Apply After Data Migration)

### 1. Game Periods → `GamePeriodType` enum
- **Current:** `period >= 4` magic number (5 occurrences)
- **Problem:** Unclear meaning, overloaded semantics
- **Proposal:**
    ```c
    typedef enum {
        PERIOD_NORMAL_1 = 0,
        PERIOD_NORMAL_2 = 1,
        PERIOD_SUPER_INNING = 2,
        PERIOD_EXTRA_INNING = 3,
        PERIOD_HOMERUN_CONTEST = 4
    } GamePeriodType;
    ```
- **Note:** Can add immediately (simple magic number replacement)

---

### 2. Player Base → `BasePosition` enum
- **Current:** `base` integers (0=home start, 4=home after run)
- **Problem:** Semantic confusion (0 means two different things!)
- **Discovery:** From Milestone 6 audit - `player.base` = "where player IS or running FROM"
- **Proposal:**
    ```c
    typedef enum {
        BASE_HOME = 0,
        BASE_FIRST = 1,
        BASE_SECOND = 2,
        BASE_THIRD = 3,
        BASE_HOME_SCORED = 4,  // Distinct from BASE_HOME
        BASE_NONE = -1
    } BasePosition;
    ```
- **Note:** Requires data migration (clarify semantics first)

---

### 3. Player State Flags → `PlayerState` enum
- **Current:** Multiple boolean flags (`isOnBase`, `out`, `wounded`, `takingFreeWalk`)
- **Problem:** Flag soup, complex conditions: `isOnBase=0 && out=0 && takingFreeWalk=0`
- **Discovery:** These represent a SINGLE state machine, not independent flags
- **Proposal:**
    ```c
    typedef enum {
        PLAYER_STATE_BATTING,
        PLAYER_STATE_RUNNING,
        PLAYER_STATE_SAFE_ON_BASE,
        PLAYER_STATE_LEADING,
        PLAYER_STATE_TAKING_FREE_WALK,
        PLAYER_STATE_OUT,
        PLAYER_STATE_WOUNDED,
        PLAYER_STATE_SCORED
    } PlayerState;
    ```
- **Note:** Requires data migration (map all flag combinations first)

---

### 4. Game Info Events → `GameEventType` enum
- **Current:** `gameInfoEvent = 1/2/3/4...` magic numbers
- **Problem:** Undocumented event codes
- **Proposal:**
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
        EVENT_NEXT_PAIR = 8
    } GameEventType;
    ```
- **Note:** Can add immediately (simple magic number replacement)

---

## Phase 0 Tasks (Before Enums)

1. **Map flag combinations** - Document all uses of player state flags
2. **Draw state diagrams** - Visualize player state machine
3. **Design clean model** - Propose simpler structure
4. **Plan migration** - Adapter pattern strategy

**See `.dev/PLAN.md` Milestone 7 for detailed migration strategy.**
