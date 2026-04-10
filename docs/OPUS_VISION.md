# Architectural Vision: Post-Phase-6 Target State

**Date:** 2026-04-10
**Purpose:** Concrete picture of what the codebase looks like after all planned phases,
with special attention to the initialization story, the normal pitch cycle, and the
disruption patterns. This is the target we're building toward.

---

## I. The Two Worlds

The game has two fundamentally different modes happening simultaneously:

### The Free Outfield

The catching team's fielders operate in a nearly stateless world. They rank themselves
by distance to ball, move to catch or field, throw to bases, replace each other. This
needs almost no lifecycle management — just physical state (positions, velocities) and
the ball location. No special structs, no state machines, no events. Just physics.

### The Home Plate Drama

All the complexity lives here. This is the "Shakespearean play" — a multi-act cycle of
batter selection, pitching, hitting, running, scoring, and resolution. This is where:
- The lifecycle tiers matter (frame / pitch / inning / game)
- The event→sticky mechanism matters
- The referee's rule adjudication matters
- The state machines manage transitions

The architecture exists to serve the Home Plate Drama while leaving the Free Outfield alone.

---

## II. The Normal Pitch Cycle

This is the heartbeat of the game. When no disruptions occur, the game loops through
this cycle indefinitely:

```
┌─────────────────────────────────────────────────────────────────┐
│                     THE PITCH CYCLE                             │
│                                                                 │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐  │
│  │ WAITING  │───>│  BATTER  │───>│ PITCHING │───>│ BALL IN  │  │
│  │ FOR      │    │  ENTERS  │    │          │    │  PLAY    │  │
│  │ BATTER   │    │          │    │          │    │          │  │
│  └──────────┘    └──────────┘    └──────────┘    └──────────┘  │
│       ^                                               │         │
│       │          ┌──────────┐                         │         │
│       └──────────│RESOLUTION│<────────────────────────┘         │
│                  │          │                                    │
│                  └──────────┘                                    │
│                       │                                         │
│              (3 outs / period end?)                              │
│                       │ yes                                     │
│                       v                                         │
│                  ┌──────────┐                                   │
│                  │  RESET   │  (end of inning / foul / etc)     │
│                  └──────────┘                                   │
└─────────────────────────────────────────────────────────────────┘
```

### How each transition works through the event mechanism:

**WAITING → BATTER ENTERS:**
- Consolidation's `checkIfNextBatterDecision()` checks:
  - No active batter? Ball has settled (ground or caught)? No foul active? No end-of-inning?
  - Sets `flowControl.waitingForBatterDecision = 1`
- Action stage processes the selection (user choice or auto)
- `batting_system.c` fires `batterEntered` event
- Referee reacts: resets strikes/balls, initializes batter safety at HOME

**BATTER ENTERS → PITCHING:**
- Pitcher prepares. `pRAI.pitchState` cycles: NONE → WINDUP → AIRBORNE
- `pitching_system.c` fires `pitchReleased` event
- Referee reacts: snapshots all base positions (`baseAtPitchStart`), clears
  `BetweenPitchState`, captures `strikesAtPitchStart`
- This is the "savepoint" — foul play can restore to this moment

**PITCHING → BALL IN PLAY:**
- Ball is live. The Free Outfield and Home Plate Drama interact:
  - `ballHitByBat` / `ballMissedByBat` → Referee promotes to `batOutcome`
  - `catchMade` → Referee starts wounding evaluation (timer + fly ball rules)
  - `playerArrivedAtBase` → Referee evaluates: run? pending run? safety change?
  - `ballHitGround` → Referee checks: foul (out of bounds)? pitch resolution?
  - Safety updates run continuously (referee tracks who controls each base)
  - Force outs, tuplahaava (double wound) checked continuously

**BALL IN PLAY → RESOLUTION:**
- Ball settles (hits ground or is caught). Wounding resolves. Pending runs resolve.
- `resolutionProcessed` flag set by referee → Consolidation clears `pitchState`
- Strike/ball count updated. Free walk offered if applicable.
- `ballHome` flag set by game_manipulation when ball returns to pitcher

**RESOLUTION → WAITING (next pitch or next batter):**
- If batter still at home and strikes < 3 → back to PITCHING (same batter, next pitch)
- If batter advanced, was wounded, or struck out → back to WAITING (next batter)
- If 3 outs or period should end → RESET via state machine

### What makes this work

No explicit pitch-lifecycle state machine is needed. The cycle emerges from:
1. **GameEvents** (transient, frame-level): the immediate physical facts
2. **BetweenPitchState** (sticky, pitch-level): the referee's validated facts for this pitch
3. **FlowControl** (consolidation-owned): the "what are we waiting for?" flags
4. **pRAI.pitchState** (action-owned): the current pitching phase

These four structs, with their different lifecycles, naturally encode the pitch cycle
without a state machine. The event→sticky→enforcement chain handles the transitions.

This is the part that works well today and needs no structural changes.

---

## III. The Four Disruptions

The normal cycle is the steady state. Disruptions are clean transitions back to it.

### Disruption 1: From-Menu Initialization (Game Start)

**When:** Game screen loads. First half-inning ever, or returning from period transition menu.
**What resets:** EVERYTHING. Clean slate.
**Who does what:**
```
loadGameScreenSettings():
  1. resetPhysicalWorld()           — Ball, players, positions, actions
  2. resetFlowState()               — FlowControl, camera, subsystems
  3. Referee_ResetForNewInning()     — All legal state (RefereeState, HIS, BPS)
  4. initializeInningPlayers()       — Team assignments, batter order
  5. Referee_ScanPhysicalWorld()     — Establish initial legal tracking from positions
```
**Resumes at:** WAITING (step 1 of pitch cycle)

### Disruption 2: Foul Play (Out of Bounds)

**When:** Ball lands out of bounds on first bounce after a hit.
**What resets:** Physical positions (restore to pitch start). Legal state restored from snapshot.
**Preserves:** Outs, runs, inning progress. Strike count incremented.
**State machine:** `FOUL_STATE_NONE → DETECTED (timer) → RESETTING → NONE`

**The handoff:**
```
Frame N-200: Referee detects foul. DETECTED. Timer starts. EVENT_OUT_OF_BOUNDS displayed.
Frame N:     Timer expires. RESETTING.
             Referee: Restores legal state from baseAtPitchStart snapshot.
                      Increments strike (or awards out if 3rd strike).
                      Returns early from further processing.
             Consolidation: resetForFoulPlay()
                      Physical world reset + position restore from referee's snapshots.
Frame N+1:   Referee: RESETTING → NONE. Clears BetweenPitchState.
             Normal cycle resumes.
```
**Resumes at:** PITCHING (step 3 — same batter, next pitch, incremented strike)

### Disruption 3: Next Pair (Homerun Contest)

**When:** Current batter/runner pair is done (ball home, no more action possible).
**What resets:** Per-pitch and per-pair state. Physical positions for new pair.
**Preserves:** Inning runs, pair counter.
**State machine:** `HR_PAIR_STATE_NONE → DETECTED (timer) → RESETTING → NONE`

**The handoff:**
```
Frame N-200: Referee detects pair end. DETECTED. Timer starts. EVENT_NEXT_PAIR displayed.
Frame N:     Timer expires. RESETTING.
             Referee: Clears per-pair legal state (strikes, balls, BPS, player tracking).
             Consolidation: Increments pair counter. resetForNextPair()
                      Physical world reset + homerun pair setup.
Frame N+1:   Referee: RESETTING → NONE. Scans physical world for new batter/runner.
             Normal cycle resumes.
```
**Resumes at:** WAITING (step 1 of pitch cycle, new pair)

### Disruption 4: End of Half-Inning

**When:** 3 outs, no more players, or period should end.
**What resets:** Everything (same as from-menu, but with scoreboard advancement).
**State machine:** `END_INNING_STATE_NONE → DETECTED (timer) → RESETTING → NONE`

**The handoff:**
```
Frame N-200: Referee detects end condition. DETECTED. Timer starts. EVENT_INNING_ENDING.
Frame N:     Timer expires. RESETTING.
             Referee: Clears ALL legal state.
             Consolidation: Advances scoreboard.inning.
                      If period boundary → transition to menu (no further game updates).
                      If continuing → resetForNewHalfInning() (full reset).
Frame N+1:   Referee: RESETTING → NONE.
             Events (batterEntered, pitchReleased) rebuild referee tracking incrementally.
             Normal cycle resumes.
```
**Resumes at:** WAITING (step 1 of pitch cycle, new inning) or MENU (period boundary)

---

## IV. The Pipeline (Post-Phase-6)

The pipeline itself doesn't change. What changes is the clarity around it.

```c
// game_frame.c (renamed from mutable_world.c)

void updateGameFrame(StateInfo* stateInfo, MenuInfo* menuInfo, unsigned int* rng_seed)
{
    if (stateInfo->match->flowControl.pause) return;

    // ──── Stage 1: Input ────
    // READS:  KeyStates
    // WRITES: ActionFlags
    actionInvocations(stateInfo);

    // ──── Stage 2: Physics & Action ────
    // READS:  ActionFlags, world state, BetweenPitchState (between-frame reads)
    // WRITES: PlayerInfo, BallInfo, GameEvents, pRAI, PendingActionState, AIState
    actionImplementation(stateInfo, rng_seed);
    gameManipulation(stateInfo);

    // ──── Stage 3: Referee (Legal Authority) ────
    // READS:  const StateInfo* (everything, read-only)
    // WRITES: RefereeState, HalfInningState, BetweenPitchState, PlayerCounters, Scoreboard
    //         (via explicit pointers — compiler enforces ownership, ZERO const-casts)
    update_referee(
        stateInfo,
        &game->referee, &game->halfInningState, &game->betweenPitchState,
        &game->playerCounters, &game->scoreboard
    );

    // ──── Stage 4: Consolidation (Enforcement + Flow + Resets) ────
    // READS:  RefereeState (decisions), BetweenPitchState (flags)
    // WRITES: PlayerInfo (enforcement), FlowControl (flow posts),
    //         pRAI.pitchState (enforcement reset), Scoreboard (inning advancement)
    // RESETS: Calls reset recipes when referee signals RESETTING
    //
    // NOTE: Consolidation READS BPS signals but does NOT write to BPS.
    // It uses guard conditions (e.g., pitchState != NONE) for idempotency
    // instead of consuming (clearing) flags. Referee clears BPS at lifecycle
    // boundaries (clearBetweenPitchState on pitchReleased).
    GameConsolidation_Update(stateInfo, menuInfo, rng_seed);

    // ──── Stage 5: Cleanup ────
    clearFrameEvents(&stateInfo->match->gameEvents);
}
```

---

## V. Data Ownership (Post-Phase-6)

### Struct Ownership Table

| Struct | Initialized by | Runtime writer | Runtime readers | Lifecycle |
|--------|---------------|----------------|-----------------|-----------|
| **GameEvents** | clearFrameEvents (auto) | Pre-referee stages (actions + physics) | Stage 3 (referee) | 1 frame |
| **BetweenPitchState** | Referee (at pitch start) | Stage 3 (referee) | Stage 2 (guards), Stage 4 | 1 pitch |
| **RefereeState** | Referee (scan or events) | Stage 3 (referee) | Stage 4 (enforcement) | 1 inning |
| **HalfInningState** | Referee (via reset API) | Stage 3 (referee) | Stage 4 (flow), renderer | 1 half-inning |
| **FlowControl** | resetFlowState() | Stage 4 posts, Stage 2 consumes | Everyone | Per-situation |
| **PlayerCounters** | resetForNewHalfInning() | Stage 3 (pool refresh) + Stage 4 (availability) | Stage 3, Stage 4 | Per-inning |
| **PlayerInfo/BallInfo** | resetPhysicalWorld() | Stage 2 + Stage 4 (enforcement) | Everyone | Continuous |
| **pRAI** | resetPhysicalWorld() | Stage 2 (action) + Stage 4 (pitchState reset) | Stage 2, Stage 3 (reads) | Mixed |
| **Scoreboard** | resetForNewHalfInning() | Stage 2 (batterOrderIndex) + Stage 3 (runs) + Stage 4 (inning++) | Everyone | Per-game |
| **ActionFlags** | Stage 1 (auto) | Stage 1 (input) | Stage 2 (action) | 1 frame |

### Ownership Categories

Not all structs follow single-owner semantics. Three categories exist:

**Single-owner (compiler-enforced):** GameEvents, BetweenPitchState, RefereeState, HalfInningState,
ActionFlags. The referee's `const StateInfo*` + explicit writable pointers enforce this at
compile time. These are the core structs that must never have boundary crossings.

**Shared facts (legitimate dual-ownership):** Scoreboard (referee writes runs, consolidation
writes inning/period, actions advance batterOrderIndex), PlayerCounters (referee writes pool
refreshes, consolidation writes availability, actions decrement on batter selection),
PlayerInfo (actions move players, consolidation enforces legal outcomes).
These have multiple writers because the responsibilities genuinely split across stages.

**Coordination structs (request/acknowledge):** FlowControl (consolidation posts, actions
consume), pRAI (actions drive forward, consolidation resets). These are communication
channels, not state owned by one party.

### The Key Insight: Initialization ≠ Runtime

Initialization and runtime have different ownership rules, and that's OK:

```
                    INITIALIZATION           RUNTIME (pipeline)
                    ──────────────           ──────────────────
Physical state:     resetPhysicalWorld()  →  Stage 2 writes, Stage 4 enforces
Flow state:         resetFlowState()      →  Stage 4 posts, Stage 2 consumes
Legal state:        Referee reset API     →  Stage 3 writes (compiler-enforced)
```

During initialization, the reset recipe functions coordinate everything.
During runtime, the pipeline stages enforce ownership through const and signatures.

The old confusion came from `initializeTemporaryGameAnalysisInfo()` mixing both physical
AND legal initialization. The fix: split it. Physical init stays in game_reset.c.
Legal init lives in referee API functions.

### The Five Communication Patterns

The pipeline stages communicate through five distinct mechanisms. Understanding these
patterns is essential — they are the vocabulary of the architecture. Every inter-stage
data flow must fit one of these patterns; anything that doesn't is a design smell.

**Pattern 1: Frame Events (GameEvents)**
```
Writer:    Pre-referee stages (actions + physics)
Reader:    Referee
Clearer:   clearFrameEvents() — automatic, end of frame
Lifecycle: 1 frame
```
Binary flags answering "what happened this frame?" Set once (0→1), never explicitly
zeroed except by `clearFrameEvents()`. This is the entry point for all game events.

**Pattern 2: Referee Decisions (BetweenPitchState)**
```
Writer:    Referee (promotes from GameEvents after validation)
Reader:    Consolidation, AI, action guards (between-frame reads)
Clearer:   Referee (clearBetweenPitchState at pitchReleased)
Lifecycle: 1 pitch
```
The event→decision→enforcement chain. Referee adds *validation* before promoting a
raw event to a sticky fact. Consumers read these as published decisions.

**Pattern 3: State Machine Signals (foulState, endOfInningState, nextPairTransitionState)**
```
Writer:    Referee (NONE → DETECTED → RESETTING)
Reader:    Consolidation (checks for RESETTING)
Clearer:   Referee (RESETTING → NONE, next frame)
Lifecycle: Multi-frame (timer + two-frame handshake)
```
All three state machines follow the same two-frame handshake protocol:
- Frame N: Referee transitions DETECTED→RESETTING, clears own legal state
- Frame N: Consolidation sees RESETTING, executes physical reset
- Frame N+1: Referee transitions RESETTING→NONE

Implementation detail: each state machine captures its state into a local variable
at the top of its if-else-if chain (e.g., `endState = refereeState->endOfInningState`).
This prevents same-frame cascading — DETECTED→RESETTING cannot fall through to
RESETTING→NONE within the same frame.

What happens at RESETTING→NONE differs by disruption type:
- Foul play: `clearBetweenPitchState()` — post-reset cleanup
- Next pair: rescan physical world — finds new batter/runner positions
- End of inning: just transitions — events (`batterEntered`, `pitchReleased`) rebuild naturally

**Pattern 4: Coordination Requests (FlowControl)**
```
Poster:    Consolidation (sets requests: waitingForBatterDecision=1, etc.)
Consumer:  Actions (acknowledges: waitingForBatterDecision=0 when batter enters)
Canceller: Consolidation (can cancel if situation changes)
Reader:    Everyone (AI, input, guards)
Lifecycle: Per-situation (until acknowledged or cancelled)
```
NOT single-owner state — it's a two-party request/acknowledge protocol. Either the
requester (consolidation) or the fulfiller (actions) can clear the flag. This is
intentional and natural: "we need a batter" → "batter selected."

**Pattern 5: UI Notifications (halfInningState.event)**
```
Writer:    Referee (sets EVENT_OUT, EVENT_RUN_SCORED, etc.)
Reader:    Renderer (game_screen.c)
Clearer:   Renderer (game_screen.c:272)
Lifecycle: Until displayed
```
A notification message channel, not game state. The renderer clears it because only
the renderer knows when the notification has been shown. Currently lives in
`HalfInningState` (referee-owned struct), creating a cross-boundary write from
the renderer. Future: extract to a dedicated notification struct.

### Known Boundary Crossings (Today)

Every cross-boundary write in the current codebase, with its status:

| Location | Write | Status |
|----------|-------|--------|
| `referee.c:955` | `(FlowControl*)` const-cast for freeWalk fields | Phase 4: move to consolidation |
| `referee.c:1141` | `(FlowControl*)` const-cast, `waitingForBatterDecision=0` | Phase 4: move to consolidation |
| `referee.c:1191` | `(MatchSession*)` const-cast for `&referee` | Phase 4: fix signature |
| `game_consolidation.c:519` | `halfInningState.endPeriod = 1` | Phase 6: move to referee |
| `game_consolidation.c:121` | `betweenPitchState.resolutionProcessed = 0` | Phase 4+: idempotent guard |
| `game_screen.c:272` | `halfInningState.event = EVENT_NONE` | Future: extract to notification struct |
| `batting_system.c:108` | `flowControl.waitingForBatterDecision = 0` | Pattern 4: legitimate |
| `action_implementation.c:245` | `flowControl.waitingForFreeWalkDecision = 0` | Pattern 4: legitimate |
| `game_consolidation.c:120` | `pRAI.pitchState = PITCH_STAGE_NONE` | Legitimate: enforcement |
| `game_manipulation.c:891,902` | `gameFlowState.ballHome = 0/1` | Legitimate: physics-derived |
| `batting_system.c:136` | `scoreboard.teams[].batterOrderIndex++` | Shared fact: action side-effect |
| `game_screen.c:75,86` | `flowControl.pause = 0/1` | Outer-loop control (see below) |
| `mutable_world.c:83` | `flowControl.pause = 1` | Debug halt (outer-loop control) |
| `common_logic.c:502` | `flowControl.freeWalkIndex/Base` | Consolidation helper (called from consolidation, misplaced in common_logic.c — Phase 8) |

**Note on `flowControl.pause`:** This field is not part of the pipeline's inter-stage
communication. It is outer-loop control — the game screen sets it on user input (HOME key),
the debug validator sets it on consistency failure, and the pipeline checks it before running.
It lives in FlowControl for convenience but is conceptually a separate concern. Future
consideration: move to a dedicated `GameLoopControl` struct or leave as-is (it's harmless).

After Phase 6, the only remaining "crossings" will be legitimate shared patterns
(Pattern 4, Pattern 5, shared facts, outer-loop control) plus physics-derived state
and the `calculateFreeWalk()` helper (Phase 8 moves it to its proper home).
Everything else will be clean.

---

## VI. The Initialization Story (Post-Phase-6 Vision)

### Building Blocks

Two layers of reset, each with clear ownership:

```c
// ═══════════════════════════════════════════
// game_reset.c — Physical + Flow resets
// ═══════════════════════════════════════════

// Resets everything the physics/action stages own.
// Does NOT touch referee-owned state (RefereeState, HalfInningState, BetweenPitchState).
void resetPhysicalWorld(StateInfo* stateInfo, unsigned int* rng_seed)
{
    MatchSession* game = stateInfo->match;
    initializeBallInfo(game);
    initializeActionInfo(game);
    initializeIndexInformation(game);
    initializePRAIInformation(game);
    initializeSpatialPlayerInformation(game, stateInfo->fieldPositions, rng_seed);
    initializeNonCriticalPlayerInformation(game);
    clearFrameEvents(&game->gameEvents);
}

// Resets consolidation-owned flow state.
// Does NOT touch referee-owned state.
void resetFlowState(MatchSession* game)
{
    game->flowControl.pause = 0;
    game->flowControl.waitingForBatterDecision = 0;
    game->flowControl.waitingForFreeWalkDecision = 0;
    game->flowControl.freeWalkCalculationMade = 1;
    game->flowControl.freeWalkIndex = -1;
    game->flowControl.freeWalkBase = BASE_NONE;

    game->playerCounters.noMorePlayers = 0;
    game->gameFlowState.ballHome = 0;
    GameConsolidation_Init(&game->gameFlowState);
    initGameManipulation(&game->gameFlowState);

    game->cameraState.homeRunCameraFlag = 0;
    // ... other camera reset ...
}
```

```c
// ═══════════════════════════════════════════
// referee.c — Legal state resets (referee API)
// ═══════════════════════════════════════════

// Full legal reset for a new half-inning. Clears EVERYTHING the referee owns.
void Referee_ResetForNewInning(
    RefereeState* ref, HalfInningState* his, BetweenPitchState* bps)
{
    initializeRefereeState(ref);    // All per-player tracking, state machines

    his->outs = 0;
    his->balls = 0;
    his->strikes = 0;
    his->runsInTheInning = 0;
    his->event = EVENT_NONE;
    his->endPeriod = 0;
    his->outOfBounds = 0;

    clearBetweenPitchState(bps);
}

// Scan physical world and establish initial legal tracking.
// Called after physical world is set up (positions are final).
void Referee_ScanPhysicalWorld(StateInfo* stateInfo)
{
    // This is the current initialize_referee() — scans playerInfo positions
    // and sets currentSafetyBase for each player found at a base.
    // Works for both normal games and homerun contest.
}
```

### The Four Reset Recipes

Each recipe calls the building blocks in the right order. Each is a self-documenting
sequence. No copy-paste. No overlap.

```c
// ═══════════════════════════════════════════
// Recipe 1: NEW HALF-INNING (full reset)
// ═══════════════════════════════════════════
void resetForNewHalfInning(StateInfo* stateInfo, unsigned int* rng_seed)
{
    // 1. Physical world (ball, players, positions, actions)
    resetPhysicalWorld(stateInfo, rng_seed);

    // 2. Flow state (consolidation-owned)
    resetFlowState(stateInfo->match);

    // 3. Legal state (referee-owned) — the referee clears its own world
    Referee_ResetForNewInning(
        &stateInfo->match->referee,
        &stateInfo->match->halfInningState,
        &stateInfo->match->betweenPitchState);

    // 4. Team setup (batter order, player counts)
    initializeCriticalGameInfo(stateInfo->match, &stateInfo->match->scoreboard);
    initializeInningPermanentPlayerInformation(
        stateInfo->match, &stateInfo->match->scoreboard, stateInfo->teamData);

    // 5. Homerun contest special setup
    if (stateInfo->match->scoreboard.period >= 4) {
        setupHomerunPhysicalState(stateInfo->match, &stateInfo->match->scoreboard,
                                  stateInfo->fieldPositions);
    }
}

// ═══════════════════════════════════════════
// Recipe 2: FOUL PLAY (restore from snapshot)
// ═══════════════════════════════════════════
// NOTE: Referee has ALREADY restored legal state from baseAtPitchStart snapshots
//       and handled strike/out consequences at the DETECTED→RESETTING transition.
//       We only need to reset the physical world and position players to match.

void resetForFoulPlay(StateInfo* stateInfo, unsigned int* rng_seed)
{
    // 1. Physical world reset
    resetPhysicalWorld(stateInfo, rng_seed);

    // 2. Flow state reset
    resetFlowState(stateInfo->match);

    // 3. Position players according to referee's legal state
    //    (the referee already restored baseAtPitchStart in its RESETTING transition)
    restorePlayersToRefereePositions(stateInfo);

    // 4. Homerun contest special setup if applicable
    if (stateInfo->match->scoreboard.period >= 4) {
        setupHomerunPhysicalState(...);
    }
}

// ═══════════════════════════════════════════
// Recipe 3: NEXT PAIR (homerun contest)
// ═══════════════════════════════════════════
// NOTE: Referee has ALREADY cleared per-pair legal state at RESETTING transition.

void resetForNextPair(StateInfo* stateInfo, unsigned int* rng_seed)
{
    // 1. Physical world reset
    resetPhysicalWorld(stateInfo, rng_seed);

    // 2. Flow state reset
    resetFlowState(stateInfo->match);

    // 3. Set up new pair
    setupHomerunPhysicalState(stateInfo->match, &stateInfo->match->scoreboard,
                              stateInfo->fieldPositions);
}

// ═══════════════════════════════════════════
// Recipe 4: FROM-MENU (game start / return)
// ═══════════════════════════════════════════
void initializeGameFromMenu(StateInfo* stateInfo, unsigned int* rng_seed)
{
    // Full reset + explicit referee scan (no prior event history)
    resetForNewHalfInning(stateInfo, rng_seed);
    Referee_ScanPhysicalWorld(stateInfo);
}
```

### What's Different from Today

| Today | Vision |
|-------|--------|
| `initializeTemporaryGameAnalysisInfo()` clears BPS, HIS.event, HIS.endPeriod, FlowControl, camera, subsystems — one function crossing all boundaries | Split into `resetFlowState()` (consolidation-owned) and `Referee_ResetForNewInning()` (referee-owned) |
| 3 places copy-paste 7 init function calls | Shared `resetPhysicalWorld()` building block called from each recipe |
| Referee clears strikes/balls at RESETTING, then `loadMutableWorldSettings` clears them again | Only `Referee_ResetForNewInning()` clears referee-owned fields. No redundancy. |
| `loadMutableWorldSettings` has a comment saying "call initialize_referee after" but some callers don't | `initializeGameFromMenu()` is the only entry point that calls `Referee_ScanPhysicalWorld()`. Runtime recipes don't need it (events rebuild). This is explicit in the recipe. |
| Four different referee init patterns, undocumented | Each recipe documents what the referee does and why (comments in the recipe) |

### The Rule

> **Physical reset functions never touch referee-owned state.**
> **Referee reset functions never touch physical state.**
> **Reset recipes compose them in the right order.**

This rule eliminates the dual-initialization problem entirely. Each field is cleared by
exactly one system. No overlaps. No "which clearing is the real one?" questions.

---

## VII. The Lifecycle Tiers (Unchanged, but Now Clean)

The four tiers remain. What changes is that each tier's reset is handled by the right owner:

```
TIER 1 — FRAME (GameEvents)
  Reset: clearFrameEvents() at end of every frame
  Owner: Pipeline (automatic)
  No changes needed.

TIER 2 — PITCH (BetweenPitchState)
  Reset: Referee on pitchReleased event (clearBetweenPitchState)
  Reset: Referee at foul-play RESETTING → NONE transition
  Owner: Referee (sole writer)
  No changes needed — already clean.

TIER 3 — INNING (RefereeState, HalfInningState)
  Reset: Referee_ResetForNewInning() via reset recipes
  Reset: Referee state machines (RESETTING transitions)
  Owner: Referee (sole writer at runtime, sole initializer at reset)
  Change: HalfInningState initialization moves FROM common_logic TO referee API.

TIER 4 — GAME (Scoreboard)
  Reset: Period transitions in consolidation
  Owner: Referee (runs) + Consolidation (inning/period advancement)
  No changes needed — the split is natural.
```

---

## VIII. File Structure (Post-Phase-6)

```
src/game/
  game_frame.c                  ← renamed from mutable_world.c
  game_frame.h                    The 5-stage pipeline. THE entry point.

  referee.c                       Legal authority. Zero const-casts.
  referee.h                       Public API including reset functions.

  game_consolidation.c            Enforcement + flow + calls reset recipes.
  game_consolidation.h

  game_reset.c                  ← NEW: reset building blocks + recipes
  game_reset.h                    Replaces scattered init in common_logic.c

  ball_update.c                 ← split from game_manipulation.c
  fielder_behavior.c            ← split from game_manipulation.c
  base_arrivals.c               ← split from game_manipulation.c
  game_manipulation.c             Orchestrator calling the above three.

  player_movement.c             ← split from common_logic.c
  game_initialization.c         ← remaining init helpers from common_logic.c

  action_invocations.c            Input → action flags (unchanged)
  action_implementation.c         Action flags → execution (unchanged)

  actions_messy/                  Batting, pitching, throwing execution
  actions_pure/                   Physics formulas
  ai_messy/                       AI execution
  ai_pure/                        AI decision functions

  rules_pure/
    rules_outs.c                  Out evaluation
    rules_runs.c                  Run evaluation
    rules_strikes.c               Strike evaluation
    base_logic.c                  Base utilities
    base_control.c                Base controller lookup
    player_utils.c                Player queries
    scoring_helpers.c           ← NEW: get_batting_team_index(), should_period_end()
```

---

## IX. How the Normal Mode and Disruptions Fit the Architecture

```
                    ┌─────────────────────┐
                    │   FROM-MENU INIT    │
                    │ (resetForNewHalf    │
                    │  Inning + ScanWorld)│
                    └─────────┬───────────┘
                              │
                              v
              ┌───────────────────────────────┐
              │                               │
              │      NORMAL PITCH CYCLE       │
              │                               │
              │  WAITING → BATTER → PITCH →   │
              │  BALL IN PLAY → RESOLUTION    │
              │         (loop)                │
              │                               │
              │  Handled entirely by:         │
              │  • GameEvents (frame-level)   │
              │  • BetweenPitchState (pitch)  │
              │  • FlowControl (flow gates)   │
              │  • pRAI.pitchState (action)   │
              │                               │
              └───────┬───────┬───────┬───────┘
                      │       │       │
              foul?   │  pair │  3 outs/
              play    │  done?│  period end?
                      │       │       │
                      v       v       v
              ┌───────┐ ┌───────┐ ┌───────┐
              │ FOUL  │ │ NEXT  │ │  END  │
              │ RESET │ │ PAIR  │ │INNING │
              │       │ │ RESET │ │ RESET │
              └───┬───┘ └───┬───┘ └───┬───┘
                  │         │         │
                  │    Each reset:    │
                  │  1. Referee clears│its own legal state (at RESETTING)
                  │  2. Consolidation │calls appropriate reset recipe
                  │  3. Next frame:   │Referee transitions to NONE
                  │         │         │
                  v         v         v
              Back to NORMAL PITCH CYCLE
              (or MENU if period boundary)
```

The architecture has exactly two concerns:
1. **The normal cycle** — handled by events and sticky states. Works. No changes needed.
2. **The disruptions** — handled by state machines + reset recipes. Needs cleanup (Phase 7 in PLAN.md).

Everything else is in service of these two concerns.

---

## X. Safe Path from Here to There

Every step below leaves all tests green and makes the codebase strictly better,
regardless of future macro-decisions. **For detailed execution steps (code snippets,
exact line numbers, verification commands), see `PLAN.md`.** This section explains
WHY the order is safe; the PLAN explains HOW to execute each step.

### Step 1: Knight Phase 3 (`test_bat_outcome_promotion` contract test)

Lock the event→sticky pattern with a test + sizeof guard. Zero risk.
**What it achieves:** Permanent protection of Phase 3 work.

### Step 2: Phase 4 — Zero const-casts

Move FlowControl writes to consolidation. Fix initialize_referee signature.
Fix resolutionProcessed consume pattern with idempotent guard.
**What it achieves:** Compiler enforces ownership. The biggest single improvement.

### Step 3: Extract `get_batting_team_index()` → `rules_pure/scoring_helpers.c`

Pure function. Replace 10 copies. Write unit tests. Zero risk.
**What it achieves:** Reduces noise. Makes referee.c shorter. Establishes scoring_helpers.

### Step 4: Extract `should_period_end()` → `rules_pure/scoring_helpers.c`

Pure function. Add to all 3 scoring paths in resolve_pending_runs. Write failing
scenario test first (Bug #1). Fix bug.
**What it achieves:** Bug fix + endPeriod centralized + pure function.

### Step 5: Move consolidation's `endPeriod = 1` to referee

The HR contest early-termination logic. After should_period_end() exists, the referee
can call it where needed.
**What it achieves:** endPeriod is now exclusively referee-written.

### Step 6: Split `initializeTemporaryGameAnalysisInfo()` by ownership

Create `resetFlowState()` (consolidation-owned fields) and move HalfInningState/BPS
fields into a new `Referee_ResetForNewInning()` function.
**What it achieves:** Eliminates the dual-initialization overlap. Each field cleared by one owner.

### Step 7: Create `resetPhysicalWorld()` building block

Extract the shared 7-function sequence into one function.
**What it achieves:** Eliminates copy-pasted init sequences in consolidation.

### Step 8: Create reset recipes

`resetForNewHalfInning()`, `resetForFoulPlay()`, `resetForNextPair()`, `initializeGameFromMenu()`.
Each composes the building blocks. Replace scattered init calls in consolidation and game_screen.
**What it achieves:** The initialization story becomes readable. Each reset is a recipe.

### Step 9: Rename `mutable_world.c` → `game_frame.c`

Add the pitch cycle comment block.
**What it achieves:** The entry point tells you what it is.

### Step 10: Standardize test registration

Pick one pattern (direct RUN_TEST or wrapper functions). Fix the test count discrepancy.
**What it achieves:** `grep RUN_TEST test_runner.c | wc -l` gives the right number.

### Step 11: Split `common_logic.c` and `game_manipulation.c`

By responsibility, as documented in Phase 8 of PLAN.md.
**What it achieves:** Single-responsibility files. Navigable file system.

### Why this order is safe

- Steps 1-5 are pure improvements (extract functions, remove casts, fix bug)
- Steps 6-8 unify initialization (the missing piece identified in this analysis)
- Steps 9-11 are organizational (names, files, tests)

Each step is independently committable and testable. If we stop at any point, the
codebase is strictly better than before. No step depends on completing all later steps.

If at any point we decide the macro-organization needs to evolve (e.g., the referee
pattern changes, the pipeline shape changes), none of these steps become wasted work:
- Pure functions are always useful
- Clean ownership is always useful
- Reset recipes are always useful
- Good names are always useful
- Fewer duplicated code paths are always useful
