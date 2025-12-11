# Agentic Development Protocol

This workflow uses two AI roles based on the user prompt.

## Role: Architect Agent

**Read `.dev/ARCHITECT_AGENT.md` for the complete protocol.**

**Goal:** Plan work and dispatch atomic tasks.

**CRITICAL:** The Architect Agent **MUST NOT** modify the source code. Its only output is updating `.dev/TODO.md` and `.dev/PLAN.md`.

**Responsibilities:**
1. Read `.dev/PLAN.md` and `docs/ARCHITECTURE_MAPS.md`
2. Analyze codebase state
3. Create atomic tasks in `.dev/TODO.md`

**Key Context Files:**
- **`.dev/TODO.md`**: Current task queue
- **`.dev/PLAN.md`**: Master plan and history
- **`docs/ARCHITECTURE_MAPS.md`**: System architecture


## Role: Task Agent

**Read `.dev/TASK_AGENT.md` for the complete protocol.**

Summary: Execute the first unchecked task in `.dev/TODO.md`, test it with `make main` (and `make test`), commit it, update TODO.md with the commit hash, commit the TODO update, and stop.

---

## Project: PNB (Pesäpallo)

### Build & Test
- **Build:** `devenv shell make main`
- **Test:** `devenv shell make test`

### Core Philosophy (The Zen)
- **Strict DAG Topology:** Dependencies flow down: Root → Coordinators → Subsystems → Pure Leaves
- **Pure Leaves First:** Extract logic to pure functions (no state dependency) in leaf nodes (e.g., `src/core/geometry.c`)
- **Minimal Scope:** Never pass `StateInfo*` to new pure functions. Pass only the specific data needed (e.g., `Vector3D`, `int`)
