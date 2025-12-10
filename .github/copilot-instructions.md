# Agentic Development Protocol

This workflow uses two AI roles based on the user prompt.

## Role: Architect Agent

**Goal:** Plan work and dispatch atomic tasks.

**Responsibilities:**
1. Read `.dev/PLAN.md` and `docs/ARCHITECTURE_MAPS.md` for context
2. Analyze codebase state via git and file inspection
3. Create atomic, precise tasks in `.dev/TODO.md`
4. Review Task Agent work via git commits
5. Fix task descriptions if build fails, not code

**Task Format:**
- `.dev/TODO.md` contains ONLY immediate tasks
- Each task is one checkbox: `- [ ] Create file X with function Y`
- Remove completed tasks after verification
- No history, no roadmap, no ambiguity

**Key Context Files:**
- **`.dev/TODO.md`**: Current task queue (execution only)
- **`.dev/PLAN.md`**: Master plan, history, backlog (Architect's notes)
- **`docs/ARCHITECTURE_MAPS.md`**: System architecture and file organization
- **`src/include/globals.h`**: Central state definition (reference)

## Role: Task Agent

**Read TASK_AGENT.md for the complete protocol.**

Summary: Execute the first unchecked task in `.dev/TODO.md`, test it with `make main` (and `make test`), commit it, update TODO.md with the commit hash, commit the TODO update, and stop.

---

## Project: PNB (Pesäpallo)

### Build & Test
- **Build:** `make main`
- **Test:** `make test`

### Core Philosophy (The Zen)
- **Strict DAG Topology:** Dependencies flow down: Root → Coordinators → Subsystems → Pure Leaves
- **Pure Leaves First:** Extract logic to pure functions (no state dependency) in leaf nodes (e.g., `src/core/geometry.c`)
- **Minimal Scope:** Never pass `StateInfo*` to new pure functions. Pass only the specific data needed (e.g., `Vector3D`, `int`)
