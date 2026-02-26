# Final Refactoring Plan

---

## Guiding Principles

1. **Data ownership is sacred.** Each pipeline stage owns specific structs. Writes must go through proper signatures, never through const-casts.
2. **Referee decides, Consolidation acts.** The referee sets legal status flags. Consolidation enforces them physically.
3. **The compiler is our enforcer.** `const` in signatures isn't documentation — it's a contract. If the code compiles without casts, ownership is correct.
4. **Small verifiable steps.** Each step must leave all 63 tests passing. No multi-step changes that can't be tested in isolation.
5. **Each step gets its own analysis.** Before implementing any step, we re-read the relevant code, verify the step is still needed, identify risks, and plan testing.

---

## Function Signature Strategy

### The Current Pattern

The main loop in `updateMutableWorld()` calls these stages:

```
1. actionInvocations(stateInfo)              — StateInfo*
2. actionImplementation(stateInfo, rng_seed)  — StateInfo*
3. gameManipulation(stateInfo)                — StateInfo*
4. update_referee(stateInfo, ref, half, bp, pc, sb)  — const StateInfo* + 5 writable ptrs
5. GameConsolidation_Update(stateInfo, menu, rng)    — StateInfo*
6. clearFrameEvents(&gameEvents)
```

The referee is the only stage that takes `const StateInfo*`. Everyone else takes mutable `StateInfo*`. This is actually correct and standard — here's why:

### Why `const` + Explicit Writable Pointers Is the Right Pattern

In C, `const T*` means "I promise not to write through this pointer." When a function takes `const StateInfo*` plus explicit non-const pointers to specific sub-structs, the signature itself becomes a **contract**:

```c
void update_referee(
    const StateInfo* stateInfo,       // I can READ everything
    RefereeState* refereeState,       // I WRITE referee rulings
    HalfInningState* halfInningState, // I WRITE inning state
    BetweenPitchState* betweenPitchState, // I WRITE pitch tracking
    PlayerCounters* playerCounters,   // I WRITE player counts
    Scoreboard* scoreboard            // I WRITE scores
);
```

This is standard C practice. The compiler enforces it — any attempt to write through `stateInfo` produces an error. The only escape is an explicit `(Type*)` cast, which is a visible red flag in code review.

### Why This Comes Naturally (Not Fighting the Language)

- **Writing through `const*` requires an explicit cast.** You have to type `((MatchSession*)game)->` to cheat. This is unnatural, ugly, and grep-able. It's C's way of saying "you're doing something wrong."
- **Writing through a non-const pointer is normal.** `refereeState->outs = 3;` compiles cleanly. No casts, no warnings. If your code compiles without casts, your ownership is correct.
- **The pattern is self-documenting.** A new developer reading the signature immediately knows: "this function reads the world and writes to these 5 things."

### What About the 6-Argument Problem?

Six parameters is a lot for a C function. But the alternatives have worse tradeoffs:

**Option: Pass `MatchSession*` directly.**
This gives write access to *everything* — no ownership contract at all. We lose the compiler as our enforcer. Rejected.

**Option: Bundle writable pointers into a context struct.**
```c
typedef struct { RefereeState* ref; HalfInningState* half; ... } RefereeContext;
```
This hides the ownership contract inside a type. It's cleaner at the call site but the permissions are no longer visible in the function signature — you have to look at the struct definition. It also adds a new type that needs maintenance. Not worth it for one function.

**Our choice: Keep explicit parameters.**
Six arguments is fine. `update_referee()` is called from exactly one place (`updateMutableWorld`). The call site is already clear:
```c
update_referee(stateInfo, &game->referee, &game->halfInningState,
               &game->betweenPitchState, &game->playerCounters, &game->scoreboard);
```
This reads naturally: "update referee, giving it write access to referee state, half-inning state, between-pitch state, player counters, and scoreboard."

### Why Other Stages Use `StateInfo*` (Non-Const)

Stages 1-3 (actions + manipulation) and stage 5 (consolidation) modify the physical world — player positions, ball state, animation flags. These touch dozens of fields across many structs. Listing every writable sub-struct would create 20+ parameter functions for no benefit.

The `const` pattern is valuable specifically for the **referee** because the referee has a special role: it sees everything but only rules on specific things. That asymmetry is exactly what `const StateInfo*` + explicit writable pointers expresses.

### The Rule Going Forward

- **Referee:** `const StateInfo*` + explicit writable pointers. Zero const-casts allowed.
- **Everyone else:** `StateInfo*` (full access). They're physical-world actors.
- **Pure functions (rules_pure, etc.):** Take values, return values. No state pointers at all.

If the referee ever needs to write to something not in its parameter list, that's a design signal: either the data belongs in a referee-owned struct, or the write belongs in consolidation.

---

## The State Machine Pattern Is Correct

The referee uses three state machines for transitions:

```
End-of-Inning:  NONE → DETECTED → (timer 200 frames) → RESETTING → (consolidation acts) → NONE
Foul Play:      NONE → DETECTED → (timer) → RESETTING → (consolidation acts) → NONE
HR Next Pair:   NONE → DETECTED → (timer 200 frames) → RESETTING → (consolidation acts) → NONE
```

This is a clean **two-phase commit protocol**: referee decides and announces (DETECTED), waits a grace period for visual feedback, then signals (RESETTING). Consolidation sees RESETTING and performs the physical reset. Referee sees that consolidation acted and returns to NONE.

These state machines live correctly in the referee. They encode *when* transitions happen (a ruling) and *how long* to wait (a presentation decision that's tightly coupled to the ruling). Moving the timers elsewhere would split the state machine across two files for no benefit.

The clearing of referee-owned state (strikes, balls, betweenPitchState flags, battingPlayers array) during the DETECTED→RESETTING transition also belongs in the referee — it's the referee preparing its own slate for the next inning/pair.

**We do not touch these state machines.**

---

## Phase 1: Dead Code Cleanup ✅ DONE

**Goal:** Remove noise so future diffs are cleaner.

### Step 1.1: Delete Dead Fixture Functions

`setup_runner_at_first_base()` and `setup_runner_at_third_base()` in `tests/integration/fixtures.c` are never called by any test. Remove from `.c` and `.h`. Run all 63 tests.

### Step 1.2: Document Intentional Omission in simulate_frames

`simulate_frames()` in `scenario_builder.c` omits `actionInvocations()`. This is intentional — tests control player/AI decisions explicitly. Add a one-line comment explaining this.

### Step 1.3: Remove Empty If-Block

`baseRunnerMovementsOnBaseArrivals()` in `game_manipulation.c` lines 379-384 contains an empty if-block with a comment saying "referee will handle this." The block does nothing. Remove it, keep the else branch.

### Step 1.4: Replace Tabs with Spaces

Convert all tabs to 4 spaces across every `.c` and `.h` file in the codebase. This is standard modern C style and makes tooling (editors, diff viewers, AI tools) more predictable. Use `expand -t 4` to do the conversion in a single commit, update `make format` to use `--indent=spaces=4` instead of `--indent=tab=4`, then add a `.git-blame-ignore-revs` entry for that commit so `git blame` skips the formatting change.

---

## Phase 2: WOUNDED Enforcement — Fix the Layer Violation 🎯 START HERE

**Goal:** Make WOUNDED follow the same pattern as OUT and SCORED: referee decides, `enforceLegalState()` acts.

### Step 2.1: Add WOUNDED Case to enforceLegalState()

In `game_consolidation.c`, `enforceLegalState()` currently handles OUT (lines 76-81) and SCORED (lines 85-88). Add a parallel block for WOUNDED:

```c
// 3. React to WOUNDED
if (game->referee.battingPlayers[i].status == PLAYER_STATUS_WOUNDED) {
    if (game->playerInfo[i].bTPI.state != PLAYER_STATE_WOUNDED) {
        game->playerInfo[i].bTPI.state = PLAYER_STATE_WOUNDED;
        game->playerInfo[i].bTPI.baseId = BASE_NONE;
        movePlayerOut(game->playerInfo, game->playerRuntime, stateInfo->fieldPositions, i);
    }
}
```

Run all 63 tests (especially `test_fly_ball_double_wound`).

### Step 2.2: Remove processPendingWounds()

Once enforceLegalState handles WOUNDED, `processPendingWounds()` in `game_manipulation.c` (lines 307-328) is redundant. Remove the function and its call site in `gameManipulation()`. Run all 63 tests.

### Step 2.3: Remove Redundant movePlayerOut in baseRunnerMovementsOnBaseArrivals

Lines 366-371 in `game_manipulation.c` check for `PLAYER_STATE_WOUNDED` and call `movePlayerOut()`. With WOUNDED handled by enforceLegalState, this is redundant. Remove the wounded branch. Run all 63 tests.

**Note:** Steps 2.1-2.3 should be done in sequence. Step 2.1 adds the new handler. Steps 2.2 and 2.3 remove the old handlers one at a time, testing after each removal. If a test breaks, we know exactly which removal caused it.

---

## Phase 3: Referee Ownership Cleanup — Eliminate All Const-Casts

**Goal:** Zero const-casts in referee.c. The type system becomes our enforcer.

### Step 3.1: Move homerunPairHasPitch into RefereeState

`homerunPairHasPitch` is written exclusively by referee (4 sites) and read exclusively by referee (1 site). It tracks whether a pitch has been released in the current HR pair — this is a referee observation, like `ballInThirdBaseSincePitch`. It belongs in `RefereeState`, not `HomeRunContestState`.

- Add `int homerunPairHasPitch;` to `RefereeState` in globals.h
- Change all 5 references in referee.c from `((MatchSession*)game)->homeRunContestState.homerunPairHasPitch` to `refereeState->homerunPairHasPitch`
- Reset it in `initializeRefereeState()` (alongside other referee fields)
- Remove it from `HomeRunContestState` (verify no other code reads it)
- Run all 63 tests

This eliminates 4 const-cast writes and 1 const-cast read (5 of 12 total).

### Step 3.2: Move waitingForBatterDecision=0 to Consolidation

Line 1101 in referee.c: `((FlowControl*)flowControl)->waitingForBatterDecision = 0;`. This cancels the batter selection prompt when inning is ending. It's a *reaction* to the referee's decision, not the decision itself.

Consolidation already checks `endOfInningState`. In `updateGameFlow()` → `checkIfNextBatterDecision()`, add a guard:

```c
if (stateInfo->match->referee.endOfInningState != END_INNING_STATE_NONE) {
    stateInfo->match->flowControl.waitingForBatterDecision = 0;
    return;
}
```

Remove line 1101 from referee.c. This eliminates 1 const-cast write.

### Step 3.3: Move Free Walk Reset to Consolidation

Lines 698-700 in referee.c (`update_pitch_resolution`): when referee determines a ball, it resets `flowControl->freeWalkCalculationMade/freeWalkIndex/freeWalkBase`. The referee's job is to count the ball. The *consequence* (recalculating free walk eligibility) is consolidation's job.

`betweenPitchState->resolutionProcessed` is already set on the same line (704). Consolidation already reads this flag (enforceLegalState lines 109-112). Add the free walk reset to consolidation's reaction to `resolutionProcessed`:

```c
if (game->betweenPitchState.resolutionProcessed) {
    game->pRAI.pitchState = PITCH_STAGE_NONE;
    // Re-evaluate free walk eligibility after pitch resolution
    game->flowControl.freeWalkCalculationMade = 0;
    game->flowControl.freeWalkIndex = -1;
    game->flowControl.freeWalkBase = BASE_NONE;
    game->betweenPitchState.resolutionProcessed = 0;
}
```

Remove lines 698-700 from referee.c and remove the `FlowControl*` parameter from `update_pitch_resolution()`. This eliminates 3 const-cast writes and the `(FlowControl*)` cast at the call site (line 916).

### Step 3.4: Fix get_base_controller Signature

`get_base_controller()` in `common_logic.c` takes `MatchSession*` but only reads. Change to `const MatchSession*`. This eliminates 1 unnecessary cast (line 87).

### Step 3.5: Fix initialize_referee Signature

`initialize_referee()` takes `const StateInfo*` but writes to `stateInfo->match->referee` (its own state) and `homeRunContestState.homerunPairHasPitch` (which will be in RefereeState after step 3.1). After step 3.1, the only thing it writes through the cast is `referee` — which is passed explicitly.

Change signature to take `MatchSession*` directly (it's called from setup code, not the main loop) or take an explicit `RefereeState*` parameter. This eliminates the last cast (line 1153).

### Step 3.6: Fix Remaining Const-Cast Reads

After steps 3.1-3.5, the remaining casts are reads:
- Line 1088: `homeRunContestState.runnerBatterPairCounter` — read
- Line 1092: `gameFlowState.ballHome` — read

These are reads through `const MatchSession*`, which doesn't need a cast in C. `const MatchSession*` allows reading all fields. Remove the unnecessary `((MatchSession*)game)->` and use `game->` directly.

**After Phase 3: Zero const-casts remain in referee.c.**

---

## Phase 4: Extract Pure Helpers

**Goal:** Reduce copy-paste in referee.c by extracting commonly-used formulas into named functions.

### Step 4.1: Extract get_batting_team_index()

The formula `(scoreboard->inning + scoreboard->playsFirst + scoreboard->period) % 2` appears 7 times in referee.c and 3 times in game_consolidation.c. It encodes a pesäpallo rule about team rotation. Extract to a pure function:

```c
int get_batting_team_index(const Scoreboard* sb) {
    return (sb->inning + sb->playsFirst + sb->period) % 2;
}
```

Place in `rules_pure/` (it's a pure game rule). Replace all 10+ call sites. Write a unit test that verifies the rotation logic for various inning/period combinations.

### Step 4.2: Extract should_period_end()

`halfInningState->endPeriod = 1` appears 6 times in referee.c, each with a slightly different condition involving period boundaries and score comparisons. Extract the condition-checking into a pure function:

```c
int should_period_end(const Scoreboard* sb, int battingTeamRuns, int catchingTeamRuns) { ... }
```

This is more complex than get_batting_team_index — the 6 sites have different context (normal game vs HR contest, different period boundaries). Needs careful analysis to find the common pattern. Write unit tests for each period transition.

---

## Phase 5: Test Strengthening

**Goal:** Add tests that verify the architectural contracts, not just game outcomes.

### Step 5.1: Unit Tests for Extracted Pure Functions

Once `get_batting_team_index()` and `should_period_end()` exist, write unit tests for them. These are pure functions — easy to test, high value.

### Step 5.2: Consider "Technical Cooperation" Tests

After Phase 2, the referee→consolidation contract for WOUNDED is enforced by existing integration tests. But as we add more contracts, consider focused tests that verify: "if referee sets X, does consolidation respond with Y within the same frame?" These would test the *pipeline cooperation*, not game scenarios.

This is a future consideration, not an immediate action. The existing 63 tests provide good coverage for now.

---

## What Comes After (Needs Re-evaluation)

These are known future goals from ARCHITECTURE.md and the analysis documents. **They should be re-planned after Phases 1-4 are complete**, because the codebase will look different and our understanding will be sharper.

### game_manipulation.c Decomposition (M19)

The 875-line "junk drawer" contains ball physics, player movement, fielder behavior, and rendering calls. Future extraction targets:
- Fielder behavior (rankPlayersAndMoveThem, basemenReplacements) → `fielder_behavior.c`
- Ball physics calls → already partially in `src/physics/`
- `updateModels()` → renderer layer

### common_logic.c Decomposition

The 992-line second junk drawer. Contains vector math wrappers, movement primitives, game rule logic, initialization helpers, and ball physics. Needs domain-based splitting similar to game_manipulation.

### Action & AI Decoupling (M20-M21)

Complete `actions_messy → actions_pure` split. Intent layer for replay/network support. This is the long-term architectural goal.

### Pure vs Messy Re-evaluation

The pure/messy split was done before the refactoring started. After Phases 1-4, revisit whether the boundaries still make sense, whether some "messy" code has become "pure" through refactoring, and whether better names exist.

---

## Summary Table

| Phase | Steps | Risk | Const-casts removed | Tests affected |
|-------|-------|------|-------------------|---------------|
| 1. Dead Code | 1.1-1.3 | None | 0 | 0 (cleanup only) |
| 2. WOUNDED | 2.1-2.3 | Low | 0 | Covered by test_fly_ball_double_wound |
| 3. Ownership | 3.1-3.6 | Medium | 12 → 0 | All 63 must pass after each step |
| 4. Helpers | 4.1-4.2 | Low | 0 | New unit tests added |
| 5. Tests | 5.1-5.2 | None | 0 | Tests only |

Each step is independently committable and testable. If any step breaks a test, we fix it before moving to the next step.
