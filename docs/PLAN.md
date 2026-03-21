# Refactoring Plan

**Created:** 2026-03-11
**Last updated:** 2026-03-21
**Supersedes:** `archive/FINAL_PLAN.md`, `archive/ALTERNATIVE_PLAN.md`

---

## Where We Are

Phases 1–2 of the original plan are **done**. Phase 3.1 (move `homerunPairHasPitch` into `RefereeState`) is **done**. Phases 1–2 of THIS plan are **done** (const-cast consolidation, knighting stable functions, 1-frame contract tests, test directory restructuring). The main loop in `mutable_world.c` is clean: five pipeline stages, proper comments, proper ownership. The referee takes `const StateInfo*` plus explicit writable pointers. **3 const-casts remain** in `referee.c` (down from 12).

**Test count:** 73 tests (54 unit + 4 contract + 15 scenario). All passing.

**Test structure:**
- `tests/unit/` — 54 pure function unit tests → `make test`
- `tests/integration/contracts/` — 4 one-frame pipeline contract tests → `make integration_test`
- `tests/scenario/` — 15 full-game scenario tests → `make scenario_test`

**Cleanups done in Phase 2 (2026-03-21):**
- Removed duplicate `#define`s in referee.c (BASE_RADIUS, HOME_RADIUS, HOME_LINE_Z already in globals.h)
- Connected `OUT_OF_BOUNDS_THRESHOLD` constant to its usage in referee.c
- Cleaned `initializeTemporaryGameAnalysisInfo`: removed duplicate FlowControl clears, fixed `freeWalkBase = -1` → `BASE_NONE`
- Removed redundant `GameConsolidation_Init` call from `create_scenario` in test infrastructure
- Removed GameEvents Standard violation in test helper (`gameEvents.catchMade = 0` — only `clearFrameEvents` should zero events)

**Next:** Phase 3 — GameEvents Migration (consolidate `batHit`/`batMiss`).

---

## Shared Vision

This is a pesäpallo game engine, and it's important to us. The codebase should be as beautiful as the game itself — clear, elegant, correct. We hold ourselves to the highest standard of C programming while embracing functional thinking: pure stages, immutable inputs, explicit data flow. We don't compromise unless necessary, and when we do, we discuss it first.

**Core beliefs:**
- **Data structure determines everything.** When the data model is right, functions become natural. When it's wrong, bad things cascade everywhere. Getting struct design right is the most important investment.
- **Functional purity in a C world.** We can't have Haskell's type system, but we can have its discipline. Each pipeline stage reads its inputs and produces its outputs. Side effects are explicit, ownership is enforced by `const`, and the compiler catches violations.
- **No hacks, no shortcuts.** If something feels sketchy, it's a symptom of a deeper problem. We fix the cause, not the symptom. Every line should be there for a reason.
- **Invalid states should be unrepresentable.** We prefer enums over flag collections, lifecycle-matching structs over manual clears, and helper functions that produce valid composite states over setting individual flags.
- **Naming matters.** Files, functions, variables — everything should say what it means. When names stop fitting, we rename. The code is documentation.

**Refactoring procedure:**
1. Understand what exists and why (read before writing)
2. Make a plan with small, independently testable steps
3. Run all 73 tests after every change — green is the only acceptable state
4. Knight stable interfaces with tests — this locks in progress permanently
5. Never test things that are about to change — that consolidates bad design
6. Discuss any decision that has multiple valid approaches

## Guiding Principles

*Technical principles that implement the vision above.*

1. **Data ownership is sacred.** Each pipeline stage owns specific structs. Writes must go through proper signatures, never through const-casts.
2. **Referee decides, Consolidation acts.** The referee sets legal status flags. Consolidation enforces them physically.
3. **The compiler is our enforcer.** `const` in signatures isn't documentation — it's a contract. If the code compiles without casts, ownership is correct.
4. **Small verifiable steps.** Each step must leave all 73 tests passing. No multi-step changes that can't be tested in isolation.
5. **Testing is knighting.** We don't test things that are probably going to change. We add tests to things we've thought hard about and are confident of their stability, naming, and purpose. The test suite is a monument to what's finished.
6. **Lifecycle determines structure.** Variables should live in structs that enforce their lifecycle. Transient (single-frame) data auto-clears. Persistent data survives. Mixing lifecycles creates defensive code.

---

## Lifecycle Architecture (Verified 2026-03-11)

*This section documents a deep analysis of variable lifecycles and struct ownership across the entire codebase. It captures conclusions about what patterns are sound, what should stay flexible, and where the remaining violations are.*

### The Four Lifecycle Tiers

The codebase has evolved four distinct lifetime tiers for game state. This tiering is **sound** — each tier has a natural reset mechanism and clear ownership:

| Tier | Lifetime | Struct | Reset Trigger | Owner (writes) |
|------|----------|--------|---------------|----------------|
| **Frame** | 1 frame | `GameEvents` | `clearFrameEvents()` at end of frame | Pre-referee stages |
| **Pitch** | Multiple frames | `BetweenPitchState` | Referee on `pitchReleased` event | Referee |
| **Inning** | Many pitches | `RefereeState`, `HalfInningState` | End-of-inning RESETTING state machine | Referee |
| **Game** | Entire game | `Scoreboard` | Period transitions | Consolidation |

Mixing lifecycles in a single struct creates defensive reset code. When a field's lifecycle matches its struct, the reset is automatic and correct.

### The GameEvents Standard

A field belongs in `GameEvents` if and only if **all** of these hold:

1. It answers **"what happened this frame?"** — not "what is happening" or "please do X."
2. It is **binary** (0→1 only). No code explicitly sets it to 0 except `clearFrameEvents`.
3. It is **set in pre-referee stages** (action_implementation, game_manipulation, batting_system, pitching_system).
4. It is **consumed by referee** (and optionally by post-referee code).
5. Its useful lifetime is **exactly 1 frame**.

Fields that fail any criterion do NOT belong in GameEvents, even if they feel "event-like."

### The Event → Decision → Enforcement Chain

This three-layer pattern is the heart of the architecture and is **confirmed correct**:

```
Physics Event (1-frame)  →  Referee Decision (1-pitch)  →  Consolidation Enforcement (1-frame reaction)
catchMade                →  catchHasBeenMade             →  WOUNDED status → player removed
ballHitGround            →  hasBallHitGround             →  (pitch resolution, foul detection)
ballHitByBat             →  strikes += 1                 →  (3-strike force run)
```

**Why keep the intermediate event?** The referee adds *validation*. A raw `catchMade` event becomes the legal fact `catchHasBeenMade` only after referee checks: was ball hit (batHit)? hasn't hit ground yet? first catch for this pitch? Letting pre-referee code directly set sticky flags would bypass this validation. The event layer IS the separation of concerns.

### Pre-Referee Code: Reading State Is Fine

The rule is NOT "pre-referee code must be stupid." The rule is **directional within a frame:**

1. Input reads keyStates → produces ActionFlags
2. Physics reads ActionFlags + world state → produces physical changes + GameEvents
3. Referee reads GameEvents + world state → produces legal decisions
4. Consolidation reads legal decisions → enforces physically

**Between frames, everyone reads everyone's published stable state.** This is the scoreboard principle — published decisions from the previous frame are public knowledge. The AI reading `betweenPitchState.catchHasBeenMade` to decide running strategy is natural and correct.

What pre-referee code must NOT do: write to referee-owned structs or react to *current-frame* GameEvents before referee has processed them. And it doesn't.

### What Should NOT Move to GameEvents

Analysis confirms these pRAI fields do NOT fit the GameEvents standard:

| Field | Why Not GameEvents | What It Actually Is |
|-------|-------------------|---------------------|
| `refreshCatchAndChange` | Not "what happened" but "please recompute." Set and consumed within game_manipulation (same stage). Internal scheduling flag. | Per-stage coordination |
| `batterCanAdvance` | Pitch-scoped (set at pitch release, lives until new batter). 1-frame clear would break base running. | Physics gate (pitch-tier) |
| `pitchState` | State machine (NONE→WINDUP→AIRBORNE). Persistent. | Shared state machine |
| `throwGoingToBase` | Persistent during entire throw sequence. | Action state |
| `willStartRunning[]` | Input-to-action bridge (user requested run, pending execution). | Pending action |
| `initBatter` | Set once when batter enters, consumed when animation starts (multi-frame). | Initialization trigger |
| `battingGoingOn` | Persistent during batting sequence. | Action state |
| `meterValue`/`swingMeterValue` | Continuous analog values, read only by renderer. | UI state (wrong struct) |

**`pRAI` is a grab-bag, and that's OK for now.** Trying to force everything into the clean event model would be worse than leaving it. A natural decomposition (per-stage locals, UI state extraction) can emerge later.

### Known Minor Violations (Low Priority)

These are cross-boundary writes that don't cause bugs but violate strict ownership:

1. **`game_screen.c:272`** clears `halfInningState.event = EVENT_NONE` — renderer writing to referee-owned struct. Semantically this is "I've displayed this notification, clear it." Pragmatically harmless. A purist fix would be a separate UI notification queue.

2. **`game_consolidation.c:519`** sets `halfInningState.endPeriod = 1` — consolidation making a rules decision (homerun contest: one team can't catch up). This logic belongs in referee. Low risk because it's only in homerun-contest code.

---

## The Three Tiers of Testing

### Tier 1: Unit Tests (Pure Functions)

Pure functions from `rules_pure/`, `actions_pure/`, `ai_pure/`. They take values and return values. No state, no side effects. They encode pesäpallo rules and physics formulas that won't change. These are the easiest to knight — if the function's API is stable, the test is forever.

**Currently knighted (54 tests):** `batting_physics` (5), `pitching_physics` (5), `rules_outs` (7), `rules_runs` (2), `base_logic` (3+4 new), `batting_ai_strategy` (4), `catching_ai_strategy` (4), `pitching_ai_strategy` (1), `cup_logic` (6), `collision` (5), `fixture_setup` (4), `text_width` (1), `get_base_controller` (1), `get_ball_at_base_index` (1), `get_active_batter_index` (1).

**Awaiting knighting:** `determine_pitch_result` — the strike zone logic only checks horizontal position; the API may expand to include height. Not stable enough to knight yet.

### Tier 2: 1-Frame Contract Tests (4 tests)

These test the **contracts between pipeline stages**, not game scenarios. They set a precise state, run `updateMutableWorld` for exactly 1 frame, and assert the immediate reaction. They answer questions like:

- "If `gameEvents.catchMade = 1` and runner has no safety base, does the referee set WOUNDED this frame?"
- "If referee sets `endOfInningState = RESETTING`, does consolidation act this frame?"
- "After `clearFrameEvents()`, are ALL transient fields zero?"
- "If `betweenPitchState.resolutionProcessed = 1`, does consolidation reset `pitchState`?"

These tests are fast (milliseconds), precise, and catch the subtle bugs that 1000-frame scenario tests miss — where one stage sets something but another doesn't react, or reacts one frame late.

**Infrastructure:** The scenario builder's `simulate_frames(ctx, 1)` already works. We just need to pre-set state precisely and assert on immediate results.

### Tier 3: Full Scenario Tests (Existing 15)

1000-frame simulations that test baseball rules end-to-end. These are already knights — they encode the rules of pesäpallo and verify the game plays correctly. They are the safety net.

---

## Function Signature Strategy

*Carried forward from the original plan. This is settled and correct.*

- **Referee:** `const StateInfo*` + explicit writable pointers. Zero const-casts allowed.
- **Everyone else:** `StateInfo*` (full access). They're physical-world actors.
- **Pure functions:** Take values, return values. No state pointers.

The 6-argument signature for `update_referee()` is intentional. It makes the ownership contract visible. We do not bundle these into a context struct.

---

## State Machine Pattern

*Carried forward. We do not touch these.*

The referee uses three state machines: End-of-Inning, Foul Play, HR Next Pair. Each follows: `NONE → DETECTED → (timer) → RESETTING → (consolidation acts) → NONE`. This is a clean two-phase commit protocol. The timers and state clearing belong in the referee.

### Clearing Events: End vs Beginning of Frame

Currently `clearFrameEvents()` runs at the END of the frame (`mutable_world.c:87`). Moving it to the beginning would be semantically cleaner ("start fresh" vs "clean up after yourself") and would eliminate the need for explicit GameEvents initialization during setup. However, the behavior is identical — no code reads events between frames. This is a cosmetic choice; if we touch that code, prefer clearing at the start. Not worth a dedicated change.

---

## Phase 1: Consolidate & Knight ✅ DONE

**Completed 2026-03-21.** Removed 6 unnecessary const-casts (9→3). Fixed 2 function signatures (`checkIfBallIsOutOfBounds`, `update_game_state_flags`) to take `const` pointers. Added 15 new unit tests for `base_logic`, `get_base_controller`, `get_ball_at_base_index`, `get_active_batter_index`, fixture setup, collision, and other stable functions. All tests passing.

<details>
<summary>Original plan (for reference)</summary>

### Step 1.1: Remove Unnecessary Read Casts (4 trivial fixes)

Four casts in `referee.c` cast `const MatchSession*` to `MatchSession*` for read-only access. In C, `const T*` already allows reading all fields. These casts are unnecessary — just delete the `(MatchSession*)` wrapper.

- **Line 92:** `get_base_controller((MatchSession*)game, ...)` → `get_base_controller(game, ...)` (function already takes `const MatchSession*` — verified in `base_control.h:18`)
- **Line 981:** `((MatchSession*)game)->gameFlowState.ballHome` → `game->gameFlowState.ballHome`
- **Line 1120:** `((MatchSession*)game)->homeRunContestState.runnerBatterPairCounter` → `game->homeRunContestState.runnerBatterPairCounter`
- **Line 1125:** `((MatchSession*)game)->gameFlowState.ballHome` → `game->gameFlowState.ballHome`

These are pure deletions — no logic changes, no signature changes. The compiler will verify correctness. Run all 54 tests as a sanity check.

### Step 1.2: Fix Two Function Signatures (2 easy fixes)

Two functions take non-const pointers but only read through them:

- **`checkIfBallIsOutOfBounds`** in `base_logic.c/.h` (line 84): change `BallInfo*` → `const BallInfo*` and `FieldPositions*` → `const FieldPositions*`. This eliminates the cast at referee.c line 187. Verify no callers rely on mutation.
- **`update_game_state_flags`** (static in `referee.c`, line 877): change `StateInfo*` → `const StateInfo*`. This function doesn't even use `stateInfo` — it only reads `events->catchMade` and writes to `betweenPitchState`. The cast at line 950 becomes unnecessary. Compiler enforces correctness.

Run all 54 tests.

### Step 1.3: Knight Stable Pure Functions

Add unit tests for:

- **`get_base_controller()`** — core domain logic: who controls a base when multiple runners share it. Test: no controller, single controller, multiple candidates (pick by `baseAtPitchStart`), invalid base ID.
- **`get_active_batter_index()`** — simple linear search. Test: batter at various indices, no batter, NULL game.
- **`get_ball_at_base_index()`** — companion to get_base_controller. Test similarly.
- **Remaining `base_logic` functions** — `base_to_int_index`, `player_is_safe_from_fly`, `count_active_batting_players`.

Follow existing pattern: extern declarations in `test_runner.c`, `RUN_TEST()` macros, return `TEST_PASSED`/`TEST_FAILED`.

**After Phase 1:** 6 of 9 const-casts eliminated. ~6-10 new unit tests added. Clean position.

</details>

---

## Phase 2: 1-Frame Contract Tests ✅ DONE

**Completed 2026-03-21.** Created 4 contract tests proving pipeline stage cooperation. Restructured test directory: scenario tests moved to `tests/scenario/`, contract tests in `tests/integration/contracts/`. Three Makefile targets: `make test`, `make integration_test`, `make scenario_test`. Cleaned up initialization code and test infrastructure. All 73 tests passing.

**Contract tests written:**
1. `test_clear_frame_events` — verifies ALL GameEvents fields clear + size guard catches struct growth
2. `test_referee_reacts_to_catch` — catchMade + batHit + runner → woundingEvaluationActive + WOUND_MARKED
3. `test_referee_reacts_to_pitch` — pitchReleased → baseAtPitchStart captured, strikes saved, BPS cleared
4. `test_foul_detection` — ballHitGround + batHit + out of bounds → foulState = DETECTED

<details>
<summary>Original plan (for reference)</summary>

### Step 2.1: Infrastructure

Determine the minimal setup needed for a 1-frame contract test. Likely:
- `create_scenario()` → place players → `initialize_referee_from_physical_state()` → manually set the specific state being tested → `simulate_frames(ctx, 1)` → assert.

The key difference from scenario tests: we don't simulate a baseball play. We set a precise pre-condition and verify a precise post-condition within one pipeline pass.

### Step 2.2: Write Initial Contract Tests

Start with 3-4 tests that prove existing architecture:

1. **`test_clearFrameEvents_completeness`** — Set all GameEvents fields to 1. Call `clearFrameEvents`. Assert all are 0. (This is actually a unit test and could be done in Phase 1, but it validates the lifecycle mechanism that Phase 2 builds on. Place it wherever feels natural.)

2. **`test_referee_wounded_on_catch`** — Set up a runner with no safety base, set `gameEvents.catchMade = 1`, `betweenPitchState.catchHasBeenMade = 0`. Run 1 frame. Assert referee sets player status to WOUNDED.

3. **`test_consolidation_reacts_to_end_of_inning`** — Set `referee.endOfInningState = END_INNING_STATE_RESETTING`. Run 1 frame. Assert consolidation resets the physical world (new batting order, positions reset).

4. **`test_consolidation_enforces_out`** — Set a player's referee status to OUT. Run 1 frame. Assert consolidation physically removes the player.

### Step 2.3: Decide Where These Live

Options:
- **In `tests/integration/`** alongside scenario tests — they use the same infrastructure.
- **In a new `tests/contract/`** directory — clearer separation.

Recommendation: put them in `tests/integration/` since they share the scenario builder, but name them `test_contract_*.c` to distinguish them from scenario tests.

**After Phase 2:** New testing capability. ~4 contract tests that prove the architecture. Foundation for safe structural moves.

</details>

---

## Phase 3: The GameEvents Migration (Big Win) ← NEXT

**Goal:** Move truly transient pRAI fields into `GameEvents`, delete defensive reset logic. The architecture enforces the lifecycle automatically.

**Getting started:** This phase has clear, mechanical steps. The duplicate fields are already identified (see Step 3.1 below). A good contract test to write alongside the migration: assert that after `clearFrameEvents`, `batHit`/`batMiss` (now in GameEvents) are zero, and that referee still correctly evaluates wounding/foul using the GameEvents versions.

### Critical Finding: Not All Fields Are Truly Transient

Our analysis revealed that only **2 of 4** proposed fields are safe to move:

| Field | Truly Transient? | Safe to Move? |
|-------|-----------------|---------------|
| `batHit` | ✅ Yes — set on collision, used same frame | ✅ Yes |
| `batMiss` | ✅ Yes — set on miss, used same frame | ✅ Yes |
| `batterCanAdvance` | ❌ No — persists across entire pitch | ❌ Not yet |
| `refreshCatchAndChange` | ❌ No — controls frame-skip logic 1-2 frames | ❌ Not yet |

`batterCanAdvance` stays 1 from pitch release until new batter selection. It gates `runToNextBase()`. Moving it to GameEvents (frame-cleared) would break base running.

`refreshCatchAndChange` prevents catch evaluation while AI ranking is updating. It lives 1-2 frames. Moving it to GameEvents would cause race conditions.

### Step 3.1: Move batHit and batMiss to GameEvents

**Important discovery:** `gameEvents.ballHitByBat` and `gameEvents.ballMissedByBat` ALREADY EXIST as partial duplicates of `pRAI.batHit`/`pRAI.batMiss`. In `batting_system.c`, both pairs are set in the same code paths:
- Line 455: `pRAI.batHit = 1` AND line 475: `gameEvents.ballHitByBat = 1`
- Line 429/481: `pRAI.batMiss = 1` AND line 483: `gameEvents.ballMissedByBat = 1`

The GameEvents versions currently drive only strike counting (`update_strikes` reads `events->ballHitByBat || events->ballMissedByBat`). The pRAI versions drive foul play detection, wounding logic, and pitch resolution.

**The migration should CONSOLIDATE, not add new fields.** Redirect the referee's foul/wounding/pitch reads from `pRAI.batHit`/`pRAI.batMiss` to the existing `gameEvents.ballHitByBat`/`gameEvents.ballMissedByBat` (or rename them to `batHit`/`batMiss` in GameEvents for consistency). Then delete the pRAI fields and their manual clears.

Verified safe by tracing all references:

**`batHit`** (7 references):
- SET: `batting_system.c:455` (bat contacts ball)
- READ: `referee.c:186, 210` (foul play + wounding logic), `batting_system.c:406` (guard against double-processing), `batting_ai.c:398` (AI decision)
- CLEARED: `batting_system.c:323` (pitch start — redundant if in GameEvents), `common_logic.c:855` (`initializePRAIInformation` — redundant)
- Lifecycle: set and consumed within single frame. Frame-clear is safe.

**`batMiss`** (5 references):
- SET: `batting_system.c:429, 481` (swing misses)
- READ: `referee.c:698` (`determine_pitch_result`), `batting_system.c:407` (guard against double-processing)
- CLEARED: `batting_system.c:324` (pitch start — redundant), `common_logic.c:856` (redundant)
- Lifecycle: set and consumed within single frame. Frame-clear is safe.

**Changes:**
- Decide naming: keep `ballHitByBat`/`ballMissedByBat` (already in GameEvents) or rename to `batHit`/`batMiss` for brevity. Either works; the GameEvents fields already exist and are already cleared by `clearFrameEvents`.
- Change all `match->pRAI.batHit` → `match->gameEvents.ballHitByBat` (or `batHit` if renamed) and same for `batMiss`
- Remove `batHit` and `batMiss` from `PlayerRelatedActionInfo` in `globals.h`
- Delete the duplicate SET in `batting_system.c` (lines 455 and 475 both set "bat hit" — after consolidation, only one SET per event)
- Delete the manual clears: `batting_system.c:323-324` and `common_logic.c:855-856` (GameEvents auto-clears handle this)
- Update `state_validator.c:193` reference
- Run all tests. Optionally write a 1-frame contract test proving they clear.

### Step 3.2: Delete Redundant Reset Logic

After step 3.1, review `initializePRAIInformation()` — the `batHit = 0` and `batMiss = 0` lines are gone. Check if other fields cleared there are also now redundant.

**NOTE (2026-03-21):** `initializeTemporaryGameAnalysisInfo()` duplicate clears were already cleaned up in Phase 2 (removed duplicate FlowControl block, fixed `freeWalkBase = -1` → `BASE_NONE`). Verify no new duplicates have crept in.

### Step 3.3: Investigate batterCanAdvance and refreshCatchAndChange

These need deeper analysis before moving. See "What Should NOT Move to GameEvents" in the Lifecycle Architecture section above for the full rationale. Summary:

- **`batterCanAdvance`:** Pitch-scoped, not frame-scoped. Lives from pitch release until new batter enters. Moving to GameEvents would break base running (`runToNextBase` gate in `common_logic.c:256`). Possible replacement: derive from pitch state (`pitchState != PITCH_STAGE_NONE` or a new BetweenPitchState field). But this changes semantics — `batterCanAdvance` is currently reset at batter entry, not pitch start. Needs investigation.
- **`refreshCatchAndChange`:** Not a game event at all — it's internal scheduling within game_manipulation ("please recompute fielder rankings"). Set and consumed within the same stage. Moving to GameEvents would be semantically wrong. Better approaches: always run the ranking (eliminate the flag), or make it local state within game_manipulation. Low priority.

These are **future work** — document findings but don't block on them.

**After Phase 3:** ~30-50 lines of defensive code deleted. `batHit`/`batMiss` lifecycle is self-enforcing. Visible, satisfying diff.

---

## Phase 4: Complete Referee Ownership (Zero Const-Casts)

**Goal:** Eliminate the remaining 3 const-casts. The compiler becomes our enforcer.

### Step 4.1: Move Free Walk Reset to Consolidation

Referee's `update_pitch_resolution()` (line 689) takes `FlowControl*` to reset `freeWalkCalculationMade`, `freeWalkIndex`, `freeWalkBase` when a ball is counted (lines 708-710). The referee's job is counting the ball. The *consequence* (recalculating free walk eligibility) belongs in consolidation.

Consolidation already reacts to `resolutionProcessed` at `game_consolidation.c:119-122`:
```c
if (game->betweenPitchState.resolutionProcessed) {
    game->pRAI.pitchState = PITCH_STAGE_NONE;
    game->betweenPitchState.resolutionProcessed = 0;
}
```
Add the free walk reset here:
```c
if (game->betweenPitchState.resolutionProcessed) {
    game->pRAI.pitchState = PITCH_STAGE_NONE;
    game->flowControl.freeWalkCalculationMade = 0;
    game->flowControl.freeWalkIndex = -1;
    game->flowControl.freeWalkBase = BASE_NONE;
    game->betweenPitchState.resolutionProcessed = 0;
}
```
Then remove lines 707-710 from `update_pitch_resolution()` in referee.c and remove the `FlowControl*` parameter from its signature. This eliminates the `(FlowControl*)` cast at line 942.

### Step 4.2: Move waitingForBatterDecision=0 to Consolidation

Line 1134 in referee.c: when the referee detects end-of-inning (`shouldEndInning == true`), it sets `waitingForBatterDecision = 0` to cancel any pending batter prompt. This is a *reaction* to the referee's decision, not the decision itself.

The referee already sets `endOfInningState = END_INNING_STATE_DETECTED` (line 1131) and `halfInningState->event = EVENT_INNING_ENDING` (line 1135) in the same block. Consolidation can check either of these and cancel the batter prompt:

In consolidation's batter-decision logic (likely `checkIfNextBatterDecision` or equivalent), add a guard:
```c
if (game->referee.endOfInningState != END_INNING_STATE_NONE) {
    game->flowControl.waitingForBatterDecision = 0;
    return;
}
```
Then remove line 1134 from referee.c. This eliminates the `(FlowControl*)` cast.

### Step 4.3: Fix initialize_referee Signature

Line 1186: `initialize_referee()` takes `const StateInfo*` but needs to write to `game->referee` (its own state). Since this is setup code called before the main loop (not during the pipeline), the const restriction is artificial.

Change signature from `void initialize_referee(const StateInfo* stateInfo)` to either:
- `void initialize_referee(StateInfo* stateInfo)` (simplest — setup code doesn't need const protection), or
- `void initialize_referee(const StateInfo* stateInfo, RefereeState* referee)` (explicit writable pointer, consistent with `update_referee` pattern)

The second option is more consistent with our ownership philosophy but adds a parameter to a function called from 2 places. Either works. This eliminates the last `(MatchSession*)` cast.

Update the declaration in `referee.h:23` to match.

### Step 4.4: Verify Zero Casts

`grep -n "(MatchSession\*)\|(FlowControl\*)\|(BallInfo\*)" src/game/referee.c` returns nothing.

**After Phase 4: Zero const-casts in referee.c.** The type system enforces data ownership. Phase 3 from the original plan is complete.

---

## Phase 5: Extract Pure Helpers

**Goal:** Reduce duplication by extracting commonly-used formulas into tested pure functions.

### Step 5.1: Extract get_batting_team_index()

The formula `(scoreboard->inning + scoreboard->playsFirst + scoreboard->period) % 2` appears **10 times** (7 in `referee.c`, 3 in `common_logic.c`). Extract to `rules_pure/`:

```c
int get_batting_team_index(const Scoreboard* sb) {
    return (sb->inning + sb->playsFirst + sb->period) % 2;
}
```

Replace all call sites. Write unit tests for various inning/period combinations.

### Step 5.2: Extract should_period_end()

`halfInningState->endPeriod = 1` is set in 6 places in `referee.c` (lines 660, 666, 672, 750, 780, 786), each with different conditions involving period boundaries and score comparisons. Extract the condition-checking into a pure function. This is more complex — the 6 sites have different context (normal game vs HR contest, different period boundaries). Needs careful analysis.

### Step 5.3: Evaluate Further Extractions

After 5.1-5.2, review `common_logic.c` for more extraction candidates. The vector math wrappers (`setVectorXYZ`, `addToVectorXZ`, etc.) and movement primitives may benefit from being in a dedicated file, though they may not be "pure" in the strict sense.

**After Phase 5:** Less duplication in referee.c. New tested pure functions. Cleaner code.

---

## Future Work (Re-evaluate After Phase 5)

These are known goals that should be re-planned after the above phases are complete.

### Rename & Reorganize

- `mutable_world.c` → `game_frame.c` or `game_loop.c` (the function is really `runGameFrame`)
- Do this **after** active refactoring stabilizes to avoid merge pain

### common_logic.c Decomposition

959 lines, 32 functions. Contains vector math, movement primitives, initialization helpers, and game logic. Split by domain:
- Vector/movement utilities → `movement.c` or `player_movement.c`
- Initialization functions → evaluate which survive lifecycle sorting

### game_manipulation.c Decomposition

904 lines. Contains ball physics, fielder AI, base runner logic, rendering calls. Future extraction:
- Fielder behavior → `fielder_behavior.c`
- `updateModels()` → renderer layer

### pRAI Lifecycle Completion

After `batHit`/`batMiss` are consolidated into GameEvents, the remaining pRAI fields sort into clear categories (see Lifecycle Architecture section):

- **UI state** (move to `UIState`): `meterValue`, `swingMeterValue` — only read by `game_screen.c`
- **Pitch-scoped gates** (investigate `BetweenPitchState` or derived state): `batterCanAdvance`
- **Internal stage coordination** (leave or make local): `refreshCatchAndChange`, `initPlayerSelection`
- **Action state** (stays in pRAI or `PendingActionState`): `pitchState`, `throwGoingToBase`, `battingGoingOn`, `batterReady`, `initBatter`, `willStartRunning[]`

Goal: pRAI contains only persistent action/physics state. No transient events, no UI values.

### Minor Ownership Fixes

- `halfInningState.event` clearing in `game_screen.c:272` — consider a separate UI notification field or accept the pragmatic violation
- `halfInningState.endPeriod = 1` in `game_consolidation.c:519` — move this homerun catch-up logic to referee

### Action & AI Decoupling

Complete `actions_messy → actions_pure` split. Intent layer for replay/network support.

---

## Summary Table

| Phase | Steps | Risk | Key Metric | Status |
|-------|-------|------|-----------|--------|
| **1. Consolidate & Knight** | 1.1-1.3 | None | 9→3 const-casts, +15 unit tests | ✅ Done |
| **2. 1-Frame Contracts** | 2.1-2.3 | None | +4 contract tests, test restructuring | ✅ Done |
| **3. GameEvents Migration** | 3.1-3.3 | Low | -30-50 lines of defensive code | 🎯 NEXT |
| **4. Referee Ownership** | 4.1-4.4 | Medium | 3→0 const-casts | ⏳ TODO |
| **5. Pure Helpers** | 5.1-5.3 | Low | -30+ lines of duplication | ⏳ TODO |

Each step is independently committable and testable. If any step breaks a test, we fix it before moving on.
