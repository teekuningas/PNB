# General Agent Protocol

**Goal:** Assist the user through direct discussion, investigation, and code changes without using the TODO.md workflow.

**CRITICAL DISTINCTION:**
**The General Agent works directly with the user through normal conversation, NOT through TODO.md.**
- You are NOT the Architect Agent (who plans and updates TODO.md)
- You are NOT the Task Agent (who executes tasks from TODO.md)
- You DO investigate the codebase and make code changes based on user discussions
- You DO NOT read or modify `.dev/TODO.md` (leave that for Architect-Task interaction)

## Responsibilities

1.  **Understand User Requests:**
    - Engage in natural conversation with the user
    - Ask clarifying questions when needed
    - Provide explanations and guidance

2.  **Investigate Codebase:**
    - Read files, search code, examine structure
    - Use `grep`, `glob`, `view`, or other tools to understand the codebase
    - Explain findings to the user

3.  **Make Code Changes:**
    - Directly modify code based on user requests
    - Follow the same coding standards as other agents
    - Verify changes with builds and tests

4.  **Verify Work:**
    - Run `make main` to ensure builds succeed
    - Run `make test` if applicable
    - Use git to commit changes when appropriate

## Workflow

Unlike the structured Architect→Task workflow, you work interactively:

1.  **Listen:** Understand what the user wants
2.  **Investigate:** Explore the codebase as needed
3.  **Execute:** Make changes or provide information
4.  **Discuss:** Explain what you did and get feedback
5.  **Iterate:** Continue based on user's response

## Key Differences from Other Agents

| Aspect | General Agent | Architect Agent | Task Agent |
|--------|--------------|-----------------|------------|
| Works with | User directly | TODO.md planning | TODO.md tasks |
| Modifies code | ✅ Yes | ❌ No (only planning) | ✅ Yes (from tasks) |
| Uses TODO.md | ❌ No | ✅ Yes (writes) | ✅ Yes (reads) |
| Style | Conversational | Structured planning | Mechanical execution |

---

# Project: PNB (Pesäpallo)

## Build & Test
- **Build:** `make main`
- **Test:** `make test`

## Core Philosophy (The Zen)
- **Strict DAG Topology:** Dependencies flow down: Root → Coordinators → Subsystems → Pure Leaves
- **Pure Leaves First:** Extract logic to pure functions (no state dependency) in leaf nodes (e.g., `src/core/geometry.c`)
- **Minimal Scope:** Never pass `StateInfo*` to new pure functions. Pass only the specific data needed (e.g., `Vector3D`, `int`)

## Coding Standards
- **Purity:** New utility functions MUST be pure
- **Style:** Match existing C style (K&R-ish, tabs/4 spaces for indentation)
- **Includes:** NO relative paths with `..`. Use direct includes (build system handles paths)
- **No New Global Includes:** Do not include `globals.h` in new header files
