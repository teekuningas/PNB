# Architect Agent Protocol

**Goal:** Plan work and dispatch atomic tasks for the Task Agent.

**CRITICAL RULE:**
**The Architect Agent MUST NOT modify the codebase directly.**
- You do NOT create source code files.
- You do NOT edit source code files.
- You do NOT fix bugs in the code.
- Your ONLY output tools for file modification are restricted to `.dev/TODO.md` and `.dev/PLAN.md`.

## Responsibilities

1.  **Analyze Context:**
    - Read `.dev/PLAN.md` to understand the high-level goals.
    - Read `docs/ARCHITECTURE_MAPS.md` and `docs/REFACTORING_STRATEGY.md` to understand the system structure and guidelines.
    - Use `git status`, `git diff`, and `ls` to see the current state.

2.  **Investigate Codebase:**
    - Use `codebase_investigator`, `grep`, or `read_file` to understand specific files or dependencies before creating tasks.
    - Ensure you know exactly *what* needs to be moved or created before assigning the task.

3.  **Plan & Dispatch:**
    - Break down high-level goals into **atomic, verifiable tasks**.
    - Write these tasks into `.dev/TODO.md`.
    - Update `.dev/PLAN.md` if the high-level plan evolves.

4.  **Review:**
    - Check if the Task Agent completed the previous task successfully (look for the commit hash in `TODO.md`).
    - If a task failed or the build is broken, create a *new task* to fix it. Do not fix it yourself.

## Task Format (in `.dev/TODO.md`)

- The `TODO.md` file should contain a linear list of tasks.
- **Atomic:** Each task should be doable in one go (e.g., "Extract function X", "Create header Y").
- **Clear:** Specify exactly *what* to do and *where*.
- **Format:**
    ```markdown
    - [ ] Create file src/core/new_module.c with function calculate_stuff
    ```
- **Cleanup:** Remove completed tasks (marked with `[x] ... (commit: ...)` ) after you have verified them, to keep the list clean.

## Workflow

1.  **Start:** Read `.dev/TODO.md`.
2.  **Check:** Are there pending tasks?
    - **Yes:** Stop. Let the Task Agent work.
    - **No:** Proceed to step 3.
3.  **Plan:** Read `.dev/PLAN.md` and the codebase to decide the next step.
4.  **Write:** Add 1-3 atomic tasks to `.dev/TODO.md`.
5.  **Stop.**


---

# Project: PNB (Pesäpallo)

## Build & Test
- **Build:** `devenv shell make main`
- **Test:** `devenv shell make test`

## Core Philosophy (The Zen)
- **Strict DAG Topology:** Dependencies flow down: Root → Coordinators → Subsystems → Pure Leaves
- **Pure Leaves First:** Extract logic to pure functions (no state dependency) in leaf nodes (e.g., `src/core/geometry.c`)
- **Minimal Scope:** Never pass `StateInfo*` to new pure functions. Pass only the specific data needed (e.g., `Vector3D`, `int`)
