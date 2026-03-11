# The Great Unentanglement: A Bold Plan for Horizontal Refactoring

## 1. The Epiphany: Vertical Slices vs. Horizontal Sorting

For months, we have been painstakingly untangling the game loop using **Vertical Slicing**. We pick a single concept (like `waitingForBatterDecision` or `WOUNDED`), trace it through the entire codebase, untangle it from the "junk drawers" of `common_logic.c` and `game_manipulation.c`, and run 1000-frame scenario tests to ensure we didn't break physics.

It works, but it's grueling. It's "death by a thousand cuts."

The friction comes from a fundamental structural flaw: **Data without a defined lifecycle.** Structs like `PlayerRelatedActionInfo` (pRAI) and `FlowControl` are currently a dumping ground for three very different things:
1.  **Lightning Strikes (Transient Events):** `batHit`, `batMiss`, `refreshCatchAndChange`. These happen in a single frame and must die immediately.
2.  **State Machines (Persistent Status):** `pitchState`, `waitingForBatterDecision`. These persist across frames until a specific game rule changes them.
3.  **Visuals & Intent (UI/AI):** `meterValue`, `throwGoingToBase`. 

Because these lifecycles are mixed together, the code doesn't trust the data. `common_logic.c` is bloated with functions like `initializePRAIInformation` and `initializeTemporaryGameAnalysisInfo` that frantically wipe variables to zero "just in case" to prevent bugs. 

**The Bold Move:** We stop trying to fix the logic, and instead fix the **Memory Layout**. By sorting variables horizontally into structs that inherently enforce their lifecycle, we can delete hundreds of lines of defensive wiping logic. The Zen realization of Step 3.1—*the best code is no code*—becomes our primary weapon.

---

## 2. The Big Wins: How We Get Fast

### A. The `GameEvents` Nuke (Instant Cleanup)
We already have a `GameEvents` struct (from Milestone 16) that is wiped completely clean at the end of every single frame (`clearFrameEvents()`). 
*   **The Action:** We move `batHit`, `batMiss`, `batterCanAdvance`, and `refreshCatchAndChange` out of `pRAI` and directly into `GameEvents`.
*   **The Win:** They will now auto-destruct at the end of the frame. We can gleefully delete all the manual reset logic in `common_logic.c` and `referee.c` that tries to manage them. The architecture enforces the lifecycle automatically.

### B. The 1-Frame Superpower (Lightning-Fast Testing)
We have 15 massive scenario tests that run 500-1000 frames to verify the game plays correctly. They are our safety net, but they are terrible for iterating quickly. 
Because we are cementing the main loop and data ownership, we now have the capability to write **1-Frame Contract Tests**.
*   **The Action:** We write tests that set a single state, run `updateMutableWorld` for exactly 1 frame, and assert the immediate reaction.
*   **The Win:** When we move `batHit` to `GameEvents`, we write a 1-frame test to prove it clears. When we finalize the Referee -> Consolidation contract, we write a 1-frame test to prove Consolidation reacts instantly. These tests run in milliseconds and give us total confidence to make massive structural changes without fear of subtle 400-frame butterfly effects.

### C. Extracting the "Pure" Math
`common_logic.c` and `game_manipulation.c` are full of nested if-statements calculating things (like who the lead runner is, or rotating the batting team index).
*   **The Action:** Extract these formulas into pure, stateless functions in `src/game/rules_pure/`. 
*   **The Win:** Pure functions have zero global state. They take inputs and return an output. They are mathematically provable and can be unit tested instantaneously. By ripping them out of the messy execution flow, the remaining flow becomes dramatically simpler to read and refactor.

---

## 3. The Execution Roadmap

This plan gets us out of the forest and back to looking at the mountains. We will execute this in highly aggressive, but totally safe, structural passes:

**Phase 1: Cement the Transient Lifecycle (High Speed, High Impact)**
1.  Implement 1-Frame tests to lock in the behavior of `clearFrameEvents`.
2.  Move all one-frame event variables from `pRAI` (and anywhere else) into `GameEvents`. 
3.  Delete the now-redundant reset logic throughout the codebase.
4.  Run the 1000-frame scenario tests to verify safety.

**Phase 2: Extract Pure Domain Rules (Zero Risk)**
1.  Target formulas in `common_logic.c` (e.g., `calculateFreeWalk` lead-runner logic, `get_batting_team_index`).
2.  Move them to `rules_pure/`.
3.  Write comprehensive Unit Tests for them. 

**Phase 3: Isolate UI and Intent**
1.  Move `meterValue` and `swingMeterValue` from `pRAI` into `CameraState` or `UIState`.
2.  Move `throwGoingToBase` into `PendingActionState`.
3.  Now `pRAI` and `FlowControl` only contain *actual persistent state machine logic*. 

**Phase 4: Return to the Referee (With a Clear View)**
1.  With the noise gone, variables like `waitingForBatterDecision` and `freeWalkCalculationMade` will be isolated. 
2.  We easily resume the original "Phase 3" goals (Steps 3.2, 3.3) of moving state reactions entirely into Consolidation, making the Referee the ultimate, read-only decision-maker.

## Summary
By shifting from "tracing logic" to "sorting lifecycles", we weaponize the compiler and the main loop. Data structures will manage themselves. We will delete code rather than rewriting it. And with 1-Frame tests, we will move faster and safer than ever before.