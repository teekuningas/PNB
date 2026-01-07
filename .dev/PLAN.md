# PNB Development Plan

## Current Phase: Action System Decoupling

We have successfully completed Milestone 14 (Referee decoupling). Now targeting the action system: split `actions_messy/` into pure analysis + state mutation, following the same pattern as the referee.

**Reference:** See `docs/ARCHITECTURE.md` for technical details.

---

## 📅 Roadmap

### ✅ Milestone 10-13: State Consolidation (Completed Jan 2026)
- **Result:** `StateInfo` is the single source of truth. `RefereeState` and `AIState` exist.
- **Key Win:** Integration tests for §33 (Outs) and §36 (Tuplahaava) are passing.

### ✅ Milestone 14: The Great Decoupling (Completed Jan 2026)
- **Goal:** Split "Messy" coordinators into "Pure Logic" + "State Applicators".
- **Completed:**
    - `Referee_Analyze` (pure) + `Referee_Apply` (impure)
    - State validator with runtime checks
    - Eliminated `baseControlIndex` array
    - 67 tests passing (53 unit + 14 integration)

### 🚧 Milestone 15: Action System Decoupling (Current Focus)
- **Goal:** Apply same pattern to action system
- **Target Files:**
    - `actions_messy/batting_system.c` (473 LOC)
    - `actions_messy/pitching_system.c` (385 LOC)
    - `actions_messy/throwing_system.c` (250 LOC)
- **Pattern:** Split into `Action_Analyze` + `Action_Apply`

### 🔮 Milestone 16: The "User Intent" Phase
- **Goal:** Decouple Input from Action.
- **Concept:** Input generates an `Intent` (e.g., `INTENT_SWING_BAT`). The Engine consumes `Intent`.
- **Why:** Enables Replays, AI-vs-AI testing, and potential Multiplayer.

### 🔮 Milestone 17: Comprehensive Rule Verification
- **Goal:** 100% Audit of `docs/SAANNOT.md`.
- **Method:** Create `test_scenario_*.c` for every major rule section (Interference, Fielder Positioning, Wrong Turn).

### 🔮 Milestone 18: The Functional Pipeline
- **Goal:** `main.c` loop becomes: `Input -> Intent -> Referee(Query) -> Resolver(Write) -> Render`.

---

## Quick Wins (Optional)

### Merge Duplicate Wounding Tracking
**Problem:** Wounding is tracked in 3 places:
- `WoundingState.woundingCatch`
- `RefereeState.woundingCatchActive`
- `GameFlowState.woundingCatchCounter`

**Solution:** Consolidate into `RefereeState` (already the authority):
```c
RefereeState {
    int woundingCatchActive;
    int woundingCatchTimer;  // Add this, remove from GameFlowState
}
```
Delete `WoundingState` struct entirely.

**Effort:** 10 minutes, reduces confusion.

---

## Technical Debt / Cleanup (Post-M15)
- **State Structure:** After M15, split `LocalGameInfo` into layers (Domain, Coordination, Presentation)
- **Messy Folders:** `src/game/ai_messy/` and `actions_messy/` - target of M15
- **Globals:** `src/include/globals.h` (773 LOC) - consider splitting after pipeline is complete


