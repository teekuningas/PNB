# PNB Project Status Report - January 7, 2026 (Updated)

## 1. Executive Summary

**Current Phase:** Milestone 14 - "The Great Decoupling"
**Overall Health:** ⭐⭐⭐⭐ (Good)
**Architecture Trend:** Moving from Monolithic/Stateful → Functional/Pipeline

The project has successfully completed a massive "State Consolidation" phase. All hidden static variables and global state have been moved into a single `StateInfo` hierarchy. We are now in the process of strictly separating **Pure Logic** (Read-Only) from **State Mutation** (Write) and **Coordination**.

---

## 2. Architecture & Subsystems Analysis

### A. Data Structures (The Source of Truth)
**Location:** `src/include/globals.h`
**Status:** ✅ **Consolidated**

We have a clear "God Struct" `StateInfo` that holds everything. Key sub-structures include:
*   **`LocalGameInfo`**: The active match state.
*   **`RefereeState`**: **CRITICAL**. Holds the "truth" for rules. Tracks:
    *   `baseAtPitchStart`: Where players were when the pitch happened (vital for Foul Play resets).
    *   `woundingType`: Explicit tracking of "Tuplahaava" vs "Normal Wound".
*   **`AIState`**: All AI timers, counters, and decision flags.
*   **`GameFlowState`**: Counters for innings, outs, and game phases.
*   **`PendingActionState`**: Physics and input buffering.

### B. The Rules Engine (§SAANNOT.md Mapping)
**Location:** `src/game/rules_pure/` & `src/game/game_analysis.c`
**Status:** 🚧 **Hybrid**

*   **Pure Logic (✅):** We have excellent pure functions for:
    *   **§33 Pesäkilpa (Outs):** `rules_outs.c`
    *   **§36 Koppilyönti (Tuplahaava):** `base_logic.c` / `RefereeState`
    *   **§40 Force Play (Pakkovaihto):** Verified via `test_scenario_force_play.c`.
    *   **§41/42 Runs:** `rules_runs.c`
*   **Impure Coordinator (⚠️):** `game_analysis.c` is still a large "check everything" function that mixes reading state and applying outcomes.
*   **Coverage Gaps (CONFIRMED):**
    *   **§31 (Fielder Positioning):** **MISSING.** `test_scenario_fielder_positioning.c` confirms no check exists for fielders being out of bounds at pitch time.
    *   **§22 (Interference/Estäminen):** partially covered by `test_scenario_foul_play.c`, but complex geometries need work.

---

## 3. Test Suite Analysis

**Location:** `tests/`
**Status:** ⭐⭐⭐ (Strong on Logic, Weak on System)

### What We Have:
1.  **Unit Tests:** Good coverage for `rules_pure` and `ai_pure`.
2.  **Integration Scenarios (`tests/integration/`):** **Excellent** coverage for complex rule interactions (§33, §36, §22, §42).

### What Is Missing:
1.  **"Whole Game" Tests:** We lack a test that plays a full inning (AI vs AI).
2.  **State Consistency Validation:** No automated checks for invalid states (e.g., two players on one base).

---

## 4. The Plan (Short, Mid, Long Term)

### Short Term (Immediate - The "Decoupling")
*   **Objective:** Complete Milestone 14.
*   **Tasks:**
    1.  **Refactor `game_analysis.c`:** Split into `Referee_Analyze` (Pure) + `Referee_Apply` (Impure).
    2.  **Refactor `action_implementation.c`:** Separate "Input -> Physics Params" from "Physics Params -> State Update".
    3.  **Implement `StateValidator`:** A tool to check state consistency and dump JSON on failure.

### Mid Term (The "Pipeline")
*   **Objective:** Establish the Linear Game Loop.
*   **Tasks:**
    1.  **Implement `UserIntent`:** A struct capturing *what* a user wants to do.
    2.  **Pure Referee:** A unified function that takes `State + Intent` and returns `outcomes`.

### Long Term (The "Platform")
*   **Objective:** Polished Game & Tooling.
*   **Tasks:**
    1.  **Replay System:** Save/load `StateInfo` ticks.
    2.  **Network Play:** Prerequisite is pure Intent/Resolution.

---

## 5. Immediate Action Items

1.  **Coding:** Begin the split of `game_analysis.c`.
2.  **Tooling:** Create `src/core/state_validator.c`.