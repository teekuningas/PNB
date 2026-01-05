# Stabilization Audit - January 5, 2026

## Overview
During the logic state consolidation (Milestone 11), several bugs were identified and fixed regarding the relationship between game rules (outs/strikes) and physical player movement. This audit documents the decoupling of these systems and the restoration of automatic triggers for unambiguous player control.

## Bugs Identified & Fixed

### 1. The Overlapping Batter Bug
**Issue:** When a batter reached 3 strikes, the system repeatedly set `batterIndex = -1` every frame. This triggered the "Select Batter" UI while the previous batter was still at the plate, leading to multiple overlapping batter models and a blocked pitcher.
**Fix:** 
- Reset `gameState.strikes = 0` immediately upon the 3rd strike.
- Removed the immediate `batterIndex = -1` assignment in `strikesAndBalls`. The batter remains the "active" player until they reach a base or are outed.

### 2. Immediate Out on Selection
**Issue:** New batters were being outed immediately upon selection if the ball was near home. This happened because they were in `PLAYER_STATE_AT_BAT`, which wasn't recognized as a "safe" state by the rule engine.
**Fix:**
- Updated `checkForOuts` and `woundingCatchEffects` to recognize `PLAYER_STATE_AT_BAT` as a safe state.
- Ensured `safeOnBaseIndex[0]` is assigned immediately in `selectBatter`.

## Architectural Changes: Rule-Movement Decoupling

### Safety-Based Triggers
We decoupled the *rules* from the *physical movement*, but linked them via `safeOnBaseIndex`. Movement is now triggered by the **loss of safety**:

1.  **Three Strikes (§18):** 
    - Rule: Batter loses safety at Home.
    - Implementation: `safeOnBaseIndex[0]` set to -1.
    - Trigger: `runToNextBase` automatically moves the player to 1st.
2.  **Chain Reaction (§20):**
    - Rule: Only the most recent arrival is safe on a base.
    - Implementation: `safeOnBaseIndex[base]` updated to the newcomer.
    - Trigger: The previous occupant (displaced) automatically triggers `runToNextBase`.
3.  **Fielder Force (§33):**
    - Rule: Leading runner loses safety if ball arrives at previous base.
    - Implementation: `safeOnBaseIndex[i]` set to -1.
    - Trigger: `runToNextBase` forces the runner forward.

### Unambiguous Control
This decoupling preserves the base-key control scheme. By ensuring only one player is safe per base, the game always knows which player to map to the `KEY_LEFT`, `KEY_RIGHT`, etc., controls.

## Verification
- **Unit Tests:** All 53 tests passing.
- **Manual Verification:** Verified that 3 strikes correctly forces the batter to run and allows the pitcher to continue after the next batter arrives. Verified that arriving at an occupied base correctly displaces the previous runner.

## Status
Codebase is stabilized. Ready for Milestone 12 (Rendering) and Milestone 13 (Further Rule Purification).
