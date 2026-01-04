# PNB Refactoring Strategy: The Pipeline Vision

## Philosophy: The Linear Functional Loop

Our goal is to transform the game loop from a web of mutable state and hidden side effects into a **strictly synchronous, phase-based functional pipeline**.

**The Zen Ideal:**
`State_Next = Resolution(Judgment(Simulation(State_Current, Input)))`

### The Target Pipeline (Frame N)

1.  **Input Phase:** Gather Raw Input (Keys) → `UserIntent` / `AI_Intent`
2.  **Simulation Phase:** `CurrentState` + `Intents` → `PhysicsResult` (New positions/velocities)
3.  **Judgment Phase (Referee):** `PhysicsResult` → `RefereeDecisions` (Outs, Runs, Rules)
4.  **Resolution Phase:** `RefereeDecisions` → `NewState` (Apply scores, reset positions)
5.  **Render Phase:** `NewState` → `Pixels`

This is pure, deterministic, and beautiful. No "messages" flying around; just data structs being transformed.

---

## Milestone Roadmap (The Path to Zen)

We are currently at **Milestone 10**. Here is the roadmap to the final architecture.

### 🏁 Milestone 10: Stabilization & Cleanup (CURRENT)
**Goal:** The "Clean Slate." No hidden state, type safety everywhere.
*   **Focus:** Eliminate `action_state.c` globals (move to `LocalGameInfo`), enforce `const` correctness, finish Enums.
*   **Why:** We cannot pass state through a pipeline if half of it is hidden in static variables.
*   **Result:** The entire game state is strictly contained in `StateInfo`.

### 🔮 Milestone 11: The Great Decoupling (Read vs. Write)
**Goal:** Split logic into "Query" and "Apply" halves.
*   **The Problem:** Currently, `game_analysis` checks for outs AND applies them.
*   **The Fix:**
    *   Split `game_analysis` into `check_rules()` (Read-only) and `apply_rules()` (Write).
    *   Split `action_implementation` into `calculate_physics()` (Read-only) and `apply_physics()` (Write).
*   **Zen:** "To judge is not to execute."

### 🔮 Milestone 12: The Intent Phase
**Goal:** Extract Input/AI into a dedicated initial phase.
*   **The Problem:** Actions happen instantaneously when keys are pressed.
*   **The Fix:**
    *   Introduce `UserIntent` struct (e.g., `INTENT_SWING`, `INTENT_THROW`).
    *   Input system produces Intents. AI produces Intents.
    *   Simulation phase consumes Intents.
*   **Zen:** "Decide first, act second."

### 🔮 Milestone 13: The Referee (Judgment Phase)
**Goal:** Separate "The Laws of the Game" from "The Physics of the World."
*   **The Fix:** Create the pure `Referee` module.
*   **Flow:** `Referee(PhysicsResult) -> Decisions`.
*   **Zen:** "The Referee observes the physical world and outputs judgments, but touches nothing."

### 🔮 Milestone 14: The Resolver (Resolution Phase)
**Goal:** Centralize state mutation.
*   **The Fix:** Create the `Resolver` module.
*   **Flow:** `Resolver(Decisions) -> NewState`.
*   **Zen:** "Only one hand moves the pieces."

### 🔮 Milestone 15: The Pipeline Integration
**Goal:** Stitch it all into `game_screen.c`.
*   **The Fix:** Rewrite the main game loop to explicitly follow the 5-phase pipeline.
*   **Zen:** The code looks like the diagram.

---

## Current Status: Milestone 10 (Stabilization)

**Foundation Quality:** 9.0/10 - Strong Types, Clean Data Structures.
**Current Focus:** Eliminating Static Globals.

**Tasks:**
1.  **Eliminate Action Globals:** Move `meterCounter`, `throwGoingOn`, etc. to `LocalGameInfo`.
2.  **Enum-ify Remaining States:** `TeamControlMode`, `TeamSide`, `ReplacementState`.
3.  **Const-Correctness:** Lock down read-only parameters.