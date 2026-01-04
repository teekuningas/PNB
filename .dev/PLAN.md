# PNB Development Plan

## Current Phase: Stabilization & Cleanup

We are preparing the codebase for a major architectural shift towards a **Functional Pipeline**.

---

## 📅 Roadmap

### ✅ Milestone 9: Type Safety & State Machines (Completed 2026-01-04)
- Strong enums for Animations, Pitching, and Actions.

### 🚧 Milestone 10: Stabilization & Cleanup (CURRENT)
- **Goal:** Centralize all state. Eliminate hidden static globals.
- **Tasks:**
    - [ ] Move action globals to `LocalGameInfo`.
    - [ ] Enum-ify `TeamControlMode`, `TeamSide`, `ReplacementState`.
    - [ ] Enforce `const` correctness.

### 🔮 Milestone 11: The Great Decoupling (Read vs. Write)
- **Goal:** Split `game_analysis` and `action_implementation` into Query/Apply pairs.
- **Why:** Essential for phase-based execution.

### 🔮 Milestone 12: The Intent Phase
- **Goal:** Explicit `UserIntent` struct decoupled from immediate execution.

### 🔮 Milestone 13: The Referee (Judgment Phase)
- **Goal:** Pure function `Referee(State) -> Decisions`.

### 🔮 Milestone 14: The Resolver (Resolution Phase)
- **Goal:** Centralized state mutation `Resolver(Decisions) -> NewState`.

### 🔮 Milestone 15: The Pipeline Integration
- **Goal:** Explicit linear game loop: `Input -> Sim -> Judge -> Resolve -> Render`.

---

## Technical Debt / Cleanup
- `action_state.c` is the primary target for elimination in M10.