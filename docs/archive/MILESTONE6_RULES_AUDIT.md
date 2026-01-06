# Complete Rules Extraction Audit

## Summary
Compared all three extracted rule functions against original logic from commit bbb146b1.

---

## 1. rules_outs.c - is_runner_forced_out()

### ❌ **BUG FOUND**

**Original Logic:**
```c
if(i > 0) baseIndex = i - 1;  // Calculate previous base
else baseIndex = 3;

if(stateInfo->localGameInfo->playerInfo[index].bTPI.base == baseIndex) {
    if(stateInfo->localGameInfo->playerInfo[index].bTPI.isOnBase == 0) {
        if(stateInfo->localGameInfo->playerInfo[index].bTPI.takingFreeWalk == 0 &&
           stateInfo->localGameInfo->gAI.outOfBounds == 0) {
            // OUT!
```

**Current (BROKEN) Call:**
```c
if(is_runner_forced_out(
    stateInfo->localGameInfo->playerInfo[index].bTPI.base,
    stateInfo->localGameInfo->playerInfo[index].bTPI.isOnBase,
    i,  // ← BUG! Should be baseIndex
    stateInfo->localGameInfo->playerInfo[index].bTPI.takingFreeWalk,
    stateInfo->localGameInfo->gAI.outOfBounds)) {
```

**Root Cause:**
- `player.base` represents "where player is AT or running FROM" (not TO)
- When running from base 0 to base 1, `player.base` stays 0 until arrival
- Original compared `player.base == baseIndex` (i.e., 0 == 0) ✓
- New compares `player.base == i` (i.e., 0 == 1) ✗

**Fix Required:**
Line 158 in `src/game/game_analysis.c` must pass `baseIndex` not `i`:
```c
if(is_runner_forced_out(
    stateInfo->localGameInfo->playerInfo[index].bTPI.base,
    stateInfo->localGameInfo->playerInfo[index].bTPI.isOnBase,
    baseIndex,  // ← CORRECTED
    stateInfo->localGameInfo->playerInfo[index].bTPI.takingFreeWalk,
    stateInfo->localGameInfo->gAI.outOfBounds)) {
```

**Function Signature Update Needed:**
The parameter name is misleading. Update from:
```c
int is_runner_forced_out(int player_base, int player_is_on_base_flag, 
                        int ball_at_target_base_index, int taking_free_walk, 
                        int is_out_of_bounds)
```
To:
```c
int is_runner_forced_out(int player_base, int player_is_on_base_flag, 
                        int ball_at_base_index, int taking_free_walk, 
                        int is_out_of_bounds)
```

And update the comment to clarify:
```c
/**
 * @param ball_at_base_index The index of the base where the ball currently is.
 *        NOTE: This is the SAME base the player is running FROM (i.e., i-1 in the loop,
 *        or baseIndex in the original code). NOT the base the player is running TO.
 */
```

---

## 2. rules_runs.c - calculate_runs()

### ✅ **CORRECT**

**Original Logic:**
```c
if(stateInfo->localGameInfo->playerInfo[index].bTPI.base == 4) {
    if (wounded == 0) {
        // Score run
    }
}
else if(stateInfo->localGameInfo->playerInfo[index].bTPI.base == 3 &&
        stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase == 0 &&
        stateInfo->localGameInfo->gAI.canMakeRunOfHonor == 1) {
    if(stateInfo->localGameInfo->playerInfo[index].bTPI.wounded == 0 &&
       stateInfo->localGameInfo->playerInfo[index].bTPI.hasMadeRunOnThirdBase == 0) {
        // Score run of honor
    }
}
```

**Current Call:**
```c
int runScored = calculate_runs(
    stateInfo->localGameInfo->playerInfo[index].bTPI.base,
    stateInfo->localGameInfo->playerInfo[index].bTPI.originalBase,
    stateInfo->localGameInfo->playerInfo[index].bTPI.wounded,
    stateInfo->localGameInfo->gAI.canMakeRunOfHonor,
    stateInfo->localGameInfo->playerInfo[index].bTPI.hasMadeRunOnThirdBase);
```

**Verification:**
- All 5 parameters match original conditions exactly ✓
- Logic order preserved ✓
- Edge cases covered (wounded, hasMadeRunOnThirdBase) ✓

---

## 3. rules_strikes.c - should_change_batter_on_strikes()

### ✅ **CORRECT**

**Original Logic:**
```c
if(stateInfo->localGameInfo->gAI.strikes == 3) {
    if(stateInfo->localGameInfo->pII.baseControlIndex[0] != -1) {
        // Force batter to run
    }
}
```

**Current Call:**
```c
if(should_change_batter_on_strikes(
    stateInfo->localGameInfo->gAI.strikes, 
    stateInfo->localGameInfo->pII.baseControlIndex[0])) {
    if(stateInfo->localGameInfo->pII.baseControlIndex[0] != -1) {
        // Force batter to run
    }
}
```

**Verification:**
- Both parameters match original conditions exactly ✓
- Logic preserved: strikes == 3 AND baseControlIndex[0] != -1 ✓

---

## Action Items

1. ✅ Fix `is_runner_forced_out()` call in `game_analysis.c` (line 158: `i` → `baseIndex`)
2. ✅ Update `rules_outs.h` parameter name for clarity
3. ✅ Update `rules_outs.c` comment to document the baseIndex semantics
4. ✅ Add regression test for the out detection bug
5. ✅ Re-run all tests to verify fix
