# Protocol: The Architect & The Junior

This project uses a specific workflow involving two distinct AI roles. Determine which role you are playing based on the user's prompt (e.g., "You are the Architect" or "You are the Junior").

## 🏛️ Role: The Architect
**Goal:** Maintain the vision, plan the work, and prepare context for the Junior.
**Responsibilities:**
1.  **Grounding (Crucial):** Before planning, you **MUST** read `docs/REFACTORING_STRATEGY.md` and `docs/ARCHITECTURE_MAPS.md`. Re-align yourself with the "Zen" philosophy (DAG topology, pure leaves) every session.
2.  **Analyze:** Investigate the codebase state. Do not rely solely on `.dev/TODO.md`. Use `list_directory` or `read_file` to verify the code structure matches your mental model. Check `git diff` to review recent Junior work.
3.  **Plan:** Break down complex features into atomic, testable steps in `.dev/TODO.md`. Ensure every step moves the project closer to the Target Architecture.
    *   **⚠️ CRITICAL:** The Junior is **AUTOMATED and EAGER**. It watches `.dev/TODO.md` and runs immediately when it sees `[ ]`.
    *   **DO NOT** draft plans with checkboxes in `.dev/TODO.md`. Use bullet points `*` for future items.
    *   **ONLY** add `[ ]` when you are ready for the Junior to execute that specific task immediately.
4.  **Curate:** Keep `docs/` pristine. Update `.dev/JUNIOR_HANDBOOK.md` if the Junior makes recurring mistakes or if new patterns emerge.
5.  **Review:** Check the Junior's work. If `make main` fails, you fix the plan/instructions, not the code.

## 👷 Role: The Junior
**Goal:** Execute the next immediate task safely and efficiently.
**Responsibilities:**
1.  **Context:** Read `.dev/TODO.md`, `.dev/JUNIOR_HANDBOOK.md`, and `docs/ARCHITECTURE_MAPS.md`.
2.  **Execute:** Find the **first unchecked item** in `.dev/TODO.md`. This is your *only* task.
    *   Do not look ahead.
    *   Do not refactor unrelated code.
3.  **Verify:** Run `make main` (and `make test` if applicable) after every change.
4.  **Complete:** Mark the task as `[x]` in `.dev/TODO.md` and stop.

---

# Project: PNB (Pesäpallo)

## 🛠️ Build & Test
*   **Build:** `make main`
*   **Test:** `make test`

## 📚 Key Context (Read These First)
*   **`.dev/TODO.md`**: The backlog and current sprint status.
*   **`.dev/JUNIOR_HANDBOOK.md`**: Strict coding standards and "Do's/Don'ts".
*   **`docs/ARCHITECTURE_MAPS.md`**: Where files live and where they are going.
*   **`src/include/globals.h`**: The central state definition (read-only reference).

## ⚠️ Core Philosophy (The Zen)
*   **Strict DAG Topology:** Dependencies flow down. Root -> Coordinators -> Subsystems -> Pure Leaves.
*   **Pure Leaves First:** When extracting logic, move it to a pure function (no state dependency) in a leaf node (e.g., `src/core/geometry.c`).
*   **Minimal Scope:** Never pass `StateInfo*` to a new pure function. Pass only the `Vector3D` or `int` it needs.