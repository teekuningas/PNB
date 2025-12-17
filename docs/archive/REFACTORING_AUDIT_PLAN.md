# Systematic Refactoring Audit Plan

## Objective
Verify that ALL structural refactorings preserved original logic faithfully. The `rules_outs` bug showed that even with passing tests, subtle parameter mismatches can break functionality.

---

## Lesson Learned from rules_outs Bug

**What we thought we were testing:**
- Pure function logic with correct inputs ✓

**What we missed:**
- Integration layer passing WRONG inputs ✗
- Semantic meaning of state variables (player.base = FROM not TO)
- Context lost during extraction (baseIndex calculation)

**Key Insight:** We need to audit THE CALL SITES, not just the pure functions!

---

## Audit Scope

### Phase 1: Actions (Milestone 5)
| Module | Extracted Function | Commit | Before Commit | Status |
|--------|-------------------|--------|---------------|--------|
| batting_physics.c | `calculate_hit_trajectory()` | 826131d | cf7780a | ⏳ TODO |
| batting_physics.c | `calculate_batting_meter()` | 826131d | cf7780a | ⏳ TODO |
| pitching_physics.c | `calculate_pitch_power()` | e8556b0 | 20a4ae5 | ⏳ TODO |
| pitching_physics.c | `calculate_pitch_trajectory()` | e8556b0 | 20a4ae5 | ⏳ TODO |

### Phase 2: AI (Milestone 5)
| Module | Extracted Function | Commit | Before Commit | Status |
|--------|-------------------|--------|---------------|--------|
| batting_ai_strategy.c | `should_ai_attempt_hit()` | 2cba8f4 | c1e26d7 | ⏳ TODO |
| batting_ai_strategy.c | `calculate_ai_batting_angle()` | 2cba8f4 | c1e26d7 | ⏳ TODO |
| catching_ai_strategy.c | `calculate_ai_movement_keys()` | 338f277 | 980eceb | ⏳ TODO |
| catching_ai_strategy.c | `should_ai_throw_ball()` | 338f277 | 980eceb | ⏳ TODO |
| catching_ai_strategy.c | `determine_lead_runner_base()` | 338f277 | 980eceb | ⏳ TODO |
| pitching_ai_strategy.c | `calculate_ai_pitch_target()` | e9499ef | 44666b3 | ⏳ TODO |

### Phase 3: Rules (Milestone 6) - COMPLETED
| Module | Extracted Function | Commit | Original Commit | Status |
|--------|-------------------|--------|----------------|--------|
| rules_outs.c | `is_runner_forced_out()` | 9e0b67d | bbb146b1 | ❌ **BUG FOUND** |
| rules_runs.c | `calculate_runs()` | 687ecfd | bbb146b1 | ✅ VERIFIED |
| rules_strikes.c | `should_change_batter_on_strikes()` | fa81a8c | bbb146b1 | ✅ VERIFIED |

---

## Audit Methodology

For each extracted function, perform the following:

### Step 1: Identify the "Before" Commit
```bash
# Find the commit BEFORE extraction
git log --oneline --all -- path/to/file.c | grep -B1 "Extract"
```

### Step 2: Compare Original vs Extracted Logic
```bash
# Get original logic
git show <before_commit>:path/to/original.c > /tmp/original.c

# Compare with current pure function
diff /tmp/original.c src/game/*_pure/extracted.c
```

### Step 3: Verify Call Site Parameters
**CRITICAL**: Don't just check the pure function - check HOW IT'S CALLED!

```c
// Example checklist:
// 1. Are all required inputs passed?
// 2. Are they passed in correct order?
// 3. Do variable names match semantic meaning?
// 4. Were any calculations (like baseIndex = i-1) lost?
// 5. Are outputs used correctly?
```

### Step 4: Trace Data Flow
For each parameter:
- What is its semantic meaning in original code?
- Is that meaning preserved in extraction?
- Are there any implicit calculations (like `baseIndex`) that got lost?

### Step 5: Document Findings
```markdown
## Function: calculate_xyz()

**Original Location**: src/game/old_file.c:123-145 (commit abc123)
**New Location**: src/game/actions_pure/new_file.c:10-25
**Call Site**: src/game/actions_messy/system.c:67

### Parameter Audit:
| Parameter | Original Variable | Current Parameter | Semantic Match? |
|-----------|------------------|-------------------|-----------------|
| param1    | stateInfo->x.y   | value1            | ✅ YES          |
| param2    | computed_val     | value2            | ❌ NO - missing calc |

### Verdict: ✅ CORRECT / ⚠️ SUSPICIOUS / ❌ BUG
**Notes**: ...
```

---

## Risk Assessment by Module

### HIGH RISK (Complex State Transformations)
- **pitching_physics**: Ball trajectory with velocity, angles, power
- **batting_physics**: Hit trajectory based on timing, angle, power
- **catching_ai_strategy**: Multi-base logic, lead runner determination

### MEDIUM RISK (Conditional Logic)
- **batting_ai_strategy**: Decision trees with multiple flags
- **pitching_ai_strategy**: Target calculation with RNG

### LOW RISK (Simple Calculations)
- **rules_strikes**: Simple boolean check (already verified)
- **rules_runs**: Clear conditional logic (already verified)

---

## Success Criteria

For audit to pass:
1. ✅ All parameters match original semantics
2. ✅ No implicit calculations lost during extraction
3. ✅ Call sites verified against original
4. ✅ Tests pass with original behavior
5. ✅ Manual testing confirms no regressions

---

## Estimated Effort

- **Per function**: 15-30 minutes thorough audit
- **Total functions**: ~10-12
- **Total time**: 3-5 hours of careful work

---

## Next Steps

1. Architect: Create TODO.md tasks for systematic audit
2. Task Agent: Execute audits one module at a time
3. Document: Update this file with findings as we go
4. Fix: Create immediate fix tasks for any bugs found
5. Test: Add regression tests for any bugs discovered

---

## Meta-Learning

**Why this matters:**
- Refactoring is NOT just "move code around"
- Context and semantics matter MORE than syntax
- "Extract as-is" requires preserving MEANING, not just LOGIC
- Integration tests matter as much as unit tests

**Process improvement:**
- Always compare call sites, not just extracted functions
- Look for calculated values (like baseIndex = i-1)
- Question variable names during extraction
- Add integration tests that exercise full data flow
