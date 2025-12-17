# PNB Documentation

## Living Documents (Active Reference)

- **`ARCHITECTURE_MAPS.md`** - Current system structure, file organization, dependency flow
- **`REFACTORING_STRATEGY.md`** - Refactoring philosophy, patterns, guidelines
- **`SAANNOT.md`** - Official Pesäpallo rules (Finnish) - reference for rule implementations
- **`DATA_AUDIT.md`** - Data structure analysis for Milestone 7 (enum candidates)
- **`MILESTONE6_COMPLETE.md`** - Summary of completed logic purification work

## Working Documents (Project Management)

- **`.dev/PLAN.md`** - Master refactoring plan, tracks milestones and progress
- **`.dev/TODO.md`** - Atomic tasks for Task Agent (updated by Architect)

## Historical Archive

Detailed audit reports and methodology documents:
- `archive/MILESTONE6_*.md` - Detailed audit reports
- `archive/REFACTORING_AUDIT_PLAN.md` - Audit methodology
- `archive/*_audit.md` - Function-by-function verification details

**When to use archive:**
- Need to understand HOW a verification was done
- Debugging similar issues in future
- Historical reference for methodology

**Day-to-day work:** Use living documents above, not archive.

---

## Quick Start for New Sessions

1. Read `.dev/PLAN.md` - Current status and next milestone
2. Read `MILESTONE6_COMPLETE.md` - Context of where we are
3. Check `DATA_AUDIT.md` if working on Milestone 7
4. Reference `ARCHITECTURE_MAPS.md` for system structure
5. Consult `archive/` only if you need detailed audit specifics
