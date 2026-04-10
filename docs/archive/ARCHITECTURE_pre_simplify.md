# PNB Architecture

**Last updated:** 2026-04-10
**Purpose:** Documents the **current state** of the codebase architecture — what exists, how it works, and where the known issues are. For the post-refactoring target, see `OPUS_VISION.md`. For the step-by-step plan to get there, see `PLAN.md`.
**Current Status:** Phases 1–3 Complete ✅ (const-casts 12→3, 73 tests, contract testing, BatOutcome consolidation) | Phase 4 Zero Const-Casts 🎯

## Vision: The Functional Pipeline

We have transformed the game loop into a strict functional pipeline where data flows in one direction.

```
State_Next = Pipeline(Physics(Input(State)))
```

**The Core Loop:**
1. **Input:** `actionInvocations` → Transforms key presses into Intent/Action Flags.
2. **Physics:** `actionImplementation`, `gameManipulation` → Updates physical world, emits transient `GameEvents`.
3. **Referee:** `update_referee` → Reads Events, updates **Legal State** (Outs, Runs, Strikes). **SOLE WRITER**.
4. **Consolidation:** `GameConsolidation_Update` → Reacts to Legal State, updates Flow, enforces physical outcomes (e.g., removing OUT players).
5. **Render:** Draws the state.

---

## Key Data Structures

### 1. The Legal State (Referee's Domain)
*   **`RefereeState`:** Persistent legal status of players (Safe, Wounded, Out) and pitch snapshots.
*   **`BetweenPitchState`:** Sticky flags for the current pitch (Catch Made, Ball Hit Ground, Bat Outcome, Resolution Processed). Promoted from GameEvents by referee. Reset by referee at pitch start.
*   **`HalfInningState`:** Outs, Strikes, Balls, Runs, event notifications (see Pattern 5 below).

### 2. The Physical State (Physics' Domain)
*   **`PlayerInfo`:** Location, velocity, animation state, baseId.
*   **`BallInfo`:** Location, velocity.
*   **`GameEvents`:** Transient flags cleared every frame (e.g., `catchMade`, `ballHitGround`, `ballHitByBat`, `pitchReleased`). Written by pre-referee stages (actions + physics). Read by referee.

### 3. Coordination State (Shared)
*   **`FlowControl`:** Request/acknowledge mailbox for user interaction flow. Consolidation posts requests (e.g., `waitingForBatterDecision = 1`), actions consume them (e.g., batting_system clears when batter enters). See Pattern 4 below.
*   **`PlayerCounters`:** Shared fact struct. Consolidation tracks player availability (`noMorePlayers`), referee resets the pool on 2-run refreshes (`nonJokerPlayersLeft`). Legitimate dual-ownership.
*   **`GameFlowState`:** Mixed ownership: `ballHome` is physics-derived (game_manipulation), camera counters are consolidation/physics shared. A grab-bag — candidate for future decomposition.
*   **`pRAI`:** Action state. Actions drive forward transitions, consolidation enforces resets (e.g., `pitchState = NONE` after resolution). Also a grab-bag.

---

## Module Responsibilities

### `src/game/referee.c` (The Judge)
*   **Role:** The **SOLE WRITER** of the Legal State.
*   **Input:** Read-only access to Physical State and `GameEvents`.
*   **Output:** Updates `RefereeState`, `BetweenPitchState`, `HalfInningState`.
*   **Logic:**
    *   Responds to events: `batterEntered` (reset strikes/balls), `pitchReleased` (snapshot state).
    *   Determines if a catch is a wound.
    *   Determines if a runner is forced out.
    *   Counts strikes/balls.
    *   Restores legal safety after a Foul Play Reset.

### `src/game/game_manipulation.c` (The Physics Engine)
*   **Role:** Simulates the physical world.
*   **Logic:**
    *   Moves ball and players based on velocity.
    *   Detects collisions and boundary checks.
    *   Emits `GameEvents` (e.g., `ballHitGround`).

### `src/game/game_consolidation.c` (The Enforcer & Game Master)
*   **Role:** Reacts to the Referee's decisions and manages flow.
*   **Logic:**
    *   **Enforcement:** Physically removes players marked `OUT` or `WOUNDED`. Triggers "panic runs" if safety is lost.
    *   **Physical Resets:** Teleports players back if foul play detected. 
    *   **Flow:** Checks for end of inning, decides when to prompt for user input (batter/free walk).

---

## The "Referee Supremacy" Pattern

1.  **Event:** Physics/Actions detect an event (e.g., Batter selected, Ball hits ground out of bounds).
    *   *Action:* Set appropriate `gameEvents` flag (e.g., `batterEntered = 1`, `ballHitGround = 1`).
2.  **Judgment:** Referee processes event.
    *   *Action:* Updates legal state (e.g., reset strikes/balls, set `foulState = DETECTED`).
3.  **Reaction:** Consolidation sees referee's decision.
    *   *Action:* Starts timer. When expired, executes physical changes (moves players).
4.  **Restoration:** Referee restores legal state after physical reset.
    *   *Action:* Restores `currentSafetyBase` from `baseAtPitchStart`.

**Critical Rule:** Only `referee.c` writes to `RefereeState`, `BetweenPitchState`, and `HalfInningState` at runtime.
All other code reads these structures or emits events. Known exceptions are documented in
"Boundary Crossings & Communication Patterns" below.

---

## The Five Communication Patterns

The game loop uses five distinct patterns for inter-stage communication. Understanding these
is essential for maintaining ownership discipline.

### Pattern 1: Frame Events (GameEvents) — CLEAN
```
Writer:    Pre-referee stages (actions + physics)
Reader:    Referee
Clearer:   clearFrameEvents() — automatic, end of frame
Lifecycle: 1 frame
```
Binary flags that answer "what happened this frame?" Set once (0→1), never explicitly zeroed
except by `clearFrameEvents()`. This is the entry point for all game events.

### Pattern 2: Referee Decisions (BetweenPitchState) — MOSTLY CLEAN
```
Writer:    Referee (promotes from GameEvents after validation)
Reader:    Consolidation, AI, action guards (between-frame reads)
Clearer:   Referee (clearBetweenPitchState at pitchReleased)
Lifecycle: 1 pitch
```
The event→decision→enforcement chain. Referee adds *validation* before promoting a raw event
to a sticky fact. Consumers read these as published decisions from the previous frame.

**Current exception:** `resolutionProcessed` is consumed (cleared to 0) by consolidation
at `game_consolidation.c:121` — the only non-referee BPS write at runtime. See the boundary
crossings table below for the planned fix (idempotent guard + let referee clear at pitch start).

### Pattern 3: State Machine Signals — CLEAN
```
Writer:    Referee (NONE → DETECTED → RESETTING)
Reader:    Consolidation (checks for RESETTING)
Clearer:   Referee (RESETTING → NONE, next frame)
Lifecycle: Multi-frame (timer + two-frame handshake)
```
Three state machines: Foul Play, Next Pair, End of Inning. All follow the same two-frame
handshake protocol:
- Frame N: Referee transitions DETECTED→RESETTING, clears own legal state
- Frame N: Consolidation sees RESETTING, executes physical reset
- Frame N+1: Referee transitions RESETTING→NONE

**Implementation note:** Each state machine captures the current state into a local variable
at the top of its if-else-if chain (e.g., `endState = refereeState->endOfInningState`).
This prevents same-frame cascading — DETECTED→RESETTING cannot fall through to RESETTING→NONE.

What happens at RESETTING→NONE differs by disruption type:
- Foul play: `clearBetweenPitchState()` — post-reset cleanup
- Next pair: rescan physical world — finds new batter/runner positions
- End of inning: just transitions — events (`batterEntered`, `pitchReleased`) rebuild naturally

### Pattern 4: Coordination Requests (FlowControl) — SHARED MAILBOX
```
Poster:    Consolidation (sets requests: waitingForBatterDecision=1, etc.)
Consumer:  Actions (acknowledges: waitingForBatterDecision=0 when batter enters)
Canceller: Consolidation (can cancel: waitingForFreeWalkDecision=0 if runner gets out)
Reader:    Everyone (AI, input, guards)
Lifecycle: Per-situation (until acknowledged or cancelled)
```
This is NOT single-owner state — it's a two-party request/acknowledge protocol. Either the
requester (consolidation) or the fulfiller (actions) can clear the flag. This is intentional
and natural: it's how "we need a batter" → "batter selected" works.

**Current violation:** Referee also writes to FlowControl via const-casts (lines 712-714, 1141).
Phase 4 moves these writes to consolidation, making the protocol clean.

### Pattern 5: UI Notifications (halfInningState.event) — MISPLACED
```
Writer:    Referee (sets EVENT_OUT, EVENT_RUN_SCORED, etc.)
Reader:    Renderer (game_screen.c)
Clearer:   Renderer (game_screen.c:272)
Lifecycle: Until displayed
```
This field is a notification message, not game state. The renderer clears it because only
the renderer knows when the notification has been displayed. This is a legitimate pattern
(message channel), but the field currently lives in `HalfInningState` — a referee-owned struct
— creating a cross-boundary write from the renderer. See boundary crossings table.

---

## Known Boundary Crossings (Runtime)

These are cross-boundary writes that exist today. Each is either planned for fixing or
documented as a legitimate shared pattern.

| Location | Write | Why | Fix |
|----------|-------|-----|-----|
| `referee.c:955` | `(FlowControl*)` const-cast for freeWalk fields | Referee triggers freeWalk recalculation | Phase 4: move to consolidation |
| `referee.c:1141` | `(FlowControl*)` const-cast, `waitingForBatterDecision=0` | Referee prevents decisions during end-of-inning | Phase 4: move to consolidation |
| `referee.c:1191` | `(MatchSession*)` const-cast for `&referee` | `initialize_referee()` signature too narrow | Phase 4: fix signature |
| `game_consolidation.c:519` | `halfInningState.endPeriod = 1` | HR contest early-termination (rules decision) | Phase 6: move to referee |
| `game_consolidation.c:121` | `betweenPitchState.resolutionProcessed = 0` | Consuming one-shot signal | Phase 4+: use idempotent guard instead |
| `game_screen.c:272` | `halfInningState.event = EVENT_NONE` | Renderer consuming UI notification | Future: extract to notification struct |
| `batting_system.c:108` | `flowControl.waitingForBatterDecision = 0` | Actions acknowledging request | Pattern 4: legitimate (request/acknowledge) |
| `action_implementation.c:245` | `flowControl.waitingForFreeWalkDecision = 0` | Actions acknowledging request | Pattern 4: legitimate (request/acknowledge) |
| `game_consolidation.c:120` | `pRAI.pitchState = PITCH_STAGE_NONE` | Enforcement reset after pitch resolution | Legitimate: same as removing OUT players |
| `game_manipulation.c:891,902` | `gameFlowState.ballHome = 0/1` | Physics detecting ball position | Legitimate: physics-derived state |

## Initialization & Transitions

### Two Distinct Patterns

**Setup (Game Start):**
- Explicit function call: `initialize_referee()` in `referee.c`
- Called during setup phase from `loadGameScreenSettings()` in `game_screen.c:477`
- Scans physical world and establishes initial legal tracking

**Transitions (Runtime):**
- Self-managed via state machines in Referee
- Three transitions: Foul Play, Next Pair, End of Inning
- Pattern: NONE → DETECTED (timer) → RESETTING → NONE
- Referee clears own state, signals Consolidation, world resets

### Transition Timing (Critical Understanding)

All three state machines follow the same two-frame handshake. Example for End of Inning:

**Frame N+200:**
1. Referee timer expires, transitions DETECTED → RESETTING
2. Referee clears its own state (strikes, balls, safety, BPS)
3. Consolidation sees RESETTING, calls `loadMutableWorldSettings()`
4. Physical world is reset

**Frame N+201:**
1. Referee transitions RESETTING → NONE
2. Normal update logic resumes
3. Safety naturally rebuilds via events (`batterEntered`, `pitchReleased`)

**Key Insight:** Referee clears state at Frame N+200 because it knows Consolidation will reset the physical world in the same frame. No event needed — it's a state machine handshake.

**Implementation detail:** Each state machine captures its state into a **local variable** before
the if-else-if chain (e.g., `EndOfInningTransitionState endState = refereeState->endOfInningState`).
The chain tests the local, not the struct field. This prevents DETECTED→RESETTING from cascading
into RESETTING→NONE within the same frame.

---

## Current Roadmap

The active refactoring plan lives in **`docs/PLAN.md`**. The architectural target is in **`docs/OPUS_VISION.md`**.

| Phase | Goal | Status |
|-------|------|--------|
| **1** | **Consolidate & Knight** — remove trivial const-casts (9→3), add unit tests for stable pure functions | ✅ Done |
| **2** | **1-Frame Contract Tests** — prove pipeline stage cooperation at the frame level | ✅ Done |
| **3** | **GameEvents Migration** — BatOutcome enum in BetweenPitchState, event→sticky pattern, fixed event semantics | ✅ Done |
| **4** | **Zero Const-Casts** — eliminate last 3 const-casts, compiler enforces ownership | **🎯 NEXT** |
| **5** | **Extract get_batting_team_index** — replace 10 copies of the batting-team formula | ⏳ TODO |
| **6** | **Bug Fix + Period Logic** — extract `should_period_end()`, fix Bug #1 (pending runs ignore endPeriod) | ⏳ TODO |
| **7** | **Initialization Unification** — reset recipes, split init by ownership, eliminate dual-init | ⏳ TODO |
| **8** | **Organization** — rename files, split large files, standardize tests | ⏳ TODO |

*Previous phases (Dead Code Cleanup, WOUNDED Enforcement, homerunPairHasPitch move) are complete. See `docs/archive/` for history.*

---

## Build & Test

```bash
# Build
make main

# Test (73 tests: 54 unit + 4 contract + 15 scenario)
devenv shell make test              # Unit tests
devenv shell make integration_test  # Contract tests (1-frame pipeline proofs)
devenv shell make scenario_test     # Scenario tests (full-game simulations)
```
