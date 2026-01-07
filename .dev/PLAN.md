# PNB Development Plan

## Current Phase: The Great Decoupling & Verification

We are in the midst of transforming the game logic from a set of coordinated "Managers" into a strict **Functional Pipeline**. We have successfully consolidated state; now we must separate **Logic (Read)** from **Mutation (Write)**.

**Reference:** See `docs/CURRENT_STATUS_JAN2026.md` for a detailed architectural snapshot.

---

## 📅 Roadmap

### ✅ Milestone 10-13: State Consolidation (Completed Jan 2026)
- **Result:** `StateInfo` is the single source of truth. `RefereeState` and `AIState` exist.
- **Key Win:** Integration tests for §33 (Outs) and §36 (Tuplahaava) are passing.

### 🚧 Milestone 14: The Great Decoupling (Current Focus)
- **Goal:** Split "Messy" coordinators into "Pure Logic" + "State Applicators".
- **Target Files:**
    - `src/game/game_analysis.c` → `Referee_Analyze` (Pure) + `Referee_Apply` (Impure).
    - `src/game/action_implementation.c` → `Physics_Solve` (Pure) + `Physics_Apply` (Impure).
- **Deliverable:** Unit tests that run `Referee_Analyze` without a full game loop.

### 🔮 Milestone 15: The "User Intent" Phase
- **Goal:** Decouple Input from Action.
- **Concept:** Input generates an `Intent` (e.g., `INTENT_SWING_BAT`). The Engine consumes `Intent`.
- **Why:** Enables Replays, AI-vs-AI testing, and potential Multiplayer.

### 🔮 Milestone 16: Comprehensive Rule Verification
- **Goal:** 100% Audit of `docs/SAANNOT.md`.
- **Method:** Create `test_scenario_*.c` for every major rule section (Interference, Fielder Positioning, Wrong Turn).

### 🔮 Milestone 17: The Functional Pipeline
- **Goal:** `main.c` loop becomes: `Input -> Intent -> Referee(Query) -> Resolver(Write) -> Render`.

---

## Technical Debt / Cleanup
- **Messy Folders:** `src/game/ai_messy/` and `actions_messy/` need to be emptied into `_pure` counterparts or centralized "Systems".
- **Globals:** `src/include/globals.h` is becoming too large. Consider splitting (carefully) into `domain_types.h` vs `engine_types.h`.

---

## Technical Debt / Cleanup
- Milestone 11 targets the elimination of all hidden state in `src/game`.
