# PNB Development Plan

## Current Phase: Stabilization & Cleanup

We are preparing the codebase for a major architectural shift towards a **Functional Pipeline**.

---

## 📅 Roadmap

### ✅ Milestone 9: Type Safety & State Machines (Completed 2026-01-04)
- Strong enums for Animations, Pitching, and Actions.

### ✅ Milestone 10: Initial Stabilization (Completed 2026-01-04)
- Centralize `action_state.c` globals into `LocalGameInfo`.
- Enum-ify `TeamControlMode`, `TeamSide`, `ReplacementState`, etc.
- Initial const-correctness sweep.

### ✅ Milestone 11: The State Consolidation (Logic) (Completed 2026-01-05)
- **Goal:** Eliminate ALL logic-related `static` and global variables from `src/game`.
- **Results:**
    - Moved AI statics into `AIState`.
    - Moved game flow counters into `GameFlowState`.
    - Moved action system internal variables into `PendingActionState`.
    - `StateInfo` is now the only source of truth for game logic.

### ✅ Milestone 12: The Rendering Unification (Completed 2026-01-05)
- **Goal:** Modernize in-game rendering to match the Menu system.
- **Results:**
    - Adopted `ResourceManager` for all in-game textures and models.
    - Eliminated all logic-related `static` GL variables from `src/game`.
    - Unified rendering architecture between game and menus.

### ✅ Milestone 13: Stabilization & Rule Decoupling (Completed 2026-01-05)
- **Goal:** Purify rule logic and stabilize safety mechanisms before large-scale decoupling.
- **Results:**
    - Purified safety logic into `player_is_protected` helpers.
    - Standardized base indexing (consistent `BaseID` usage).
    - Implemented `PitchResult` enum.
    - Removed `initLocals` legacy counter.
    - Added comprehensive integration tests for §33 (Chain Reaction) and §36 (Tuplahaava).

### ✅ Milestone 13.5: Comprehensive Rule Audit & Referee State (Completed 2026-01-06)
- **Goal:** Ensure 100% alignment with §SAANNOT and implement central Referee state.
- **Results:**
    - **RefereeState:** Created explicit `RefereeState` struct to track rule-specific data (base snapshots, pending wounds).
    - **Snapshotting:** Implemented pitch-start snapshots for reliable foul play restoration and run validation.
    - **Explicit Logic:** Enumerated all "Tuplahaava" and "Wounding" cases explicitly in code.
    - **Legacy Cleanup:** Removed `originalBase`, `pendingWound`, `woundingSourceBase` from player structs.
    - **Tests:** Added comprehensive integration tests for complex scenarios (Tuplahaava exceptions, overtaking).

### 🚧 Milestone 14: The Great Decoupling (Read vs. Write) (CURRENT)
- **Goal:** Split logic into "Query" and "Apply" halves.
- **Why:** Essential for phase-based execution.

### 🔮 Milestone 15: The Intent Phase
- **Goal:** Explicit `UserIntent` struct decoupled from immediate execution.

### 🔮 Milestone 16: The Referee (Judgment Phase)
- **Goal:** Pure function `Referee(State) -> Decisions`.

### 🔮 Milestone 17: The Resolver (Resolution Phase)
- **Goal:** Centralized state mutation `Resolver(Decisions) -> NewState`.

### 🔮 Milestone 18: The Pipeline Integration
- **Goal:** Explicit linear game loop: `Input -> Sim -> Judge -> Resolve -> Render`.

---

## Technical Debt / Cleanup
- Milestone 11 targets the elimination of all hidden state in `src/game`.
