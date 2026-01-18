# Next Session Instructions

**Date:** January 17, 2026
**Status:** Phase 2B Cleanup In Progress (Test cleanup done, Struct split next)

## 📍 Where We Are
We have just finished cleaning up the test suite and inlining trivial functions. The codebase is stable with 48 passing tests. We clarified the distinction between Referee timers (decisions) and Flow timers (UX).

## 🎯 Immediate Goal: Split GameControl
The `GameControl` struct is a hybrid of Referee logic (pitch state) and System logic (flow control). We need to split it.

### Step-by-Step Plan

1.  **Rename & Prepare:**
    - [ ] Rename `GameControl` fields in `globals.h` with `pitch_` and `flow_` prefixes (e.g., `catchHasBeenMade` -> `pitch_catchHasBeenMade`).
    - [ ] Apply these renames across the codebase (`referee.c`, `game_analysis.c`, etc.).

2.  **Create New Structs:**
    - [ ] Define `PitchState` in `globals.h` (Referee-owned, reset every pitch).
    - [ ] Define `FlowControl` in `globals.h` (System-owned, persistent).
    - [ ] Update `MatchSession` to replace `GameControl` with these two new structs.

3.  **Migrate Logic:**
    - [ ] Update `common_logic.c` to initialize/reset `PitchState` correctly at pitch start.
    - [ ] Update `referee.c` to write to `PitchState`.
    - [ ] Update `game_analysis.c` to use `FlowControl`.

4.  **Documentation:**
    - [ ] Create `docs/REFEREE_PATTERN.md` (Crucial: Document Timer distinction).
    - [ ] Document `referee.c` with section headers.

## 📝 Resources
- **Plan:** `.dev/PLAN.md` (Updated with correct timer strategy)
- **Summary:** `docs/SESSION_SUMMARY_2026_01_17_PART1.md`