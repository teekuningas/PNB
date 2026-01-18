# PNB Development Plan

## Current Phase: Referee Consolidation Complete! ✅ → Ready for Milestone 18 🎯

**Status:** Milestone 17 Phase 2B COMPLETE ✅ | All 61 tests passing | Documentation consolidated

**Latest Achievement (2026-01-18):**
- ✅ Split GameControl → BetweenPitchState + FlowControl (22 files, ~200 references)
- ✅ Fixed wounding evaluation violation in batting_system.c
- ✅ All 61 tests passing (48 unit + 13 integration)
- ✅ Clean ownership boundaries established
- ✅ Documentation consolidated (16 files → 5 core files)

**Next Major Goal:** Milestone 18 - Extract pure physics from game_manipulation.c

---

## The Golden Rule

1. **One Way Flow:** Physics → Events → Referee → Legal State → Reconciliation → Physics
2. **Limited Scope:** Referee MUST NOT mutate other structures (e.g., `PlayerInfo`, `BallInfo`, `pRAI`, `AIState`). Its output is strictly the Legal State (`RefereeState`, `HalfInningState`, `BetweenPitchState`).

---

## Ownership Map (Post-M17)

| Structure | Owner (Writer) | Readers | Lifecycle |
| :--- | :--- | :--- | :--- |
| `GameEvents` | Physics/Actions | Referee | Clear every frame |
| **`BetweenPitchState`** | **Referee ONLY** | Everyone | Reset at pitch start |
| `FlowControl` | Menus/Input (+ referee temp*) | Game loop, Referee | User-controlled |
| `RefereeState` | **referee.c ONLY** | Reconcile, UI, AI | Persistent |
| `HalfInningState` | **referee.c ONLY** | Everyone | Persistent (inning-scoped) |

\* *Note: Referee writes to FlowControl temporarily for free walk reset. Will be refactored to event-driven in future.*

**Exception:** Initialization functions may write during setup (marked with `// REFEREE INIT`):
- `initializeRefereeState()` - Game start
- `initializeBatter()` - New batter enters
- `releasePitch()` - Pitch start snapshot
- `applyFoulPlayReset()` - Foul reset

---

## 📅 Milestone History

### ✅ Milestones 10-15 (Completed 2025-2026)
- **M10-13:** State Consolidation - `StateInfo` as single source of truth
- **M14:** The Great Decoupling V1 - `Referee_Analyze` (pure) + `Referee_Apply` (impure)
- **M15:** Referee Architecture V2 - `Referee_Update` sequential pipeline (eliminated Decisions struct)

### ✅ Milestone 16: Event System (Completed Jan 2026)
**Achievement:** Split control flags into `GameEvents` (transient) + `GameControl` (stateful)

**Changes:**
- Created `GameEvents` struct for frame-only events
- Established Physics → Events → Referee communication pattern
- All tests passing with new event system

### ✅ Milestone 17: Referee Consolidation (Completed Jan 2026)

#### Phase 2A: Core Rule Migration ✅
**Completed:**
- Strike/Ball counting moved to referee.c
- Free walk logic moved to referee.c  
- Out of bounds refactored (referee decides → reconciliation executes)
- Wounding logic refactored (timer in referee, monitors ball drop)
- Pattern established: Physics → Events → Referee → Decisions → Reconciliation

**Results:**
- Removed ~250 lines from game_analysis.c
- Added ~150 lines to referee.c
- All 51 unit + 11 integration tests passing

#### Phase 2B: Structural Cleanup ✅ (Completed 2026-01-18)
**What was done:**

1. **GameControl Split** (5 hours)
   - Created `BetweenPitchState` (4 referee decision flags)
   - Created `FlowControl` (6 user interaction flags + free walk data)
   - Migrated ~200 references across 22 files
   - Updated function signatures (Referee_Update, should_ai_drop_ball, etc.)
   - Deleted old GameControl struct

2. **Fixed Wounding Violation** (5 min)
   - Removed writes to referee state from batting_system.c
   - Moved wounding reset to referee.c pitchReleased handler

3. **Documentation Consolidation** (30 min)
   - Reduced from 16 markdown files to 5 core files
   - Archived 8 outdated documents (session notes, one-time audits)
   - Updated ARCHITECTURE.md and PLAN.md with current state

**Final Status:**
- ✅ All 61 tests passing (48 unit + 13 integration)
- ✅ Clean ownership boundaries
- ✅ Zero technical debt blocking M18
- ✅ Documentation current and accurate

---

## 🎯 Milestone 18: Physics/State Split (Next - 7-8 sessions)

**Goal:** Separate pure physics from state management in `game_manipulation.c` (~1500 LOC)

**Why This Matters:**
Currently `game_manipulation.c` mixes:
- Pure physics integration (ball movement, collision)
- Game state mutations (baseId, hasBallIndex)
- Event emission (ballHitGround, catchMade)
- Geometry checks (out of bounds)

**Benefits:**
1. **Testability:** Pure physics functions can be unit tested
2. **Replay/Determinism:** Physics separate from side effects
3. **Performance:** Can optimize physics without touching state
4. **Clarity:** Clear boundary between "what happened" and "what it means"

**High-Level Plan:**

1. **Create PhysicsEngine Module** (~2 sessions)
   - Input: `State + dt` (delta time)
   - Output: `NewPositions + PhysicsEvents`
   - Pure functions: `updateBallPhysics()`, `updatePlayerPhysics()`, `detectCollisions()`
   - No mutations, no globals, no side effects

2. **Create PhysicsObserver** (~1 session)
   - Watches physics output
   - Emits `GameEvents` (catchMade, ballHitGround, etc.)
   - Bridge between pure physics and event system

3. **Refactor game_manipulation.c** (~2-3 sessions)
   - Extract physics → PhysicsEngine
   - Extract event emission → PhysicsObserver  
   - Keep only: state application, player AI triggers
   - Rename to something like `game_state_updater.c`

4. **Test & Validate** (~1 session)
   - Physics unit tests
   - Integration tests should still pass
   - Performance benchmarks

**Success Criteria:**
- Physics engine has zero dependencies on game state structs
- All physics logic is pure and testable
- Event emission is centralized in observer
- All existing tests pass
- New physics unit tests cover edge cases

---

## 🔮 Long-Term Goals (Milestones 19-20)

### Milestone 19: Action System Decoupling
**Goal:** Split `actions_messy/` into pure logic + execution
**Benefit:** Testable action logic, clear separation of concerns

### Milestone 20: User Intent Layer
**Goal:** Create Input → Intent → Engine pipeline
**Benefit:** Foundation for replay system and networked multiplayer

---

## 📊 Current State Summary

### Codebase Stats
- **Total game code:** ~9,000 LOC
- **Referee code:** ~750 LOC (referee.c)
- **Pure functions:** ~740 LOC (rules_pure/, actions_pure/, ai_pure/)
- **Test code:** 61 tests (48 unit + 13 integration)

### Code Quality
- ✅ Zero technical debt blocking M18
- ✅ Clean ownership boundaries
- ✅ Event-driven architecture established
- ✅ Comprehensive test coverage
- 🚧 Physics still coupled to state (M18 target)
- 🚧 Actions still messy (M19 target)

### Key Files
- `src/game/referee.c` - Referee Update pipeline (750 LOC)
- `src/game/mutable_world.c` - Main loop, reconciliation
- `src/game/game_manipulation.c` - Physics + events (M18 target)
- `src/game/game_analysis.c` - Flow logic (slimmed down)
- `src/game/rules_pure/` - Pure rule functions
- `src/include/globals.h` - All type definitions

---

## 🧹 Remaining Minor Tasks (Optional)

These are nice-to-have improvements that don't block M18:

### 1. Add Section Markers to referee.c (15 min)
Add clear section comments to organize the ~750 LOC:
```c
// ============================================================================
// Foul Play & Wounding Logic
// ============================================================================

// ============================================================================  
// Safety Management
// ============================================================================

// ... etc
```

### 2. Document Initialization Pattern (15 min)
Add to ARCHITECTURE.md:
- Explain which functions can write to RefereeState during setup
- Mark as temporary until event-driven approach in post-M18

### 3. Free Walk Refactor (Future)
Move free walk flag reset from referee to game_analysis when it detects balls count changed.

**Decision:** These can be done anytime. Not blocking M18.

---

## 📚 Documentation

### Active Documents
1. **docs/ARCHITECTURE.md** - System design, current state, roadmap
2. **docs/SAANNOT.md** - Pesäpallo rules reference (Finnish)
3. **docs/README.md** - Contributor quick start guide
4. **.dev/PLAN.md** - This file (development roadmap)
5. **Agent protocols** (.dev/ARCHITECT_AGENT.md, TASK_AGENT.md, GENERAL_AGENT.md)

### Archived Documents
- Session summaries (docs/archive/)
- One-time audits (docs/archive/)
- Old reference docs (docs/archive/)

---

## 🎯 Quick Start for Next Session

**Goal:** Begin Milestone 18 (Physics/State Split)

**Preparation:**
1. Read `src/game/game_manipulation.c` to understand current structure
2. Review physics code in `src/physics/` directory
3. Identify pure physics functions vs state mutations

**First Steps:**
1. Create `src/physics/physics_engine.h` with pure function signatures
2. Extract ball physics into pure functions
3. Write unit tests for extracted physics
4. Gradually migrate game_manipulation.c

**Timeline:** 7-8 sessions, ~10-12 hours total

---

## 📋 Testing Commands

```bash
# Build
make main
make clean

# Unit tests (48 tests)
devenv shell make test

# Integration tests (13 tests)  
devenv shell make integration_test

# Debug mode
./main --debug-state crash.json
```

---

**Last Updated:** 2026-01-18  
**Current Status:** Milestone 17 Complete ✅ | Ready for Milestone 18 🎯
