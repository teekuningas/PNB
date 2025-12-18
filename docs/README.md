# PNB Documentation

## Quick Start (Start Here!)

**Current Status:** Milestone 6 Complete (8.5/10) → Starting Milestone 7 (Data Renaissance)

**For new session:**
1. Read `.dev/PLAN.md` - Master plan, current status, next milestone details
2. Read `MILESTONE6_COMPLETE.md` - Summary of completed work and semantic discoveries
3. Read `ARCHITECTURE_MAPS.md` - System structure, file organization, target architecture

**Starting Milestone 7 (Data Renaissance):**
- Read `DATA_AUDIT.md` - Enum proposals and migration strategy
- See `.dev/PLAN.md` Milestone 7 section for phased approach

---

## Living Documents (Active Reference)

- **`ARCHITECTURE_MAPS.md`** (253 lines) - Current & target architecture, file organization, functional dataflow principles
- **`MILESTONE6_COMPLETE.md`** (271 lines) - Completed logic purification, bug fixes, semantic insights
- **`DATA_AUDIT.md`** (97 lines) - Enum proposals for Milestone 7 (updated for data-first approach)
- **`REFACTORING_STRATEGY.md`** (487 lines) - Refactoring philosophy, milestone roadmap, patterns
- **`SAANNOT.md`** (2686 lines) - Official Pesäpallo rules (Finnish) - reference for rule implementations

---

## Working Documents (Project Management)

- **`.dev/PLAN.md`** - Master refactoring plan (updated 2025-12-18)
  - Tracks all milestones (1-8+)
  - Current status and next steps
  - Data-first Milestone 7 strategy with safety layers
  - Functional dataflow architecture (NOT event-driven)
  
- **`.dev/TODO.md`** - Atomic tasks for Task Agent
  - Updated by Architect only
  - Flat list of checkboxes
  - Small batches (1-5 tasks)
  - Currently empty (awaiting Milestone 7 planning)

---

## Historical Archive

Detailed audit reports and methodology (for reference only):
- `archive/MILESTONE6_*.md` - Detailed function-by-function audits
- `archive/REFACTORING_AUDIT_PLAN.md` - Audit methodology
- `archive/*_audit.md` - Verification details

**When to use archive:**
- Understanding HOW verification was done
- Debugging similar issues
- Historical reference

**Day-to-day work:** Use living documents above, not archive.

---

## Key Architecture Principles (Updated 2025-12-18)

✅ **Functional dataflow** - Synchronous function calls (main → coordinators → subsystems → pure)  
✅ **Data flows in/out** - Like breathing (not sideways between subsystems)  
✅ **Pure functions dominate** - 80% pure logic, 20% coordination  
✅ **DAG topology** - Dependencies flow down only  

❌ **No event buses** - No message queues or pub/sub patterns  
❌ **No observer pattern** - No callbacks or listeners  
❌ **No async messaging** - Synchronous, debuggable execution  

---

## Document Status Summary

| Document | Lines | Status | Last Updated |
|----------|-------|--------|--------------|
| ARCHITECTURE_MAPS.md | 253 | ✅ Current | 2025-12-18 |
| DATA_AUDIT.md | 97 | ✅ Current | 2025-12-18 |
| MILESTONE6_COMPLETE.md | 271 | ✅ Final | 2025-12-17 |
| REFACTORING_STRATEGY.md | 487 | ✅ Current | 2025-12-18 |
| SAANNOT.md | 2686 | ✅ Reference | (unchanging) |
| .dev/PLAN.md | ~350 | ✅ Current | 2025-12-18 |
| .dev/TODO.md | ~5 | ✅ Current | Empty (ready) |
