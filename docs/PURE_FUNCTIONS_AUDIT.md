# Pure Functions Audit & Cleanup Plan

## 📊 Function Call Site Analysis

### Rules Pure Functions

| Function | Lines | Calls | Status | Action |
|----------|-------|-------|--------|--------|
| **is_runner_forced_out** | 27 | 1 | ✅ | Keep - Complex logic, well-tested |
| **is_regular_run** | 7 | 2 | ✅ | Keep - Clear rule (§41) |
| **is_run_of_honor** | 14 | 2 | ✅ | Keep - Clear rule (§42) |
| **should_change_batter_on_strikes** | 5 | 1 | ⚠️ | **INLINE** - Trivial check |
| **determine_pitch_result** | 12 | 0 | ❌ | **DELETE** - Unused! |
| **player_is_safe_from_fly** | 20 | 1 | ✅ | Keep - Complex wounding logic |
| **get_active_batter_index** | 18 | 5 | ✅ | Keep - Used multiple places |
| **get_base_controller** | 23 | 7 | ✅ | Keep - Complex safety logic |
| **get_ball_at_base_index** | 35 | 2 | ✅ | Keep - Complex geometry |

### Base Logic Functions (base_logic.c)

| Function | Lines | Calls | Status | Action |
|----------|-------|-------|--------|--------|
| **base_get_next** | 7 | 4 | ✅ | Keep - Used multiple places |
| **base_get_prev** | 6 | 0 | ❌ | **DELETE** - Unused |
| **base_is_safe_haven** | 2 | 0 | ❌ | **DELETE** - Unused |
| **base_is_index** | 2 | 1 | ⚠️ | Used internally only |
| **base_can_advance** | 2 | 0 | ❌ | **DELETE** - Unused |
| **base_is_at_least** | 2 | 0 | ❌ | **DELETE** - Unused |
| **base_cmp** | 4 | 0 | ❌ | **DELETE** - Unused |
| **base_to_int_index** | 5 | 3 | ✅ | Keep - Used multiple places |
| **player_is_protected** | 3 | 1 | ⚠️ | **INLINE** - Trivial enum check |
| **count_active_batting_players** | ? | 2 | ✅ | Keep - Non-trivial logic |
| **checkIfBallIsOutOfBounds** | ? | 2 | ✅ | Keep - Geometry calculation |
| **is_run_of_honor_possible** | ? | 2 | ✅ | Keep - Rule logic |

### Action Pure Functions (Physics)

| Function | Calls | Status | Notes |
|----------|-------|--------|-------|
| **calculate_batted_ball_velocity** | 0 | ❌ | **DELETE** - Unused complex physics! |
| **calculate_batting_vertical_angle** | ? | ✅ | Keep (used in batting_system.c) |
| **calculate_pitch_power** | ? | ✅ | Keep (used in pitching_system.c) |
| **calculate_pitch_angle** | ? | ✅ | Keep (used in pitching_system.c) |

### AI Pure Functions

| Function | Calls | Status | Notes |
|----------|-------|--------|-------|
| **should_ai_drop_ball** | 0 | ❌ | **DELETE** - Well-tested but UNUSED! |
| **calculate_batting_strategy** | ? | ✅ | Keep (AI core) |
| **should_ai_throw** | ? | ✅ | Keep (AI core) |

---

## 🗑️ **DELETE: Unused Functions**

### Critical Findings - Well-Tested but UNUSED:

#### 1. `calculate_batted_ball_velocity` (batting_physics.c)
- **Lines:** 20 (complex physics)
- **Tests:** 1 test in test_batting_physics.c
- **Calls:** 0 ❌
- **Reason:** Replaced by inline physics in batting_system.c
- **Action:** DELETE both function AND test

#### 2. `should_ai_drop_ball` (catching_ai_strategy.c)
- **Lines:** 13 (tactical drop logic)
- **Tests:** 1 test in test_catching_ai_strategy.c  
- **Calls:** 0 ❌
- **Reason:** AI doesn't use tactical drop feature yet
- **Action:** DELETE both function AND test (can restore when AI improved)

#### 3. `determine_pitch_result` (rules_strikes.c)
- **Lines:** 12 (strike/ball determination)
- **Tests:** 0
- **Calls:** 0 ❌
- **Reason:** Logic duplicated in batting_system.c
- **Action:** DELETE function

### Base Logic Dead Code:

#### 4. `base_get_prev` - Unused
#### 5. `base_is_safe_haven` - Unused  
#### 6. `base_can_advance` - Unused
#### 7. `base_is_at_least` - Unused
#### 8. `base_cmp` - Unused

**Action:** DELETE all 5 unused base functions

---

## ⚠️ **INLINE: Over-Abstracted Functions**

### 1. `should_change_batter_on_strikes`
```c
// Current (5 lines)
int should_change_batter_on_strikes(const HalfInningState* h) {
    return h->strikes >= 3;
}

// Inline to single call site in game_analysis.c:
if (stateInfo->match->halfInningState.strikes >= 3) { ... }
```
**Reason:** Trivial check, single call site, test overhead not worth it

### 2. `player_is_protected`
```c
// Current (3 lines)
bool player_is_protected(PlayerUnitState state) {
    return (state == PLAYER_STATE_ON_BASE || state == PLAYER_STATE_AT_BAT);
}

// Inline to single call site in referee.c:
int is_protected = (state == PLAYER_STATE_ON_BASE || state == PLAYER_STATE_AT_BAT);
```
**Reason:** Simple enum check, single call site

---

## ✅ **KEEP: Valuable Functions**

### Complex Rule Logic (High Value)
- ✅ `is_runner_forced_out` - Complex conditions, well-tested
- ✅ `is_regular_run` - Clear rule (§41), multiple calls
- ✅ `is_run_of_honor` - Clear rule (§42), multiple calls  
- ✅ `player_is_safe_from_fly` - Complex wounding rule

### Used Multiple Places
- ✅ `get_active_batter_index` - 5 calls
- ✅ `get_base_controller` - 7 calls (complex safety logic)
- ✅ `base_get_next` - 4 calls
- ✅ `base_to_int_index` - 3 calls

### Non-Trivial Logic
- ✅ `get_ball_at_base_index` - Complex geometry
- ✅ `count_active_batting_players` - Non-trivial loop
- ✅ `checkIfBallIsOutOfBounds` - Geometry calculation
- ✅ `is_run_of_honor_possible` - Multiple conditions

### Physics (Used in System Code)
- ✅ All pitching_physics functions (used in pitching_system.c)
- ✅ Most batting_physics functions (except `calculate_batted_ball_velocity`)

### AI Strategy
- ✅ `calculate_batting_strategy` - Core AI
- ✅ `should_ai_throw` - Core AI
- ✅ Other AI functions (except `should_ai_drop_ball`)

---

## 📋 Cleanup Checklist

### Phase 1: Delete Unused Code (Immediate)
- [ ] Delete `calculate_batted_ball_velocity` + test
- [ ] Delete `should_ai_drop_ball` + test  
- [ ] Delete `determine_pitch_result`
- [ ] Delete 5 unused base_logic functions
- [ ] Remove corresponding test cases
- [ ] Update headers

**Impact:** -100 lines code, -2 tests, 0 coverage loss

### Phase 2: Inline Trivial Functions (Next Session)
- [ ] Inline `should_change_batter_on_strikes` (1 call site)
- [ ] Inline `player_is_protected` (1 call site)
- [ ] Remove corresponding tests
- [ ] Update headers

**Impact:** -15 lines code, -2 tests, clearer code

### Phase 3: Document Remaining Functions
- [ ] Add docstrings to all kept functions
- [ ] Explain parameters and return values
- [ ] Link to Pesäpallo rules where applicable

---

## 📊 Impact Summary

### Before Cleanup:
- **Pure function files:** 8 files
- **Pure functions:** ~35 functions
- **Lines:** ~400 lines
- **Tests:** 14 test files, 51 tests
- **Unused functions:** 10+ (29% waste!)

### After Cleanup:
- **Pure function files:** 8 files (same)
- **Pure functions:** ~23 functions (-12)
- **Lines:** ~285 lines (-115)
- **Tests:** 12 test files, 47 tests (-4)
- **Unused functions:** 0 (0% waste!)

**Efficiency gain:** +40% signal-to-noise ratio

---

## 🎯 Key Insights

### What We Learned:

1. **Over-abstraction:** Small functions with 1 call site are noise
2. **Dead code exists:** Even well-tested functions can be unused!
3. **Test without usage:** `should_ai_drop_ball` is tested but never called
4. **Base utilities:** Most base_* helper functions are unused

### Naming Audit:

✅ **Good Names:**
- `is_runner_forced_out` - Clear, describes rule
- `get_active_batter_index` - Clear getter
- `calculate_batted_ball_velocity` - Clear physics (shame it's unused!)

⚠️ **Questionable Names:**
- `player_is_protected` - Protected from what? (Safe on base)
- `base_is_safe_haven` - Same as `base_is_index`? Confusing

✅ **Overall:** Naming is generally good

---

## 🎓 Recommendations

### Immediate Actions:
1. **Delete 10 unused functions** - Biggest impact, no downside
2. **Delete 2 unused tests** - Reduces maintenance
3. **Inline 2 trivial functions** - Improves clarity

### Future Considerations:
1. **AI Features:** If tactical drop added, restore `should_ai_drop_ball`
2. **Physics Refactor:** Keep physics functions for Milestone 18
3. **Rule Expansion:** Pure rule functions are foundation - keep all used ones

### Philosophy:
**"Every function should earn its keep. If it's not called, delete it. If it's trivial, inline it. If it's complex, test it well."**

---

## ✅ Final Verdict

**Pure functions are generally well-designed.** The main issue is **dead code** (~29% unused). After cleanup:
- Clearer codebase
- Less test maintenance  
- Same coverage
- Better signal-to-noise

**Action:** Proceed with cleanup - low risk, high reward! 🎯
