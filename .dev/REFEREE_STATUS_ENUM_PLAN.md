# Referee Status Enum Refactor Plan
**Date:** 2026-01-19  
**Goal:** Replace flag-based player status with explicit enum state machine

---

## 🎯 Problem Statement

**Current Model: Multiple Independent Flags**
```c
typedef struct _RefereePlayerState {
    BaseID currentSafetyBase;
    int isOut;
    int hasScored;
    int runOfHonorScored;
    PendingWoundState pendingWoundState;  // Already an enum! (NONE, PENDING, EVALUATED)
    WoundingType woundingType;            // NONE, NORMAL, TUPLAHAAVA
    BaseID woundingSourceBase;
    int hasPendingRun;
    int hasPendingRunOfHonor;
    // ... other fields
} RefereePlayerState;
```

**Issues:**
1. **Mutually exclusive states represented as independent flags**
   - Can't be OUT and SCORED simultaneously (but nothing prevents it)
   - WOUND_MARKED vs WOUND_PENDING vs OUT (what takes precedence?)
2. **Fragile order-dependent logic**
   - Must check `isOut` before `hasScored` in reconciliation
   - Must check `pendingWoundState == EVALUATED` before granting safety
3. **Implicit state machines scattered across 882 lines**
   - State transitions hidden in assignment statements
   - Hard to see the full lifecycle: ACTIVE → MARKED → PENDING → WOUNDED → OUT/SCORED

---

## 🎨 Proposed Solution: Explicit Status Enum

### New Primary Status Enum

```c
typedef enum {
    PLAYER_STATUS_ACTIVE,              // Playing normally, no special status
    PLAYER_STATUS_WOUND_MARKED,        // Vulnerable: was not at original base when catch occurred
                                       // Evaluation timer running, wound can be canceled if ball drops
    PLAYER_STATUS_WOUND_PENDING,       // DOOMED (normal wound): will get WOUNDED or OUT (if ball reaches next base)
                                       // Must advance forward, cannot retreat
    PLAYER_STATUS_WOUND_PENDING_DOUBLE,// DOOMED (tuplahaava): will get WOUNDED or OUT
                                       // CAN retreat to previous base to avoid OUT
    PLAYER_STATUS_WOUNDED,             // TERMINAL: Out of play, must return to home base
    PLAYER_STATUS_OUT                  // TERMINAL: Out of play, must return to home base
} RefereePlayerStatus;
```

**Key Insight:** These are **mutually exclusive player statuses** representing the wounding/out lifecycle.

**Status Meanings:**
- **ACTIVE**: Normal play, no wound threat
- **WOUND_MARKED**: Vulnerable but not doomed yet (ball held, timer running, can cancel if dropped)
- **WOUND_PENDING**: Doomed normal wound (must advance, will get WOUNDED or OUT)
- **WOUND_PENDING_DOUBLE**: Doomed tuplahaava (can retreat to previous base, will get WOUNDED or OUT)
- **WOUNDED**: Terminal state - execution phase, player returns to home base
- **OUT**: Terminal state - player returns to home base

### Secondary Flags (Not Mutually Exclusive, But Related to Status)

**SCORING and STATUS interact** - status determines if runs are honored:

```c
typedef struct _RefereePlayerState {
    // === PRIMARY STATUS (NEW) ===
    RefereePlayerStatus status;         // One status at a time (mutually exclusive)
                                        // Determines if pending runs will be honored or voided
    
    // === SAFETY & POSITION ===
    BaseID currentSafetyBase;           // Which base has their safety
    BaseID baseAtPitchStart;            // Snapshot at pitch start (also used as wound return target)
    
    // === PENDING RUNS (temporal - set optimistically, resolved by status) ===
    int hasPendingRun;                  // Set when reaching home while ball in air (ACTIVE/MARKED)
                                        // VOIDED if status becomes PENDING/WOUNDED/OUT
                                        // CASHED IN if status resolves favorably (stays/returns to ACTIVE)
    int hasPendingRunOfHonor;           // Same logic for run of honor at 3rd
    
    // === SCORING (only set when status permits) ===
    int hasScored;                      // Only set when status allows (NOT while PENDING/WOUNDED/OUT)
    int runOfHonorScored;               // Only set when status allows (NOT while PENDING/WOUNDED/OUT)
    
    // === EVENT SNAPSHOTS ===
    BaseID baseAtLastEvent;
    int hadSafetyAtLastEvent;
} RefereePlayerState;
```

**What gets REMOVED:**
- ~~`int isOut`~~ → replaced by `status == PLAYER_STATUS_OUT`
- ~~`PendingWoundState pendingWoundState`~~ → replaced by `status` (NONE→ACTIVE, PENDING→WOUND_PENDING, EVALUATED→WOUNDED)
- ~~`WoundingType woundingType`~~ → replaced by `status` (NORMAL→WOUND_PENDING, TUPLAHAAVA→WOUND_PENDING_DOUBLE)
- ~~`BaseID woundingSourceBase`~~ → redundant! Always equals `baseAtPitchStart`
- ~~`int woundingPlayersMarked[]`~~ (in RefereeState) → replaced by `status == WOUND_MARKED`

**What gets KEPT (as separate flags):**
- `hasScored` / `runOfHonorScored` - separate but **affected by status** (only set when status permits)
- `hasPendingRun` / `hasPendingRunOfHonor` - temporal "bets" that status resolves
- `currentSafetyBase` - can have safety in various states
- `baseAtPitchStart` - serves double duty: snapshot AND wound return target

---

## 📊 State Transition Map

### Current Flag Combinations → New Status

| Current Flags | Mapped Status | Notes |
|---------------|---------------|-------|
| All zeros/NONE, not marked | `ACTIVE` | Normal play |
| `woundingPlayersMarked[i]==1` | `WOUND_MARKED` | Evaluation timer running, can cancel |
| `pendingWoundState==PENDING` + `woundingType==NORMAL` | `WOUND_PENDING` | Doomed (normal wound) |
| `pendingWoundState==PENDING` + `woundingType==TUPLAHAAVA` | `WOUND_PENDING_DOUBLE` | Doomed (tuplahaava) |
| `pendingWoundState==EVALUATED` | `WOUNDED` | Terminal: return to home |
| `isOut==1` | `OUT` | Terminal: return to home |

**REMOVED from mapping:**
- ~~`hasScored==1`~~ → Keep as **separate flag** (related to status, not part of it)
- ~~`runOfHonorScored==1`~~ → Keep as **separate flag** (related to status, not part of it)

### State Lifecycle Examples

**Normal Play → Out:**
```
ACTIVE → (force out at base) → OUT
```

**Fly Ball Catch → Normal Wounding:**
```
ACTIVE → WOUND_MARKED (catch made, timer starts)
       → WOUND_PENDING (timer expired, doomed)
       → WOUNDED (returned to source base) OR OUT (ball reached next base while running)
```

**Tuplahaava (Double Wound):**
```
ACTIVE at base → WOUND_MARKED (catch made at different base)
              → WOUND_PENDING_DOUBLE (wounded player arrives, collision)
              → Can retreat to source to become WOUNDED
              → OR advance and risk OUT if ball reaches next base
```

**Run Scoring (affected by status):**
```
ACTIVE + reaches home while ball in air → hasPendingRun = 1
  → Ball lands safely → ACTIVE + hasScored = 1 (run awarded)
  
WOUND_MARKED + reaches home while ball in air → hasPendingRun = 1
  → Timer expires, becomes WOUND_PENDING → hasPendingRun = 0 (run VOIDED)
  → OR ball drops, becomes ACTIVE → hasScored = 1 (run awarded)

WOUND_PENDING → Can NEVER get hasScored = 1 (doomed, no runs possible)
```

**Key Rule:** Pending runs are optimistic "bets" that get resolved based on how status settles.

**Note:** Pending runs can be set in ACTIVE/WOUND_MARKED states. Status determines if they're honored (ACTIVE→award) or voided (PENDING/WOUNDED/OUT).

---

## 🔍 Complete Audit of Flag Usage

### 1. WRITES (Status Transitions)

**File:** `referee.c`

| Line | Current Code | New Status | Context |
|------|--------------|------------|---------|
| 101 | `pendingWoundState = NONE` | `status = ACTIVE` | Pitch start snapshot (clear wound) |
| 103 | `woundingSourceBase = BASE_NONE` | DELETE | Redundant (use baseAtPitchStart) |
| 102 | `woundingType = NONE` | DELETE | Type encoded in status |
| 205 | `pendingWoundState = PENDING` | `status = WOUND_PENDING` | Timer expired (normal wound) |
| 208-209 | Check/set `woundingType = NORMAL` | (implicit) | Type encoded in WOUND_PENDING |
| 220 | `pendingWoundState = EVALUATED` | `status = WOUNDED` | Timer expired, evaluated |
| 308 | `pendingWoundState = PENDING` | `status = WOUND_PENDING_DOUBLE` | Tuplahaava collision |
| 314 | `woundingType = TUPLAHAAVA` | (implicit) | Type encoded in WOUND_PENDING_DOUBLE |
| 315 | `woundingSourceBase = physicalBase` | DELETE | Redundant (use baseAtPitchStart) |
| 320 | `woundingType = NORMAL` | (implicit) | Arriving wounded player is normal |
| 377 | `isOut = 1` | `status = OUT` | Force out at base |
| 419 | `isOut = 1` | `status = OUT` | Tuplahaava: ball at next |
| 423 | `pendingWoundState = EVALUATED` | `status = WOUNDED` | Tuplahaava: ball at next |
| 430 | `pendingWoundState = EVALUATED` | `status = WOUNDED` | Tuplahaava: ball at source |
| 437 | `pendingWoundState = EVALUATED` | `status = WOUNDED` | Tuplahaava: at source/next |
| 535 | `isOut = 1` | `status = OUT` | Run of honor overtaking |
| 862-867 | Initialize all flags | `status = ACTIVE` | Initialization (simpler!) |

**Scoring writes (KEEP as separate flags):**
| Line | Current Code | Keep As-Is | Reason |
|------|--------------|------------|--------|
| 510 | `hasScored = 1` | Keep | Orthogonal to status |
| 521 | `runOfHonorScored = 1` | Keep | Orthogonal to status |
| 702 | `hasScored = 1` | Keep | Orthogonal to status |
| 716 | `runOfHonorScored = 1` | Keep | Orthogonal to status |
| 742 | `hasScored = 1` | Keep | Orthogonal to status |
| 863-864 | Initialize score flags | Keep | Orthogonal to status |

**woundingPlayersMarked[] special handling:**
- Line 161: `woundingPlayersMarked[i] = 1` → Set `status = WOUND_MARKED`
- Line 311: `woundingPlayersMarked[j] = 1` → Set `status = WOUND_MARKED`
- Line 224, 188, 49, 873: Clear marked → No longer needed (status handles it)
- Line 294: Check marked → Check `status == WOUND_MARKED`

**Outside referee.c:**
- `game_setup.c:113` - Clear pending wound on foul reset → `status = ACTIVE`

### 2. READS (Status Checks)

**File:** `referee.c`

| Line | Current Code | New Check | Context |
|------|--------------|-----------|---------|
| 164 | `woundingSourceBase = baseAtPitchStart` | DELETE | Redundant assignment |
| 208-209 | Check `woundingType != TUPLAHAAVA` | Check `status != WOUND_PENDING_DOUBLE` | Determine wound type |
| 217 | Check `woundingType != TUPLAHAAVA` | Check `status != WOUND_PENDING_DOUBLE` | Remove safety for normal |
| 284 | `pendingWoundState == EVALUATED` | `status == WOUNDED` | Don't grant safety if wounded |
| 292-294 | `pendingWoundState == PENDING/EVALUATED` or marked | `status >= WOUND_MARKED` | Tuplahaava displacement check |
| 306-307 | `pendingWoundState == PENDING/EVALUATED` | `status >= WOUND_PENDING` | Immediately mark occupant |
| 317 | Read `woundingSourceBase` | Use `baseAtPitchStart` | For teleport target |
| 407-408 | `pendingWoundState == PENDING && type==TUPLAHAAVA` | `status == WOUND_PENDING_DOUBLE` | Tuplahaava exceptions |
| 409 | `!isOut` | `status != OUT` | Only process non-out players |
| 411 | `woundingSourceBase` | Use `baseAtPitchStart` | Get source base |
| 501 | `runOfHonorScored` | KEEP | Orthogonal flag check |
| 735 | `pendingWoundState == NONE` | `status < WOUND_MARKED` | Award run if not wounded |
| 824 | `!isOut` | `status != OUT` | Homerun pair complete check |

**File:** `mutable_world.c`

| Line | Current Code | New Check | Context |
|------|--------------|-----------|---------|
| 54 | `isOut` | `status == OUT` | Reconcile: move player out |
| 64 | `hasScored` | KEEP | Orthogonal flag check |
| 76 | `pendingWoundState == EVALUATED` | `status == WOUNDED` | Don't force run if wounded |

**File:** `game_manipulation.c`

| Line | Current Code | New Check | Context |
|------|--------------|-----------|---------|
| 314 | `pendingWoundState == EVALUATED` | `status == WOUNDED` | Teleport wounded player |
| 316 | `woundingSourceBase` | Use `baseAtPitchStart` | Get return target |
| 317 | `woundingType` | Check `status == WOUND_PENDING_DOUBLE` | Determine wound type |

**File:** `game_setup.c`

| Line | Current Code | New Check | Context |
|------|--------------|-----------|---------|
| 157 | `isOut` | `status == OUT` | Restore player position |
| 160 | `hasScored` | KEEP | Orthogonal flag check |

**File:** `base_logic.c`

| Line | Current Code | New Check | Context |
|------|--------------|-----------|---------|
| 153 | `!runOfHonorScored` | KEEP | Separate flag, logic unchanged |

**Tests:** (11 occurrences)
- Mostly assertions checking wound/score status
- Update wound checks to use `status`
- Keep score checks as-is (separate flags, logic unchanged)

---

## 🛠️ Two-Step Migration Strategy

### Step 1: Add Enum, Keep Flags (Parallel State)

**Goal:** Replace all flag reads/writes with status, but keep old flags in sync.

**Changes:**
1. Add `RefereePlayerStatus status` field to `RefereePlayerState`
2. **WRITE SITES:** Every flag write also sets corresponding status
   ```c
   // Old:
   referee->battingPlayers[i].isOut = 1;
   
   // Step 1 (verbose, redundant):
   referee->battingPlayers[i].isOut = 1;
   referee->battingPlayers[i].status = PLAYER_STATUS_OUT;
   ```
3. **READ SITES:** Replace all flag reads with status checks
   ```c
   // Old:
   if (referee->battingPlayers[i].isOut) { ... }
   
   // Step 1:
   if (referee->battingPlayers[i].status == PLAYER_STATUS_OUT) { ... }
   ```
4. **Special: woundingPlayersMarked[]**
   - When marking: also set `status = WOUND_MARKED`
   - When checking marked: check `status == WOUND_MARKED`
   - Keep array for now (cleanup in Step 2)

**Validation:** Run all 61 tests. They should pass because old flags still exist.

### Step 2: Remove Flags, Consolidate Logic

**Goal:** Delete old flags, simplify code.

**Changes:**
1. Remove old fields from struct:
   - ~~`int isOut`~~
   - ~~`PendingWoundState pendingWoundState`~~
   - ~~`WoundingType woundingType`~~
   - ~~`BaseID woundingSourceBase`~~
2. Remove `woundingPlayersMarked[]` array from RefereeState
   - Just use `status == WOUND_MARKED`
3. Simplify reconciliation with switch-case:
   ```c
   switch (referee->battingPlayers[i].status) {
       case PLAYER_STATUS_OUT:
           if (game->playerInfo[i].bTPI.state != PLAYER_STATE_OUT) {
               game->playerInfo[i].bTPI.state = PLAYER_STATE_OUT;
               movePlayerOut(...);
           }
           break;
       case PLAYER_STATUS_SCORED:
           if (game->playerInfo[i].bTPI.state != PLAYER_STATE_SCORED) {
               game->playerInfo[i].bTPI.state = PLAYER_STATE_SCORED;
               movePlayerOut(...);
           }
           break;
       case PLAYER_STATUS_WOUNDED:
           // Don't force run
           break;
       default:
           // Check for panic run displacement
           if (referee->battingPlayers[i].currentSafetyBase != physBase) {
               runToNextBase(...);
           }
           break;
   }
   ```
4. Simplify initialization:
   ```c
   void initializeRefereeState(RefereeState* referee) {
       for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
           referee->battingPlayers[i].status = PLAYER_STATUS_ACTIVE;  // ONE LINE!
           referee->battingPlayers[i].currentSafetyBase = BASE_NONE;
           referee->battingPlayers[i].woundingType = WOUNDING_TYPE_NONE;
           // ... fewer lines total
       }
   }
   ```

**Validation:** Run all 61 tests again.

---

## 📋 Detailed Implementation Checklist

### Phase 1: Preparation (30 min)

- [ ] Add `RefereePlayerStatus` enum to `globals.h` (after `PendingWoundState`)
- [ ] Add `RefereePlayerStatus status;` field to `RefereePlayerState` struct
- [ ] Initialize `status = PLAYER_STATUS_ACTIVE` in `initializeRefereeState()`
- [ ] Document enum states in comments

### Phase 2: Step 1 - Parallel State (3 hours)

#### 2.1: Update Write Sites in referee.c (1.5 hours)

**Wounding State Transitions:**
- [ ] Line 101: `pendingWoundState = NONE` → also set `status = ACTIVE`
- [ ] Line 102: DELETE `woundingType = NONE` (type encoded in status)
- [ ] Line 103: DELETE `woundingSourceBase = BASE_NONE` (redundant field)
- [ ] Line 205: `pendingWoundState = PENDING` → also set `status = WOUND_PENDING`
- [ ] Line 208-209: Check wound type → Check `status != WOUND_PENDING_DOUBLE`
- [ ] Line 220: `pendingWoundState = EVALUATED` → also set `status = WOUNDED`
- [ ] Line 308: `pendingWoundState = PENDING` (tuplahaava) → also set `status = WOUND_PENDING_DOUBLE`
- [ ] Line 314: DELETE `woundingType = TUPLAHAAVA` (type encoded in status)
- [ ] Line 315: DELETE `woundingSourceBase = physicalBase` (use baseAtPitchStart)
- [ ] Line 320: DELETE `woundingType = NORMAL` (type encoded in status)
- [ ] Line 423: `pendingWoundState = EVALUATED` → also set `status = WOUNDED`
- [ ] Line 430: `pendingWoundState = EVALUATED` → also set `status = WOUNDED`
- [ ] Line 437: `pendingWoundState = EVALUATED` → also set `status = WOUNDED`

**Out Transitions:**
- [ ] Line 377: `isOut = 1` → also set `status = OUT`
- [ ] Line 419: `isOut = 1` → also set `status = OUT`
- [ ] Line 535: `isOut = 1` → also set `status = OUT`

**Initialization:**
- [ ] Lines 862-867: Set `status = ACTIVE`, DELETE redundant flag inits

**Special: woundingPlayersMarked[]:**
- [ ] Line 161: When marking, also set `status = WOUND_MARKED`
- [ ] Line 311: When marking for tuplahaava, also set `status = WOUND_MARKED`

**Scoring writes (NO CHANGE - separate flags, logic unchanged):**
- Lines 510, 521, 702, 716, 742, 863-864 stay as-is

#### 2.2: Update Read Sites in referee.c (30 min)

- [ ] Line 164: DELETE `woundingSourceBase = baseAtPitchStart` (redundant)
- [ ] Line 208-209: `woundingType != TUPLAHAAVA` → `status != WOUND_PENDING_DOUBLE`
- [ ] Line 217: `woundingType != TUPLAHAAVA` → `status != WOUND_PENDING_DOUBLE`
- [ ] Line 284: `pendingWoundState == EVALUATED` → `status == WOUNDED`
- [ ] Line 292-294: Complex wound check → `status >= WOUND_MARKED`
- [ ] Line 306-307: Pending/evaluated check → `status >= WOUND_PENDING`
- [ ] Line 407-408: Pending tuplahaava → `status == WOUND_PENDING_DOUBLE`
- [ ] Line 409: `!isOut` → `status != OUT`
- [ ] Line 411: `woundingSourceBase` → `baseAtPitchStart`
- [ ] Line 735: `pendingWoundState == NONE` → `status < WOUND_MARKED`
- [ ] Line 824: `!isOut` → `status != OUT`

**NO CHANGE (separate flags, logic unchanged):**
- Line 501: `runOfHonorScored` check stays as-is

#### 2.3: Update Other Files (30 min)

**mutable_world.c:**
- [ ] Line 54: `isOut` → `status == OUT`
- [ ] Line 64: `hasScored` → NO CHANGE (separate flag, logic unchanged)
- [ ] Line 76: `pendingWoundState == EVALUATED` → `status == WOUNDED`

**game_manipulation.c:**
- [ ] Line 314: `pendingWoundState == EVALUATED` → `status == WOUNDED`
- [ ] Line 316: `woundingSourceBase` → `baseAtPitchStart`
- [ ] Line 317: Check `woundingType` → Check `status == WOUND_PENDING_DOUBLE`

**game_setup.c:**
- [ ] Line 113: Clear pending wound → also set `status = ACTIVE`
- [ ] Line 157: `isOut` → `status == OUT`
- [ ] Line 160: `hasScored` → NO CHANGE (separate flag, logic unchanged)

**base_logic.c:**
- [ ] Line 153: `!runOfHonorScored` → NO CHANGE (separate flag, logic unchanged)

**Tests:** (30 min)
- [ ] Update wound-related assertions to check `status`
- [ ] Keep score-related assertions as-is (separate flags, logic unchanged)

#### 2.4: Build & Test Step 1 (30 min)

- [ ] `make clean && make main` - Must compile
- [ ] `make test` - All 61 tests must pass
- [ ] Manual smoke test - Play a few pitches
- [ ] Commit: "refactor: add RefereePlayerStatus enum (parallel state)"

### Phase 3: Step 2 - Remove Flags (2 hours)

#### 3.1: Remove Old Fields (15 min)

- [ ] Delete from `RefereePlayerState` struct:
  - ~~`int isOut;`~~
  - ~~`PendingWoundState pendingWoundState;`~~
  - ~~`WoundingType woundingType;`~~
  - ~~`BaseID woundingSourceBase;`~~
- [ ] Delete from `RefereeState` struct:
  - ~~`int woundingPlayersMarked[PLAYERS_IN_TEAM + JOKER_COUNT];`~~
- [ ] Delete enum `PendingWoundState` from `globals.h`
- [ ] Delete enum `WoundingType` from `globals.h`

**KEEP these fields (separate from status, but related):**
- `int hasScored;`
- `int runOfHonorScored;`
- `int hasPendingRun;`
- `int hasPendingRunOfHonor;`

#### 3.2: Remove Redundant Writes (30 min)

Go through all lines from Phase 2.1 and remove the old flag writes:

- [ ] Remove all `isOut = ` lines (keep status writes)
- [ ] Remove all `pendingWoundState = ` lines (keep status writes)
- [ ] Remove all `woundingType = ` lines (type encoded in status)
- [ ] Remove all `woundingSourceBase = ` lines (use baseAtPitchStart)
- [ ] Remove all `woundingPlayersMarked[i] = ` lines (status handles it)

**DO NOT remove:**
- `hasScored = ` lines (separate flag, logic unchanged)
- `runOfHonorScored = ` lines (separate flag, logic unchanged)

#### 3.3: Simplify Reconciliation (45 min)

Replace `reconcileLegalAndPhysicalState()` in `mutable_world.c`:

```c
for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
    RefereePlayerStatus status = game->referee.battingPlayers[i].status;
    
    // React to terminal states
    if (status == PLAYER_STATUS_OUT) {
        if (game->playerInfo[i].bTPI.state != PLAYER_STATE_OUT) {
            game->playerInfo[i].bTPI.state = PLAYER_STATE_OUT;
            game->playerInfo[i].bTPI.baseId = BASE_NONE;
            movePlayerOut(...);
        }
        continue;
    }
    
    // React to scoring (separate check, but affected by status)
    if (game->referee.battingPlayers[i].hasScored && 
        game->playerInfo[i].bTPI.state != PLAYER_STATE_SCORED) {
        game->playerInfo[i].bTPI.state = PLAYER_STATE_SCORED;
        game->playerInfo[i].bTPI.baseId = BASE_NONE;
        movePlayerOut(...);
        continue;
    }
    
    // React to displacement (panic run) - unless WOUNDED
    if (status != PLAYER_STATUS_WOUNDED) {
        if (game->playerInfo[i].bTPI.state == PLAYER_STATE_ON_BASE || 
            game->playerInfo[i].bTPI.state == PLAYER_STATE_LEADING) {
            BaseID physBase = game->playerInfo[i].bTPI.baseId;
            if (game->referee.battingPlayers[i].currentSafetyBase != physBase) {
                runToNextBase(game, stateInfo->fieldPositions, i, physBase);
            }
        }
    }
}
```

Key insight: **Scoring check stays as separate if-statement** (separate flag, but status affects when it gets set)

#### 3.4: Build & Test Step 2 (30 min)

- [ ] `make clean && make main` - Must compile
- [ ] `make test` - All 61 tests must pass
- [ ] Manual smoke test
- [ ] Commit: "refactor: remove flag-based status, use enum exclusively"

### Phase 4: Documentation (30 min)

- [ ] Add state machine diagram to `ARCHITECTURE.md`
- [ ] Document status transitions in referee.c header comment
- [ ] Update this plan with "COMPLETED" status
- [ ] Commit: "docs: document RefereePlayerStatus state machine"

---

## ⏱️ Estimated Timeline

| Phase | Tasks | Time |
|-------|-------|------|
| Preparation | Add enum, add field | 30 min |
| Step 1: Writes | Update all 20 write sites | 1.5 hours |
| Step 1: Reads | Update all 15 read sites | 1 hour |
| Step 1: Test | Build & validate | 30 min |
| Step 2: Remove | Delete flags, simplify | 1.5 hours |
| Step 2: Test | Build & validate | 30 min |
| Documentation | Update docs | 30 min |
| **Total** | | **~6 hours** |

---

## 🎯 Success Criteria

After completion:

1. **Zero Compilation Errors**
   - All files compile cleanly
   - No undefined references to old flags

2. **All Tests Pass**
   - 48 unit tests pass
   - 13 integration tests pass
   - No regressions

3. **Code is More Explicit**
   - State transitions are clear: `status = PLAYER_STATUS_OUT`
   - Switch-case logic replaces order-dependent if-chains
   - Impossible to set contradictory states

4. **Reduced Lines of Code**
   - Initialization is simpler (fewer flags)
   - Reconciliation is cleaner (switch-case)
   - Estimated ~50-100 lines saved

5. **Documentation Current**
   - State machine diagram in ARCHITECTURE.md
   - Enum values commented
   - Transition rules documented

---

## 🚨 Risk Mitigation

**Risk 1: Miss a flag usage site**
- *Mitigation:* Compiler will catch writes (field doesn't exist)
- *Mitigation:* Grep audit before starting

**Risk 2: Incorrect status precedence**
- *Mitigation:* Step 1 keeps flags in parallel (validates mapping)
- *Mitigation:* Extensive test suite catches behavioral changes

**Risk 3: Tests fail after Step 1**
- *Mitigation:* Revert, fix mapping, try again
- *Mitigation:* Step 1 should be 100% equivalent to old behavior

**Risk 4: Complexity in woundingPlayersMarked**
- *Mitigation:* Keep array in Step 1, remove in Step 2
- *Mitigation:* WOUND_MARKED status replaces array naturally

---

## 💡 Key Insights

1. **Scoring is NOT orthogonal, but NOT mutually exclusive either**
   - Pending runs (`hasPendingRun`) are temporal "bets" set optimistically
   - Status determines if those bets pay off (ACTIVE→award, PENDING→void)
   - Actual scoring (`hasScored=1`) only happens when status permits
   - Keep as separate flags because multiple players can have pending runs simultaneously

2. **woundingSourceBase is redundant**
   - Always equals `baseAtPitchStart` (verified in code)
   - Can delete it entirely, use `baseAtPitchStart` everywhere

3. **WoundingType encoded in status**
   - NORMAL → `WOUND_PENDING`
   - TUPLAHAAVA → `WOUND_PENDING_DOUBLE`
   - No need for separate enum

4. **woundingPlayersMarked[] is redundant**
   - It's effectively `status == WOUND_MARKED`
   - Can delete entire array in Step 2

5. **Reconciliation logic clarified**
   - Terminal statuses: OUT (check status)
   - Scoring: separate check but **affected by status** (pending runs get voided/cashed based on status)
   - Panic run: avoid if WOUNDED (check status)

6. **Step 1 is critical**
   - Validates our mapping before point of no return
   - Old flags ensure tests pass during transition
   - Scoring flags stay throughout both steps but logic remains unchanged

---

## 📝 Notes

- Total flag writes: ~20 sites (excluding scoring flags which stay)
- Total flag reads: ~15 sites (excluding scoring flags which stay)
- Total test updates: ~11 assertions (only wound-related)
- Referee.c size: 882 lines (should reduce to ~850 after Step 2)

**Fields being removed:**
- `int isOut` (3 uses)
- `PendingWoundState pendingWoundState` (15 uses)
- `WoundingType woundingType` (8 uses)
- `BaseID woundingSourceBase` (6 uses)
- `int woundingPlayersMarked[]` (13 uses)

**Fields being KEPT (separate from status, but related):**
- `int hasScored` - Only set when status permits (not PENDING/WOUNDED/OUT)
- `int runOfHonorScored` - Only set when status permits (not PENDING/WOUNDED/OUT)
- `int hasPendingRun` - Temporal "bet" that status resolves (ACTIVE→award, PENDING→void)
- `int hasPendingRunOfHonor` - Temporal "bet" that status resolves

**This refactor sets the stage for:**
- Cleaner M18 physics extraction
- Easier debugging (single status instead of flag soup)
- Future AI integration (state machine is self-documenting)
- **Clearer run resolution logic** (pending runs as "bets" that status resolves)

---

**Ready to execute?** Start with Phase 1 (Preparation).
