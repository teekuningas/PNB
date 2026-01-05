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

We are currently at **Milestone 13.5**. Here is the roadmap to the final architecture.

### ✅ Milestone 10: Initial Stabilization (Complete)
**Goal:** The "Clean Slate."
*   **Results:** `action_state.c` globals eliminated, essential enums implemented, const-correctness sweep started.

### ✅ Milestone 11: The State Consolidation (Logic) (Complete)
**Goal:** Eliminate ALL logic-related `static` and global variables from `src/game`.
*   **Result:** `StateInfo` is the **only** source of truth for game logic.

### ✅ Milestone 12: The Rendering Unification (Complete)
**Goal:** Modernize rendering to match the menu system.
*   **Result:** Rendering is a service that consumes `StateInfo` and assets via a central `ResourceManager`.

### ✅ Milestone 13: Stabilization & Rule Decoupling (Complete)
**Goal:** Purify rule logic and stabilize safety mechanisms.
*   **Results:** Pure helpers for safety (§20, §36), `BaseID` enum standardization, `PitchResult` enum, and comprehensive integration tests.

### 🚧 Milestone 13.5: Comprehensive Rule Audit & Final Polish (CURRENT)
**Goal:** Perfect type safety and rulebook alignment.
*   **Tasks:** Full `BaseID` signature sweep, §SAANNOT edge cases, and additional scenario tests.

### 🔮 Milestone 14: The Great Decoupling (Read vs. Write)
**Goal:** Split logic into "Query" and "Apply" halves.
*   **The Fix:**
    *   Split `game_analysis` into `check_rules()` (Read-only) and `apply_rules()` (Write).
    *   Split `action_implementation` into `calculate_physics()` (Read-only) and `apply_physics()` (Write).
*   **Zen:** "To judge is not to execute."

### 🔮 Milestone 14: The Intent Phase
**Goal:** Extract Input/AI into a dedicated initial phase.
*   **The Fix:**
    *   Introduce `UserIntent` struct (e.g., `INTENT_SWING`, `INTENT_THROW`).
    *   Input system produces Intents. AI produces Intents.
*   **Zen:** "Decide first, act second."

### 🔮 Milestone 15: The Referee (Judgment Phase)
**Goal:** Separate "The Laws of the Game" from "The Physics of the World."
*   **The Fix:** Create the pure `Referee` module.
*   **Flow:** `Referee(PhysicsResult) -> Decisions`.

### 🔮 Milestone 16: The Resolver (Resolution Phase)
**Goal:** Centralize state mutation.
*   **The Fix:** Create the `Resolver` module.
*   **Flow:** `Resolver(Decisions) -> NewState`.

### 🔮 Milestone 17: The Pipeline Integration
**Goal:** Stitch it all into `game_screen.c`.
*   **The Fix:** Rewrite the main game loop to explicitly follow the 5-phase pipeline.
