# PNB - Documentation

**Milestone 13.5 COMPLETE ✅** (2026-01-06)  
**Current Focus:** Milestone 14 - The Great Decoupling

### What We've Achieved
- **Referee State Pattern:** Implemented a centralized `RefereeState` to track authoritative rule data (snapshots, pending wounds) separate from physical player state.
- **Legacy Cleanup:** Completely removed `originalBase` and other legacy flags, ensuring a single source of truth for rule decisions.
- **Robust Rule Logic:** Explicitly enumerated and implemented complex rules like "Tuplahaava" (Double Wound) and its exceptions.
- **Logic State Consolidation:** Eliminated all logic-related `static` variables. `StateInfo` is the single source of truth.
- **Rendering Unification:** Adopted `ResourceManager` for all in-game assets and eliminated logic-related `static` GL variables.
- **Rule-Movement Decoupling:** Decoupled game rules (outs/strikes) from physical movement while maintaining automatic triggers for unambiguous control.
- **Type-safe domain state:** (PlayerUnitState, BaseID, GameEventType)
- **Comprehensive Testing:** Passed 53 unit tests and 14 complex integration scenario tests covering physics, AI, and rules.

### What's Next
**Philosophy:** With the rules stabilized and centralized, we can now safely decouple the "Query" (Read) and "Apply" (Write) phases of the game loop.

---

## Quick Start

### Understanding the Codebase
1. Read `.dev/PLAN.md` - High-level roadmap and current status
2. Read `.dev/TODO.md` - Current tasks and detailed steps
3. Read this README - Documentation guide

### Key Documents

**Strategy & Planning:**
- `DATA_STRUCTURE_STRATEGY.md` - **START HERE** - Why data cleanup before Referee
- `DATA_STRUCTURE_VISION.md` - **TARGET STATE** - The "After" picture of our data models
- `MILESTONE7_POSITION_ANALYSIS.md` - Comprehensive analysis before next steps
- `REFACTORING_STRATEGY.md` - Historical refactoring decisions

**Technical Documentation:**
- `DATA_STRUCTURE_AUDIT.md` - **AUDIT** - Detailed mapping of "God Objects" to clean structures
- `ARCHITECTURE_MAPS.md` - Component relationships and data flow
- `MIGRATION_AUDIT.md` - Milestone 7 verification audit
- `DATA_AUDIT.md` - Pre-Milestone 7 analysis (historical)

**Completed Milestones:**
- `MILESTONE6_COMPLETE.md` - Rules engine extraction complete
- `MILESTONE11_COMPLETE.md` - (To be created) State consolidation complete
- `MILESTONE12_COMPLETE.md` - (To be created) Rendering unification complete

**Game Rules:**
- `SAANNOT.md` - Finnish baseball rules (in Finnish)

---

## Document Guide

### 📋 .dev/ - Active Planning (Read First!)

**PLAN.md** - Master roadmap
- Current milestone status
- Next steps
- Completed milestones
- Decision log

**TODO.md** - Current tasks
- Detailed checklist for current milestone
- What to do today/tomorrow
- Phase breakdowns

### 📚 docs/ - Reference Documentation

#### Strategy Documents (Understanding WHY)

**DATA_STRUCTURE_STRATEGY.md** ⭐ **IMPORTANT**
- Why data cleanup before Referee pattern
- Detailed analysis of control flag pollution
- The path forward (Milestone 7.5 → 8)
- ~600 lines of deep strategic thinking

**MILESTONE7_POSITION_ANALYSIS.md**
- Comprehensive assessment after Milestone 7
- Data model analysis
- Pure functions assessment
- Test suite review
- Risk assessment for Milestone 8

**REFACTORING_STRATEGY.md**
- Historical refactoring decisions
- Lessons learned
- Patterns that worked/didn't work

#### Technical Documents (Understanding HOW)

**ARCHITECTURE_MAPS.md**
- System component diagram
- Data flow visualization
- Layer boundaries
- Dependency relationships

**MIGRATION_AUDIT.md**
- Milestone 7 verification
- Field-by-field analysis
- Write/read migration audit
- Safety verification

**DATA_AUDIT.md** (Historical)
- Pre-Milestone 7 state
- Original data model mess
- Problems identified
- Solutions proposed

#### Milestone Completions

**MILESTONE6_COMPLETE.md**
- Rules engine extraction
- Pure function creation
- Test coverage achieved

*(More to be added as we complete milestones)*

---

## The Refactoring Journey

### Completed Milestones

**Milestone 5: Logic Purification**
- Extracted physics to pure functions
- Extracted AI to pure functions
- Created unit test foundation

**Milestone 6: Rules Engine Extraction**
- Extracted outs/runs/strikes to `rules_pure/`
- Comprehensive audit (found 1 bug!)
- 100% rules coverage

**Milestone 7: Data Renaissance** ✅
- Eliminated legacy state flags
- Type-safe enums (PlayerUnitState, BaseID, GameEventType)
- Write-both pattern migration
- Deleted state_adapter.c/h
- All tests passing

**Milestone 11: State Consolidation** ✅
- Moved all logic-related statics into `StateInfo`.
- Centralized AI and Game Flow counters.

**Milestone 12: Rendering Unification** ✅
- Modernize in-game rendering to use `ResourceManager`.
- Eliminate orphan GLuint globals.

### Current Work

**Milestone 14: The Great Decoupling**
- Decouple Read/Write logic.
- Split analysis and manipulation into distinct phases.

### Future Work

**Milestone 15+: The Functional Pipeline**
- Implement Intent Phase.
- Implement Referee Phase (Judgment).
- Explicit linear game loop.

---

## Key Principles

### 1. Data Shapes Architecture
Clean data structures make good architecture obvious. Messy data makes good architecture impossible.

### 2. Pure Functions First
Extract logic to pure functions before changing architecture. This makes the system testable and understandable.

### 3. Small, Safe Steps
Make the smallest possible change. Test. Then make the next small change.

### 4. Tests Are Safety Nets
53 unit tests give us confidence to refactor boldly.

### 5. Foundation Before Skyscraper
Don't build complex architectural patterns on messy foundations. Clean first, build second.

---

## Contributing

### For New Contributors
1. Read `DATA_STRUCTURE_STRATEGY.md` - understand our current focus
2. Read `.dev/PLAN.md` - understand the roadmap
3. Read `.dev/TODO.md` - see current tasks
4. Pick an unchecked box and start coding!

### For AI Agents
See `.dev/ARCHITECT_AGENT.md`, `.dev/TASK_AGENT.md`, or `.dev/GENERAL_AGENT.md` for your specific role.

---

## Questions?

**"Where are we now?"** → Read `.dev/PLAN.md`

**"What should I do next?"** → Read `.dev/TODO.md`

**"Why this approach?"** → Read `docs/DATA_STRUCTURE_STRATEGY.md`

**"How does the system work?"** → Read `docs/ARCHITECTURE_MAPS.md`

**"What have we achieved?"** → Read `docs/MILESTONE*_COMPLETE.md` files

---

**Last Updated:** 2026-01-05  
**Status:** Stabilizing rules and unifying rendering 🚀
