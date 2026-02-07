# PNB Development Plan

## 🎯 CURRENT MILESTONE: Milestone 18 (Referee Refactoring & Debug Improvements)

**Current Status:** Milestone 18.1 Complete ✅ | M18.2 Test Fixture Unification Next
**Date:** 2026-02-07

With M18.1 debug logging improvements complete, we now focus on unifying test fixtures before tackling the internal referee refactoring.

---

## 🏔️ The Plateau (Current State)

We have achieved **Referee Supremacy** (complete):
*   ✅ `Referee_Update` is the **sole writer** of `RefereeState` and `BetweenPitchState`.
*   ✅ All "initialization exceptions" (foul reset, batter entry) are now handled via **Events**.
*   ✅ `game_analysis` has been merged into **`game_consolidation.c`**.
*   ✅ The Main Loop is strictly ordered: **Input → Physics → Referee → Consolidation**.
*   ✅ All 63 tests (48 unit + 15 integration) are passing.
*   ✅ **Strike Reset Bug Fixed:** Strikes/balls no longer reset after swinging.
*   ✅ **Test Infrastructure Unified:** All 15 integration tests follow Referee Supremacy pattern.
*   ✅ **Debug Logging Upgraded:** Now shows all players, scoreboard state, and game counters.

---

## ✅ Milestone 17.5: Homerun Contest & Final Referee Cleanup - COMPLETE

**Goal:** Test the Homerun Contest mode thoroughly and complete the final referee consolidation.

### Phase 1: Test Infrastructure Refactoring ✅ COMPLETE
*   ✅ All 15 integration tests refactored to follow **Referee Supremacy Pattern**
*   ✅ Tests now set up physical state first, then let referee infer legal state via events
*   ✅ Zero manual referee state manipulation in tests or helpers
*   ✅ Unified test patterns across all test types (fly ball, pitching, free walk, timing edge cases)
*   ✅ Test directory structure reorganized: `tests/unit/` and `tests/integration/`
*   ✅ New standardized helpers: `initialize_referee_from_physical_state()`, `snapshot_pitch_start_state()`, `move_pitcher_away()`
*   ✅ Removed unused helpers: `give_ball_to_fielder()`, `simulate_until()`

### Phase 2: Homerun Contest Logic Refactoring ✅ COMPLETE
*   ✅ Added `homerunPairHasPitch` tracking flag to prevent premature pair transitions
*   ✅ Removed `is_run_of_honor_possible()` function (homerun-contest specific, overly complex)
*   ✅ Rewrote pair-ending logic with three explicit, simple conditions:
    *   **Condition 1:** Runner at third, ball home, batter stuck (not advancing 2nd→3rd, no home safety)
    *   **Condition 2:** Runner not at third, ball home, batter can't reach third
    *   **Condition 3:** Ball home, no players with safety (safety net)
*   ✅ Logic is now **permissive** (allows play to continue) unless specific stuck conditions met
*   ✅ Uses simple state checks: `baseId`, `currentSafetyBase`, `ballHome` (no complex physical checks)
*   ✅ History-aware: only checks after first pitch (`homerunPairHasPitch`)

### Success Criteria - ALL MET ✅
*   ✅ All integration tests follow unified Referee Supremacy pattern
*   ✅ Test directory structure properly organized
*   ✅ Homerun Contest pair-ending logic simplified and working correctly
*   ✅ All tests pass (zero failures)
*   ✅ Zero direct writes to referee state from non-referee code (except `initializeRefereeState()` calls)

**Note:** GameFlowState cleanup and `setRunnerAndBatter()` refactoring deferred to later - not blocking for current work.


---

## ✅ Milestone 18.0: Initialization Cleanup - COMPLETE (2026-02-07)

**Goal:** Remove the confusing `gameInitialized` event and establish clear initialization patterns.

### Changes Made:

1. **Added `initialize_referee()` function**
   - Public function to scan physical world during setup
   - Replaces gameInitialized event pattern

2. **Renamed `Referee_Update` → `update_referee`**
   - Consistent snake_case naming

3. **Removed gameInitialized event completely**
   - From GameEvents struct
   - From event handling code
   - From clearFrameEvents

4. **Fixed double-initialization bug**
   - returnToGame() now consistent with initializeGameFromMenu()

5. **Documentation updated**
   - ARCHITECTURE.md and PLAN.md reflect current state

### Result:
✅ All 63 tests passing
✅ Clear patterns: Setup = explicit calls, Transitions = state machines
✅ Consistent naming throughout

---

## ✅ Milestone 18.1: Debug Logging Improvements - COMPLETE (2026-02-07)

**Goal:** Fix incomplete debug logging that hides critical game state.

**Problem Found:** Debug logs only showed active players, missing idle ones and global state.

**Changes Made:**
1.  **Full Player Roster:** Logs all 21 players (12 batting + 9 fielding) regardless of state.
2.  **Scoreboard Metadata:** Added period, inning, pairCount, run totals, and **batterOrder**.
3.  **Game Counters:** Added outs, strikes, balls, `nonJokerPlayersLeft`, `jokersLeft`.
4.  **Referee State:** Explicitly printing `safetyBase`, `baseAtPitchStart` for batting team.
5.  **Validation:** Verified correct indexing for batting/fielding teams.

---

## 🎯 Milestone 18.2: Test Fixture Unification (NEXT)

**Goal:** Ensure all test fixtures follow the same initialization pattern.

**Tasks:**
- ⏳ Audit all test initialization code
- ⏳ Verify integration tests use `initialize_referee()`
- ⏳ Verify human fixtures can use same pattern
- ⏳ Document standard test fixture setup procedure
- ⏳ Ensure all 63 tests pass with unified approach

---

## Milestone 18.3: Referee Internal Refactoring

**Goal:** Clean up referee.c internal structure.

**Tasks:**
- Extract state machines to dedicated functions
- Create RefereeContext struct for shared data
- Separate transition handling (end of inning, next pair, out of bounds)
- Improve code organization and readability

---

## 🎯 Milestone 19: Physics/State Split (Future)
*   Parameter explosion (functions take 5-7 parameters)
*   Numbering chaos in comments ("2.5", "3.5" showing organic growth)
*   Hard to see dependencies and what writes what

**Design Principles:**
*   Referee is beautifully decoupled from outside world ✅ (keep this!)
*   Explicit over clever - list conditions even if redundant
*   Clear state machines with explicit transitions
*   Visible boundaries: what's input, what's output, what's internal

---

#### 🟢 LOW-HANGING FRUIT (High Impact, Low Risk)

These changes improve readability immediately without breaking anything:

**Task 1: Extract State Machines to Functions**
*   ⏳ Move homerun contest logic (lines 962-1127) to `update_homerun_pair_state_machine()`
*   ⏳ Move end-of-inning logic to `update_end_of_inning_state_machine()`
*   ⏳ Both follow same pattern as `update_foul_play_logic()` and `update_wounding_logic()`
*   **Why:** Reduces main function from ~270 lines to ~150 lines
*   **Why:** Makes state machines consistent (all are functions, not inline)

**Task 2: Add Clear Section Headers**
*   ⏳ Replace numbered comments ("2.5", "3.5") with clear phase headers:
    ```c
    // ═══════════════════════════════════════════════════════
    // PHASE 1: STATE TRANSITIONS (Events)
    // ═══════════════════════════════════════════════════════
    
    // ═══════════════════════════════════════════════════════
    // PHASE 2: SPECIAL SITUATIONS (Fouls, Wounds)
    // ═══════════════════════════════════════════════════════
    
    // ═══════════════════════════════════════════════════════
    // PHASE 3: CORE RULINGS (Safety, Outs, Runs, Strikes)
    // ═══════════════════════════════════════════════════════
    
    // ═══════════════════════════════════════════════════════
    // PHASE 4: GAME FLOW STATE MACHINES
    // ═══════════════════════════════════════════════════════
    ```
*   **Why:** Makes structure immediately obvious when reading code

**Task 3: Rename for Clarity**
*   ⏳ `update_initialization_events()` → `handle_state_transitions()` (clearer: handles events)
*   ⏳ `update_game_state_flags()` → `update_sticky_flags()` (clearer: tracks ball state)
*   ⏳ Consider: `update_X` → `rule_on_X` for actual rulings (e.g., `rule_on_runs()`)
*   **Why:** "update" is vague; "handle"/"rule_on"/"process" show intent

---

#### 🟡 MEDIUM EFFORT (Larger Impact, Still Safe)

These changes reduce parameter passing and make dependencies explicit:

**Task 4: Introduce RefereeContext Struct**

Create a context struct to reduce parameter passing:

```c
typedef struct {
    // ─────────────────────────────────────────────
    // INPUTS (Read-Only - External World State)
    // ─────────────────────────────────────────────
    const StateInfo* state;
    const GameEvents* events;
    const FlowControl* flow;
    
    // ─────────────────────────────────────────────
    // OUTPUTS (Referee Writes - The "Ruling")
    // ─────────────────────────────────────────────
    RefereeState* referee;
    HalfInningState* halfInning;
    BetweenPitchState* betweenPitch;
    
    // ─────────────────────────────────────────────
    // SHARED OUTPUTS (Referee + Consolidation)
    // ─────────────────────────────────────────────
    PlayerCounters* players;
    Scoreboard* scoreboard;
    
    // ─────────────────────────────────────────────
    // CACHED (Computed once per frame)
    // ─────────────────────────────────────────────
    int ballAtBase;
    int ballHome;
} RefereeContext;
```

*   ⏳ Introduce struct gradually (one section at a time)
*   ⏳ Update helper functions to take `RefereeContext*` instead of 5-7 parameters
*   ⏳ Main function prepares context once, passes to all helpers
*   **Why:** Makes dependencies crystal clear
*   **Why:** Reduces visual clutter (one parameter instead of seven)
*   **Why:** Easy to add new cached values without changing all function signatures

**Task 5: Consistent State Machine Pattern**

All state machines follow the same structure:

```c
static void process_SYSTEM_state_machine(RefereeContext* ctx) {
    // 1. Read current state
    SystemState currentState = ctx->referee->systemState;
    
    // 2. State transitions (explicit switch/if-else)
    if (currentState == STATE_NONE) {
        // Check conditions...
        // Transition to STATE_DETECTED
    } else if (currentState == STATE_DETECTED) {
        // Update timer...
        // Transition to STATE_RESETTING
    } else if (currentState == STATE_RESETTING) {
        // Reset state...
        // Transition back to STATE_NONE
    }
}
```

*   ⏳ Ensure all state machines (`foul`, `wounding`, `homerun_pair`, `end_of_inning`) follow this pattern
*   ⏳ Move initialization/reset logic into explicit helper functions
*   **Why:** Consistent = easier to understand
*   **Why:** Makes state machines greppable and auditable

**Task 6: Group Related Helpers**

Organize internal helpers by concern with clear comments:

```c
// ═══════════════════════════════════════════════════════
// INITIALIZATION & RESET HELPERS
// ═══════════════════════════════════════════════════════
static void reset_referee_for_new_pitch(RefereeState* ref);
static void reset_referee_for_new_pair(RefereeState* ref, HomeRunContestState* hr);
static void reset_referee_for_new_inning(RefereeState* ref);
static void reset_referee_for_foul(RefereeState* ref);

// ═══════════════════════════════════════════════════════
// PLAYER STATE QUERIES
// ═══════════════════════════════════════════════════════
static int get_batter_index(const RefereeState* ref);
static int get_runner_index(const RefereeState* ref, BaseID base);
static bool player_has_safety_at(const RefereeState* ref, int playerIdx, BaseID base);

// ═══════════════════════════════════════════════════════
// BALL STATE QUERIES
// ═══════════════════════════════════════════════════════
static int get_ball_at_base(const StateInfo* state);
static bool is_ball_home(const MatchSession* game);
```

*   ⏳ Extract repeated logic (finding batter/runner, checking safety) into small query functions
*   ⏳ Group by concern with clear section headers
*   **Why:** Reduces duplication
*   **Why:** Makes helper functions discoverable

---

### Success Criteria for Milestone 18

**Part A (Test Fixtures):**
*   ⏳ All initialization paths documented
*   ⏳ Single unified initialization pattern
*   ⏳ Human test fixtures follow unified pattern
*   ⏳ All tests still pass

**Part B (Referee Refactoring):**
*   ⏳ Main `Referee_Update()` reduced to ~150 lines (from ~270)
*   ⏳ All state machines are functions (not inline)
*   ⏳ Clear phase headers in main function
*   ⏳ RefereeContext struct reduces parameter passing
*   ⏳ Consistent state machine pattern across all systems
*   ⏳ Helper functions grouped by concern
*   ⏳ All existing tests still pass
*   ⏳ Code is more readable and maintainable

**Quality Check:**
*   Can a new developer understand the referee flow in 10 minutes?
*   Are dependencies (inputs/outputs) obvious at a glance?
*   Can you add a new state machine without studying existing code?

---

## 🎯 Milestone 19: Physics/State Split (Next 7-8 sessions)

**Goal:** Deconstruct `game_manipulation.c` (~1500 LOC) into a pure Physics Engine.

**Why:** Currently, `game_manipulation.c` mixes pure physics integration (velocity/gravity) with game state mutations and event logic. We want to separate "what happened physically" from "game rules".

### Phase 1: Pure Physics Core
*   Create `PhysicsEngine` module.
*   Functions should take `State` + `dt` and return `NewPosition` / `CollisionEvents`.
*   No side effects, no dependencies on `RefereeState` or `GameControl`.

### Phase 2: Physics Observer
*   Create a bridge that watches Physics output and emits `GameEvents` (e.g., "Ball Hit Ground", "Catch Made").
*   This decouples the physics calculation from the event system.

### Phase 3: The Split
*   Refactor `game_manipulation.c`:
    *   Extract movement logic to `PhysicsEngine`.
    *   Extract event triggering to `PhysicsObserver`.
    *   Rename remaining logic to `game_state_updater.c` (or similar).

---

## 🔮 Future Milestones

*   **Milestone 20:** Action System Decoupling (Split `actions_messy/` into pure logic + execution).
*   **Milestone 21:** User Intent Layer (Input → Intent → Engine). Foundation for replay/network.

---

## 📋 Task Log (Completed)

| Milestone | Task | Result |
| :--- | :--- | :--- |
| **M17.5** | **Homerun Contest & Final Cleanup** | **✅ COMPLETE** |
| | Test Infrastructure Refactoring | ✅ All 15 tests unified |
| | Test Directory Reorganization | ✅ tests/unit/ and tests/integration/ |
| | Standardized Test Helpers | ✅ initialize_referee, snapshot_pitch, move_pitcher_away |
| | Strike Reset Bug | ✅ Fixed batterEntered event timing |
| | Homerun Contest Pair Logic | ✅ Simplified with 3 explicit conditions |
| | Added homerunPairHasPitch Tracking | ✅ Prevents premature transitions |
| | Removed is_run_of_honor_possible | ✅ Logic now inline and clearer |
| | Pair-Ending Conditions | ✅ Permissive, history-aware, simple |
| **M18** | **Referee Refactoring & Test Unification** | **⏳ IN PROGRESS** |
| | Debug Logging Improvements | ✅ Show all players + metadata |
| | Test Fixture Unification | ⏳ TODO |
| | Extract State Machines | ⏳ TODO (homerun pair, end of inning) |
| | Add Clear Section Headers | ⏳ TODO |
| | Rename for Clarity | ⏳ TODO |
| | Introduce RefereeContext Struct | ⏳ TODO |
| | Consistent State Machine Pattern | ⏳ TODO |
| | Group Related Helpers | ⏳ TODO |
| **M17** | **Referee Consolidation** | **✅ COMPLETED** |
| | GameControl Split | ✅ Separated Flow vs. Rules |
| | Wounding Logic | ✅ Fully timer-based in Referee |
| | Event-Driven Init | ✅ Batter Entry & Pitch Start use events |
| | Foul Reset Fix | ✅ Event-driven reset, zero writes outside Referee |
| | Loop Reordering | ✅ Analysis/Consolidation runs after Referee |
| | Consolidation Module | ✅ Merged Analysis + Reconciliation |

---

**See Also:**
*   `docs/ARCHITECTURE.md` - Detailed architectural vision.
*   `.dev/TASK_AGENT.md` - Protocol for task execution.