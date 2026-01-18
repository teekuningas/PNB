# PNB Documentation

This directory contains the core project documentation.

## Active Documents

### 1. [ARCHITECTURE.md](ARCHITECTURE.md)
High-level system design, data structures, and refactoring roadmap.
- Current state and vision
- Milestone progress
- Key data structures
- Referee pattern
- Game loop reference

### 2. [SAANNOT.md](SAANNOT.md)
Pesäpallo rules reference (Finnish).
- Official game rules
- Section references (§33-§42, etc.)
- Rule interpretations

### 3. [../.dev/PLAN.md](../.dev/PLAN.md)
Master development roadmap and session tracker.
- Current phase status
- Completed work
- Remaining tasks
- Milestone planning

## Archived Documentation

Older session notes, audits, and one-time analyses have been moved to `archive/` for reference.

## For Contributors

**Start here:**
1. Read ARCHITECTURE.md for system overview
2. Check .dev/PLAN.md for current status and next steps  
3. Review SAANNOT.md for game rules when working on referee logic

**Key Principles:**
- **Referee Supremacy:** Only referee.c writes to RefereeState, BetweenPitchState
- **One-Way Flow:** Physics → Events → Referee → Decisions → Reconciliation
- **Clear Ownership:** Each struct has one writer, many readers
- **Test Coverage:** All rule changes must have integration tests

**Testing:**
```bash
devenv shell make test             # Unit tests (48)
devenv shell make integration_test # Integration tests (13)
```
