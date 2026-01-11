# Referee Consolidation Plan

**Goal:** Decouple referee logic into a frame-independent monitoring system that is the sole authority on RefereeState and HalfInningState.

**Current Problem:**
*   Referee logic is split between `referee.c` (analysis) and `game_analysis.c` (state updates).
*   Referee relies on frame-perfect synchronization (e.g., `catchTimer`).
*   Physical and legal state are mixed.

**The Solution:**
1.  **Observes** transient physical events (`GameEvents`)
2.  **Updates** RefereeState and HalfInningState based on observations
3.  **Signals** persistent control flags (`GameControl`) for other systems to react

**The Golden Rule:**
4.  **One Way Flow:** Physics -> Events -> Referee -> Legal State -> Reconciliation -> Physics
5.  **Limited Scope** - Referee MUST NOT mutate other structures (e.g., `PlayerInfo`, `BallInfo`, `pRAI`, `AIState`). Its output is strictly the Legal State (`RefereeState`, `HalfInningState`, `GameControl`). Physical enforcement is the job of `reconcile`.

---

## Phase 1: Structural Reorganization & Event System (Done)

**Goal:** Create clean, semantically clear structures and establish robust event system.

### 1. Structure Cleanup (Before Logic Changes)
- [x] Rename `GameControlFlags` -> `GameEvents` (transient, cleared every frame).
- [x] Create `GameControl` struct (stateful, persisted flags).
- [x] Migrate fields to appropriate struct:
    - **GameEvents:** `catchMade`, `playerArrivedAtBase`
    - **GameControl:** `pause`, `catchHasBeenMade` (new persistent flag)
- [x] Create `HalfInningState` (formerly GameState)
    - Keep: `outs`, `strikes`, `balls`, `runsInTheInning`
    - Move `ballHome` -> `GameFlowState` (it's logic state, not score)

### 2. Event System Pattern
- **Producer (Physics/Actions):** Sets `gameEvents.eventX = 1`
- **Consumer (Referee):** Reads `gameEvents.eventX`, updates legal state.
- **Consumer (Reconcile):** Reads legal state, updates physics if needed.
- **Cleanup:** `clearFrameEvents()` called at end of frame.

---

## Phase 2: Referee Consolidation (In Progress)

**Goal:** Make referee.c the sole authority on `RefereeState` and `HalfInningState`.

### The Loop
1.  **Physics (`gameManipulation`, `actionImplementation`):** Updates positions, collisions. Writes to **GameEvents** (e.g., `catchMade`).
2.  **Referee (`Referee_Update`):** Reads **GameEvents** + Current State. Makes legal decisions (Out, Run, Strike, Ball). Updates **HalfInningState** and **RefereeState**. Sets **GameControl** flags if cleanup is needed (`pitchResolutionProcessed`).
3.  **Reconcile (`reconcileLegalAndPhysicalState`):** Reads **RefereeState** and **GameControl**. Updates Physical State to match Legal State (moves player if Out, resets `pitchState` if resolution processed).

### Step 1: Move Simple Logic
- [ ] Move strike/ball counting from `game_manipulation.c` to referee.
- [ ] Move `outOfBounds` flag management to referee.
- [ ] Move `endPeriod` flag setting to referee.
- [ ] Consolidate all `HalfInningState.event` setting in referee.

### Step 2: Event-Driven Wounding
- [ ] Replace frame counters in `game_analysis.c` with `RefereeState.woundingCatchActive` + timer.
- [ ] Referee observes `gameEvents.catchMade` -> starts timer.
- [ ] Referee observes timer expiry -> marks players wounded.

### Step 3: Delete game_analysis.c
- [ ] Move remaining initialization logic to `game_setup.c`.
- [ ] Delete `game_analysis.c`.

---

## Final State Architecture

### Ownership Map

| Structure | Owner (Writer) | Readers |
| :--- | :--- | :--- |
| GameEvents | Physics/Actions | Referee |
| RefereeState | **referee.c ONLY** | Reconcile, UI, AI |
| HalfInningState | **referee.c ONLY** | Everyone |
| GameControl | Referee (mostly) | Physics, AI |
| PlayerInfo | Physics/Actions | Referee (Read-only) |

### Key Migrations

**HalfInningState mutations OUTSIDE referee.c:**
- `game_manipulation.c`: Strikes, Balls (Must move)
- `game_analysis.c`: Outs (Must move)
- `action_implementation.c`: Runs (Must move)

**Goal:** Only referee.c writes to RefereeState/HalfInningState
**Goal:** Only game_analysis.c writes to Scoreboard (GlobalGameInfo) - *Wait, Scoreboard should be updated by Referee for runs? Yes.*
**Correction:** Referee updates `Scoreboard.teams[].runs`. `game_analysis.c` (or new flow controller) updates `Scoreboard.inning/period`.

---

## Verification Plan

### Tests
1.  **Unit Tests:** Test `Referee_Update` in isolation with mocked `GameEvents`.
2.  **Integration Tests:** Verify `catchMade` -> `catchHasBeenMade` -> `wounding` pipeline.

### Success Criteria
- ✅ `game_analysis.c` is deleted or empty.
- ✅ Only referee.c writes to HalfInningState
- ✅ Referee does not depend on `frame % x` logic directly (uses internal timers).
- ✅ Other systems write GameEvents (never write Referee/HalfInningState)
