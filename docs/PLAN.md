# Refactoring Plan

**Created:** 2026-03-11
**Last updated:** 2026-04-10
**Supersedes:** `archive/FINAL_PLAN.md`, `archive/ALTERNATIVE_PLAN.md`
**Architectural target:** `OPUS_VISION.md`

---

## Where We Are

Phases 1–3 are **done**. The main loop in `mutable_world.c` is clean: five pipeline stages,
proper comments, proper ownership. The referee takes `const StateInfo*` plus explicit writable
pointers. **3 const-casts remain** in `referee.c` (down from 12). Phase 3 consolidated
`batHit`/`batMiss` into the event→sticky pattern.

**Test count:** 73 tests (54 unit + 4 contract + 15 scenario). All passing.

**Test structure:**
- `tests/unit/` — 54 pure function unit tests → `make test`
- `tests/integration/contracts/` — 4 one-frame pipeline contract tests → `make integration_test`
- `tests/scenario/` — 15 full-game scenario tests → `make scenario_test`

**Known Bug #1:** `resolve_pending_runs()` (referee.c:798-879) scores runs at 3 sites
(lines 818, 832, 858) but has ZERO `endPeriod` checks. Every other scoring path
(`update_runs`, `update_free_walk_resolution`) checks and sets `endPeriod`. This means
pending runs that resolve after the period should have ended are incorrectly scored.
Fix is in Phase 6.

**Known dead field:** `HalfInningState.outOfBounds` (globals.h:499) is never written to.
Foul tracking moved to `referee.foulState` state machine but the field was not removed.
Cleanup is in Phase 7.

**Next:** Knight Phase 3 → Phase 4 → Phase 5 → Phase 6 → Phase 7 → Phase 8.

<details>
<summary>Phase 2 cleanups (2026-03-21)</summary>

- Removed duplicate `#define`s in referee.c (BASE_RADIUS, HOME_RADIUS, HOME_LINE_Z already in globals.h)
- Connected `OUT_OF_BOUNDS_THRESHOLD` constant to its usage in referee.c
- Cleaned `initializeTemporaryGameAnalysisInfo`: removed duplicate FlowControl clears, fixed `freeWalkBase = -1` → `BASE_NONE`
- Removed redundant `GameConsolidation_Init` call from `create_scenario` in test infrastructure
- Removed GameEvents Standard violation in test helper (`gameEvents.catchMade = 0` — only `clearFrameEvents` should zero events)
</details>

<details>
<summary>Phase 3 cleanups (2026-03-22)</summary>

- Consolidated `pRAI.batHit`/`pRAI.batMiss` into `BetweenPitchState.batOutcome` (`BatOutcome` enum: NONE/HIT/MISSED)
- Fixed `gameEvents.ballHitByBat` semantics: now only fires on actual bat contact (was incorrectly firing on vertical-angle misses too)
- Referee promotes `ballHitByBat`/`ballMissedByBat` events to sticky `batOutcome` flag (same pattern as `catchMade→catchHasBeenMade`)
- Added `clearBetweenPitchState()` static helper in referee.c (replaces 4 identical field-by-field reset blocks)
- Removed `batHit`/`batMiss` fields from `PlayerRelatedActionInfo` in globals.h
- Deleted manual clears from `batting_system.c` and `initializePRAIInformation()`
- Updated contract tests: foul/catch tests use `batOutcome`, pitch test verifies `batOutcome` reset
- Added `betweenPitchState` section to state_validator debug dump
</details>

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
ballHitByBat             →  batOutcome = HIT             →  (foul detection, wounding, strike count)
ballMissedByBat          →  batOutcome = MISSED          →  (pitch resolution, strike count)
```

**Why keep the intermediate event?** The referee adds *validation*. A raw `catchMade` event becomes the legal fact `catchHasBeenMade` only after referee checks: was ball hit (batHit)? hasn't hit ground yet? first catch for this pitch? Letting pre-referee code directly set sticky flags would bypass this validation. The event layer IS the separation of concerns.

### Pre-Referee Code: Reading State Is Fine

The rule is NOT "pre-referee code must be stupid." The rule is **directional within a frame:**

1. Input reads keyStates → produces ActionFlags
2. Physics reads ActionFlags + world state → produces physical changes + GameEvents
3. Referee reads GameEvents + world state → produces legal decisions
4. Consolidation reads legal decisions → enforces physically

**Between frames, everyone reads everyone's published stable state.** This is the scoreboard principle — published decisions from the previous frame are public knowledge. The AI reading `betweenPitchState.catchHasBeenMade` to decide running strategy is natural and correct. The batting guard reading `betweenPitchState.batOutcome` to prevent double-processing is the same principle — it reads the referee's published decision from the previous frame.

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

## Phase 3: The GameEvents Migration (Big Win) ✅ DONE

**Completed 2026-03-22.** Consolidated `pRAI.batHit`/`pRAI.batMiss` into `BetweenPitchState.batOutcome` using the event→sticky pattern. Fixed `ballHitByBat` event semantics (was incorrectly firing on vertical-angle misses). All 73 tests passing.

**Key design decisions:**
- **`BatOutcome` enum** over two boolean flags — "invalid states should be unrepresentable" (hit+miss simultaneously is impossible)
- **Event→sticky promotion** by referee — same pattern as `catchMade→catchHasBeenMade` and `ballHitGround→hasBallHitGround`
- **Guard timing is a feature** — batting_system reads sticky flag before referee promotes, which correctly allows processing on the hit frame and blocks re-entry from next frame
- **`clearBetweenPitchState()`** static helper in referee.c — replaces 4 copy-paste reset blocks (the 5th caller in common_logic.c clears inline since it's initialization code)

<details>
<summary>Original plan (for reference)</summary>

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

</details>

### Knight Phase 3: Contract Test for batOutcome Promotion

**Status: TODO (can be done at any point — see also Phase 8 Step 8.6)**

Phase 3 introduced the `batOutcome` event→sticky pattern. The existing foul and catch contract tests now set `betweenPitchState.batOutcome` directly as a precondition, but no test verifies the promotion chain itself — that `ballHitByBat` / `ballMissedByBat` events cause the referee to set `batOutcome`.

Write `test_bat_outcome_promotion` in `tests/integration/contracts/`:

1. **HIT promotion** — Set `gameEvents.ballHitByBat = 1`, run 1 frame, assert `betweenPitchState.batOutcome == BAT_OUTCOME_HIT`.
2. **MISSED promotion** — Set `gameEvents.ballMissedByBat = 1`, run 1 frame, assert `betweenPitchState.batOutcome == BAT_OUTCOME_MISSED`.
3. **Reset on pitchReleased** — Dirty `batOutcome = BAT_OUTCOME_HIT`, fire `pitchReleased`, run 1 frame, assert `batOutcome == BAT_OUTCOME_NONE`.

This knights the new pattern permanently. Also add a `sizeof(BetweenPitchState)` guard
(parallel to the `sizeof(GameEvents)` guard in `test_clear_frame_events`) so new BPS
fields force a review of `clearBetweenPitchState()`.

---

## Phase 4: Complete Referee Ownership (Zero Const-Casts)

**Goal:** Eliminate the remaining 3 const-casts. The compiler becomes our enforcer.

**Verified 2026-04-10:** All three casts located, removal paths confirmed safe. No other
non-owned writes exist in referee.c beyond these three.

### Step 4.1: Fix initialize_referee Signature

**Risk: ZERO.** Pure signature change.

Line 1191: `initialize_referee()` takes `const StateInfo*` but needs to write to
`game->referee` via `&((MatchSession*)game)->referee`. Change signature to pass explicit
writable pointer:

```c
// Before:
void initialize_referee(const StateInfo* stateInfo)
// After:
void initialize_referee(const StateInfo* stateInfo, RefereeState* referee)
```

Update callers:
- `game_screen.c:477` → `initialize_referee(stateInfo, &stateInfo->match->referee)`

Update `referee.h` declaration. Run all 73 tests.

### Step 4.2: Move waitingForBatterDecision=0 to Consolidation

**Risk: VERY LOW.** One-line move.

Line 1141: referee sets `waitingForBatterDecision = 0` when it detects end-of-inning.
The referee already signals via `endOfInningState = END_INNING_STATE_DETECTED`. Move the
consequence to consolidation:

In consolidation's batter-decision logic, add a guard:
```c
if (game->referee.endOfInningState != END_INNING_STATE_NONE) {
    game->flowControl.waitingForBatterDecision = 0;
    return;
}
```
Remove line 1141 from referee.c. This eliminates one `(FlowControl*)` cast.

### Step 4.3: Move Free Walk Reset to Consolidation

**Risk: LOW.** Three-field move, same trigger condition.

Referee's `update_pitch_resolution()` (line 955) takes `FlowControl*` to reset
`freeWalkCalculationMade`, `freeWalkIndex`, `freeWalkBase` after pitch resolution.
Consolidation already reacts to `resolutionProcessed` at line 119-122:

```c
if (game->betweenPitchState.resolutionProcessed) {
    game->pRAI.pitchState = PITCH_STAGE_NONE;
    // Add here:
    game->flowControl.freeWalkCalculationMade = 0;
    game->flowControl.freeWalkIndex = -1;
    game->flowControl.freeWalkBase = BASE_NONE;
    game->betweenPitchState.resolutionProcessed = 0;
}
```

Remove the three writes from `update_pitch_resolution()` and remove the `FlowControl*`
parameter from its signature. This eliminates the last `(FlowControl*)` cast.

### Step 4.4: Verify Zero Casts

```bash
grep -n "(MatchSession\*)\|(FlowControl\*)\|(BallInfo\*)" src/game/referee.c
```
Returns nothing. Run all 73 tests.

**After Phase 4: Zero const-casts in referee.c.** The type system enforces data ownership.

---

## Phase 5: Extract get_batting_team_index()

**Goal:** Replace 10 copies of the batting-team formula with a tested pure function.

### Step 5.1: Create scoring_helpers.c

Create `src/game/rules_pure/scoring_helpers.c` and `scoring_helpers.h`:

```c
int get_batting_team_index(const Scoreboard* sb) {
    return (sb->inning + sb->playsFirst + sb->period) % 2;
}
```

Add to Makefile's `OBJ_MAIN` list.

### Step 5.2: Replace All 10 Call Sites

**referee.c** (7 sites): lines 616, 627, 654, 731, 817, 831, 857
**common_logic.c** (3 sites): lines 634, 824, 863
**game_consolidation.c** (1 site): line 511

Each local `int battingTeamIndex = (scoreboard->inning + ...)` becomes
`int battingTeamIndex = get_batting_team_index(scoreboard)`.

### Step 5.3: Write Unit Tests

Test: period 0 inning 0, period 0 inning 1, period 1, HR contest (period 4+), playsFirst
toggle. Follow existing pattern in `test_runner.c`.

Run all 73 tests + new unit tests.

**After Phase 5:** One formula, one name, tested. Referee.c shorter and less noisy.

---

## Phase 6: Bug Fix + Period Logic

**Goal:** Fix Bug #1 (pending runs ignore endPeriod) by extracting `should_period_end()`
as a pure function and adding it to the missing locations.

### Step 6.1: Extract should_period_end()

Create pure function in `rules_pure/scoring_helpers.c`:

```c
int should_period_end(const Scoreboard* sb, int batting_runs, int catching_runs,
                      int batting_period0_runs, int catching_period0_runs) {
    if (sb->period < 4) {
        if ((sb->inning + 1) % sb->halfInningsInPeriod == 0 ||
            sb->inning + 1 == sb->halfInningsInPeriod * 2 + 2) {
            if (batting_runs > catching_runs) return 1;
            if (sb->inning + 1 == sb->halfInningsInPeriod * 2 &&
                batting_period0_runs > catching_period0_runs &&
                catching_runs == batting_runs) return 1;
        }
    } else {
        if ((sb->inning + 1) % 2 == 0) {
            if (batting_runs > catching_runs) return 1;
        }
    }
    return 0;
}
```

Write unit tests for normal game, super inning, HR contest.

### Step 6.2: Replace Existing Period-End Checks

Replace inline period-end logic in:
- `update_runs()` (referee.c:653-678) → call `should_period_end()`
- `update_free_walk_resolution()` (referee.c:781-792) → call `should_period_end()`

Run all 73 tests. Behavior must be identical — this is a pure extraction.

### Step 6.3: Fix Bug #1 — Add endPeriod Checks to resolve_pending_runs

**The bug:** `resolve_pending_runs()` (referee.c:798-879) scores at three sites (lines
818, 832, 858) without checking or setting `endPeriod`. Every other scoring path does.

**The fix:** After each scoring block in `resolve_pending_runs()`, add the
`should_period_end()` call and set `halfInningState->endPeriod = 1` when true.

Optionally: write a scenario test first that demonstrates the bug (period should have
ended but pending run was scored anyway), then apply the fix and verify the test passes.

### Step 6.4: Move endPeriod Write from Consolidation to Referee

`game_consolidation.c:519` sets `halfInningState.endPeriod = 1` for HR contest
early-termination (catching team too far ahead). Now that `should_period_end()` exists,
move this logic into the referee — it's a rules decision, not an enforcement action.

The referee can check this condition after each HR contest pair scoring and set endPeriod
directly via its owned pointer.

Run all 73 tests.

**After Phase 6:** Bug #1 fixed. `endPeriod` exclusively written by referee. Period-end
logic is a single tested pure function.

---

## Phase 7: Initialization Unification

**Goal:** Eliminate the dual-initialization problem. Each field cleared by exactly one owner.
No copy-paste init sequences. Reset recipes document intent.

**Architectural target:** See `OPUS_VISION.md` Section VI for the full vision.

### The Problem Today

`initializeTemporaryGameAnalysisInfo()` (common_logic.c:771-805) crosses ALL ownership
boundaries: it clears FlowControl (consolidation-owned), BetweenPitchState (referee-owned
at runtime), HalfInningState.event/endPeriod (referee-owned), camera, and subsystems.

The referee ALSO clears BPS and HIS fields at RESETTING transitions. Result: fields are
cleared in two places and it's unclear which clearing is authoritative.

Additionally, 7-function init sequences are copy-pasted in 3 places:
- `executeFoulPlayTeleport()` (consolidation:150-156)
- `checkIfNextPair()` (consolidation:522-530)
- `loadMutableWorldSettings()` (common_logic:924-940) — the superset

### Step 7.1: Split initializeTemporaryGameAnalysisInfo() by Ownership

Create `resetFlowState()` with ONLY consolidation-owned fields:

```c
void resetFlowState(MatchSession* match) {
    match->flowControl.pause = 0;
    match->flowControl.waitingForBatterDecision = 0;
    match->flowControl.waitingForFreeWalkDecision = 0;
    match->flowControl.freeWalkCalculationMade = 1;
    match->flowControl.freeWalkIndex = -1;
    match->flowControl.freeWalkBase = BASE_NONE;
    match->playerCounters.noMorePlayers = 0;
    match->gameFlowState.ballHome = 0;
    GameConsolidation_Init(&(match->gameFlowState));
    initGameManipulation(&(match->gameFlowState));
    match->cameraState.homeRunCameraFlag = 0;
    match->cameraState.targetPoint.x = 0.0f;
    match->cameraState.targetPoint.y = 0.0f;
    match->cameraState.targetPoint.z = 0.0f;
    clearFrameEvents(&match->gameEvents);
}
```

Remove the referee-owned fields (BPS, HIS.event, HIS.endPeriod) from this function.
Those will be handled by the referee's own reset API (Step 7.2).

Replace `initializeTemporaryGameAnalysisInfo()` calls with `resetFlowState()`.

### Step 7.2: Create Referee_ResetForNewInning()

New public API in referee.c that clears ALL referee-owned state:

```c
void Referee_ResetForNewInning(
    RefereeState* ref, HalfInningState* his, BetweenPitchState* bps)
{
    initializeRefereeState(ref);  // existing: clears all player tracking + state machines

    his->outs = 0;
    his->balls = 0;
    his->strikes = 0;
    his->runsInTheInning = 0;
    his->event = EVENT_NONE;
    his->endPeriod = 0;

    clearBetweenPitchState(bps);
}
```

This absorbs the HIS fields from `initializeCriticalGameInfo()` (common_logic.c:822-833)
that were crossing the ownership boundary. The remaining fields in that function
(`playerCounters`, `batterSelectionIndex`) stay in a team setup helper.

**Note:** `clearBetweenPitchState()` is currently `static` in referee.c. Either make it
non-static or inline the clearing into `Referee_ResetForNewInning()`. The referee's
RESETTING transitions can continue to call it directly.

### Step 7.3: Create resetPhysicalWorld() Building Block

Extract the shared 7-function sequence into one function:

```c
void resetPhysicalWorld(StateInfo* stateInfo, unsigned int* rng_seed) {
    MatchSession* game = stateInfo->match;
    initializeBallInfo(game);
    initializeActionInfo(game);
    initializeIndexInformation(game);
    initializePRAIInformation(game);
    initializeSpatialPlayerInformation(game, stateInfo->fieldPositions, rng_seed);
    initializeNonCriticalPlayerInformation(game);
}
```

**Rule: This function does NOT call `resetFlowState()` or touch any referee state.**

### Step 7.4: Create Reset Recipes

Four named recipes, each composing the building blocks. These replace the scattered init
calls in consolidation and game_screen. Place in a new `game_reset.c` file.

```c
// Recipe 1: Full reset for new half-inning
void resetForNewHalfInning(StateInfo* stateInfo, unsigned int* rng_seed) {
    resetPhysicalWorld(stateInfo, rng_seed);
    resetFlowState(stateInfo->match);
    Referee_ResetForNewInning(&match->referee, &match->halfInningState, &match->betweenPitchState);
    // Team setup (remaining fields from initializeCriticalGameInfo):
    initializeTeamForInning(stateInfo);  // playerCounters, batterSelectionIndex
    initializeInningPermanentPlayerInformation(...);
    if (scoreboard->period >= 4) setupHomerunPhysicalState(...);
}

// Recipe 2: Foul play — referee already restored legal state from snapshot
void resetForFoulPlay(StateInfo* stateInfo, unsigned int* rng_seed) {
    resetPhysicalWorld(stateInfo, rng_seed);
    resetFlowState(stateInfo->match);
    restorePlayersToRefereePositions(stateInfo);  // extracted from current executeFoulPlayTeleport
    if (scoreboard->period >= 4) setupHomerunPhysicalState(...);
}

// Recipe 3: Next HR pair — referee already cleared per-pair state
void resetForNextPair(StateInfo* stateInfo, unsigned int* rng_seed) {
    resetPhysicalWorld(stateInfo, rng_seed);
    resetFlowState(stateInfo->match);
    setupHomerunPhysicalState(...);
}

// Recipe 4: From menu — full reset + explicit referee scan
void initializeGameFromMenu(StateInfo* stateInfo, unsigned int* rng_seed) {
    resetForNewHalfInning(stateInfo, rng_seed);
    initialize_referee(stateInfo, &stateInfo->match->referee);
}
```

Replace:
- `loadGameScreenSettings()` body → calls `initializeGameFromMenu()`
- `checkIfEndOfInning()` reset path → calls `resetForNewHalfInning()`
- `executeFoulPlayTeleport()` → calls `resetForFoulPlay()`
- `checkIfNextPair()` reset path → calls `resetForNextPair()`

### Step 7.5: Remove initializeTemporaryGameAnalysisInfo and loadMutableWorldSettings

After recipes are in place, these functions have no callers. Delete them. Their
responsibilities are now split cleanly between `resetPhysicalWorld()`, `resetFlowState()`,
`Referee_ResetForNewInning()`, and the team setup helper.

Run all 73 tests after each substep.

**After Phase 7:** Each field cleared by exactly one system. No copy-paste init.
Reset recipes make the initialization story readable. The dual-initialization
problem is gone.

---

## Phase 8: Organization & Polish

**Goal:** Make the codebase navigable. Good names, focused files, discoverable tests.

### Step 8.1: Rename mutable_world.c → game_frame.c

Rename file. Update Makefile. Add the pipeline documentation comment block from
OPUS_VISION.md Section IV showing stages, ownerships, and data flow.

### Step 8.2: Standardize Test Registration

Currently `test_runner.c` uses two patterns: direct `RUN_TEST()` calls (45 of them) and
wrapper functions (`run_rules_outs_tests`, `run_rules_runs_tests`) that internally call
`RUN_TEST()`. This makes `grep RUN_TEST test_runner.c | wc -l` give the wrong count.

Pick one pattern (recommend: all direct `RUN_TEST` calls) and standardize. Update docs
to reflect accurate count.

### Step 8.3: Remove Dead outOfBounds Field

`HalfInningState.outOfBounds` (globals.h:499) is never written to — foul tracking uses
`referee.foulState` state machine. Remove the field. Update any documentation that
references it (`rules_outs.h:13`).

### Step 8.4: Split common_logic.c

952 lines, ~31 functions spanning 6 responsibilities. Split by domain:

- **`player_movement.c`** — ~12 functions: `moveTowardsXZ`, `moveTowardsXYZ`,
  `setVectorXYZ`, `addToVectorXZ`, etc. Pure movement/vector helpers.
- **`game_initialization.c`** — Remaining init helpers that aren't absorbed by game_reset.c
  (`initializeBallInfo`, `initializeActionInfo`, `initializeSpatialPlayerInformation`, etc.)

The functions that move to `game_reset.c` in Phase 7 are already gone from common_logic.c
by this point.

### Step 8.5: Split game_manipulation.c

~904 lines. Split by domain:

- **`ball_update.c`** — Ball physics, ground detection, bounce behavior
- **`fielder_behavior.c`** — Fielder AI ranking, catch/throw decisions, positioning
- **`base_arrivals.c`** — `playerArrivedAtBase` event firing, base advancement

Keep `game_manipulation.c` as a thin orchestrator calling the above.

**Note:** `playerLocationOrientationAndTargets()` (~210 lines) may need decomposition
before extraction. Evaluate when we get here.

### Step 8.6: Knight Phase 3 Completion

If not done earlier: add `test_bat_outcome_promotion` contract test + `sizeof(BetweenPitchState)`
guard. This was originally planned before Phase 4 and should be done whenever convenient —
it can be done at any point without conflicting with other phases.

**After Phase 8:** Files are focused. Names match responsibilities. Tests are discoverable.
The codebase is navigable.

---

## Future Work (Re-evaluate After Phase 8)

These are known goals that are not part of the current plan:

### pRAI Lifecycle Completion

Remaining pRAI fields sort into categories (see Lifecycle Architecture section):
- **UI state** (`meterValue`, `swingMeterValue`) → move to `UIState`
- **Pitch-scoped gates** (`batterCanAdvance`) → investigate `BetweenPitchState`
- **Internal coordination** (`refreshCatchAndChange`) → make local or always-compute
- **Action state** (the rest) → stays in pRAI

### Minor Ownership Fixes

- `halfInningState.event` clearing in `game_screen.c:272` — consider separate UI notification
- Any new violations discovered during Phases 4-8

### Action & AI Decoupling

Complete `actions_messy → actions_pure` split. Intent layer for replay/network support.

---

## Summary Table

| Phase | Goal | Risk | Key Metric | Status |
|-------|------|------|-----------|--------|
| **1. Consolidate & Knight** | Remove trivial casts, knight stable functions | None | 9→3 casts, +15 unit tests | ✅ Done |
| **2. 1-Frame Contracts** | Prove pipeline contracts | None | +4 contract tests | ✅ Done |
| **3. GameEvents Migration** | batOutcome event→sticky | Low | BatOutcome enum, -25 lines | ✅ Done |
| **4. Zero Const-Casts** | Compiler enforces ownership | Medium | 3→0 const-casts | 🎯 NEXT |
| **5. get_batting_team_index** | Eliminate 10-copy formula | None | -10 duplicates, +unit tests | ⏳ TODO |
| **6. Bug Fix + Period Logic** | Fix Bug #1, extract should_period_end | Medium | Bug fixed, endPeriod unified | ⏳ TODO |
| **7. Init Unification** | Reset recipes, split init by ownership | Medium | No dual-init, no copy-paste | ⏳ TODO |
| **8. Organization** | Rename, split files, standardize tests | Low | Navigable codebase | ⏳ TODO |

Each step is independently committable and testable. Every phase leaves the codebase
strictly better than before. If we stop at any point, nothing is wasted:
- Pure functions are always useful
- Clean ownership is always useful
- Reset recipes are always useful
- Good names are always useful
- Fewer duplicated code paths are always useful
