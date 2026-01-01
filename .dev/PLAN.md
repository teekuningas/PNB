## Refactoring Master Plan

## Current Status: Milestone 7 COMPLETE ✅ → Milestone 7.5 (Data Structure Cleanup)

**Foundation Quality:** 10.0/10 - Excellent Type-Safe Domain State
**Current Focus:** Clean up data structures before Milestone 8 (Referee Pattern)
**Philosophy:** Data shapes architecture - clean the foundation before building the skyscraper

---

## Milestone 7: Data Renaissance - COMPLETE ✅

**Achievement (2026-01-01):** Successfully eliminated ALL legacy state flags!

### What We Accomplished
- ✅ Deleted 6 boolean flags: `isOnBase`, `out`, `wounded`, `leading`, `takingFreeWalk`
- ✅ Replaced with type-safe enums: `PlayerUnitState`, `BaseID`, `GameEventType`
- ✅ Eliminated impossible states (compiler-enforced correctness)
- ✅ All 51 unit tests + 5 integration tests passing
- ✅ Deleted state_adapter.c/h (no longer needed)

### Key Insight Discovered
The flags we eliminated were **DOMAIN STATE** (representing game reality).
The flags remaining are **CONTROL STATE** (implementation bookkeeping).

**This distinction is critical for next steps.**

---

## Milestone 7.5: Data Structure Cleanup (CURRENT - 1-2 weeks)

**Philosophy:** Data shapes architecture. Clean the foundation before building the Referee pattern.

### Why Data First?
1. **Architectural patterns reflect data structures** - Can't build clean architecture on messy data
2. **Understanding through doing** - Cleaning data teaches us the game loop deeply
3. **Safety** - Small, testable steps vs. large architectural leaps
4. **Natural emergence** - Referee pattern will become obvious after cleanup

### The Remaining Pollution

**Problem:** Control flags mixed with domain data in structs

**Example:**
```c
typedef struct _BattingTeamPlayerInfo {
    // Domain: Game state ✅ CLEAN
    PlayerUnitState state;
    BaseID baseId;
    
    // Control: Implementation bookkeeping ⚠️ POLLUTION
    int arrivedToBase;       // Optimization flag
    int woundedApply;        // Deferred execution
    int passedPathPoint;     // State machine variable
    int goingForward;        // Direction tracking
    int hasMadeRunOnThirdBase; // Guard flag
} BattingTeamPlayerInfo;
```

**Goal:** Separate domain state from control state

### Phases

**Phase 0: Data Structure Audit** (1 day)
- Map all flags in GameAnalysisInfo (40+ fields!)
- Understand purpose of each flag
- Classify: Domain, Control, Camera, or Eliminable
- Document dependencies between flags

**Phase 1: Extract PlayerRuntimeState** (2 days)
- Create `PlayerRuntimeState` struct for control flags
- Move control flags OUT of `BattingTeamPlayerInfo`
- Migrate one flag at a time (testable, safe)
- Keep domain state clean

**Phase 2: Split GameAnalysisInfo God Object** (3-4 days)
- Break 40+ field monster into focused structs:
  - `GameState` (outs, strikes, balls, runs)
  - `GameControlFlags` (pause, initLocals, etc.)
  - `WoundingState` (wounding system)
  - `CameraState` (camera/UI)
  - `PlayerCounters` (player tracking)
- Mechanical refactoring (moving fields)
- Test continuously

**Phase 3: Stabilize & Document** (1-2 days)
- Update all documentation
- Draw new hierarchy diagrams
- Full test suite + manual playtest
- Celebrate clean foundation!

### Expected Outcome
- Clear separation: Domain vs Control vs UI state
- Referee pattern boundaries become obvious
- Safe, tested foundation for Milestone 8

---

## Milestone 8: The Referee Architecture (2-3 weeks) 🔭

**AFTER** data cleanup, implement the Referee pattern.

### The Vision

**The Referee Layer:**
- Analyzes physical world (player positions, ball state)
- Outputs **Abstract State** (Outs, Runs, Strikes)
- Outputs **Permissions** ("Can Pitch", "Can Bat", "Can Throw to Base X")

**Benefits:**
1. **Explicit Permissions** - Code checks permissions, doesn't calculate them
2. **Synchronous "Breathing"** - Input → Physics → Referee → Logic → Render
3. **Snapshotting & Replay** - Clean state enables instant replay
4. **Testability** - Referee is pure function (easier to test)

### Why After Data Cleanup?

The Referee pattern NEEDS clean separation of:
- What to analyze (domain state)
- What NOT to analyze (control flags)
- What to output (abstract state)

**Clean data makes Referee obvious. Messy data makes Referee impossible.**

---

## Completed Milestones

### ✅ Milestone 7: Data Renaissance (COMPLETE 2026-01-01)
- Eliminated ALL legacy state flags
- Type-safe enums: PlayerUnitState, BaseID, GameEventType
- 51 unit + 5 integration tests passing
- Compiler-enforced correctness

### ✅ Milestone 6: Rules Engine Extraction
- Extracted outs, runs, strikes to `rules_pure/`
- Comprehensive audit completed (1 bug found/fixed)
- All game "brains" now pure and tested

### ✅ Milestone 5: Logic Purification
- Extracted physics and AI to pure functions
- Created comprehensive unit tests

---

## Decision Log

### 2026-01-01: Data First, Then Architecture
**Decision:** Do Milestone 7.5 (data cleanup) BEFORE Milestone 8 (Referee pattern)
**Rationale:**
- Data structures shape architecture
- Can't build clean architecture on messy data
- Small, safe steps vs. large architectural leaps
- Understanding through doing (cleanup teaches us the game loop)
- Referee pattern will emerge naturally from clean data

### 2025-12-31: The "Write-Both" Pattern
**Decision:** Abandoned bidirectional adapter pattern in favor of write-both.
**Rationale:**
- Bidirectional adapter was scary - easy to overwrite data accidentally
- Write-both is simple, obvious, and safe
- Local reasoning at each write site
- No timing dependencies or order-sensitive sync calls
- Safe migration of reads can happen gradually

### 2025-12-30: The "Climb" Strategy
**Decision:** Break migration into Rendering first, then Logic.
**Rationale:****
- Rendering is "read-only" relative to game state. Safer to migrate first.
- Visual feedback provides immediate confirmation of Adapter correctness.
- Logic migration is higher risk, so it comes second.

### 2025-12-18: Data-First Strategy
**Decision:** Don't add enums to messy structures. Clean first.

---

*For detailed milestone achievements, see `docs/MILESTONE6_COMPLETE.md`*