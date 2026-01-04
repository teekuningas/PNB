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

### ✅ Milestone 11: The State Consolidation (Logic) (Completed 2026-01-04)
- **Goal:** Eliminate ALL logic-related `static` and global variables from `src/game`.
- **Results:**
    - Moved AI statics into `AIState`.
    - Moved game flow counters into `GameFlowState`.
    - Moved action system internal variables into `PendingActionState`.
    - `StateInfo` is now the only source of truth for game logic.

### 🚧 Milestone 12: The Rendering Unification (CURRENT)
- **Goal:** Modernize in-game rendering to match the Menu system.
- **Tasks:** Adopt `ResourceManager`, eliminate orphan `GLuint` globals, and unify GL context setup.

### 🔮 Milestone 13: The Great Decoupling (Read vs. Write)
- **Goal:** Split `game_analysis` and `action_implementation` into Query/Apply pairs.
- **Why:** Essential for phase-based execution.

### 🔮 Milestone 14: The Intent Phase
- **Goal:** Explicit `UserIntent` struct decoupled from immediate execution.

### 🔮 Milestone 15: The Referee (Judgment Phase)
- **Goal:** Pure function `Referee(State) -> Decisions`.

### 🔮 Milestone 16: The Resolver (Resolution Phase)
- **Goal:** Centralized state mutation `Resolver(Decisions) -> NewState`.

### 🔮 Milestone 17: The Pipeline Integration
- **Goal:** Explicit linear game loop: `Input -> Sim -> Judge -> Resolve -> Render`.

---

## Technical Debt / Cleanup
- Milestone 11 targets the elimination of all hidden state in `src/game`.
