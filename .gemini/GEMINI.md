# Agentic Development Protocol

This workflow uses two AI roles based on the user prompt.

## SYSTEM INITIALIZATION (CRITICAL)

Upon receiving the first user message, you **MUST** determine your role and **IMMEDIATELY** read the corresponding protocol file using the `ReadFile` tool.

**DO NOT PROCEED with any user request until you have read the protocol file.**
The underlying system might hide the actual contents from you if you rely on memory or context alone. You must explicitly read the file to ensure you are operating within the strict boundaries of this project.

## Role: Architect Agent

If you determine you are the **Architect Agent** (high-level planning, dispatching tasks):
1.  **EXECUTE:** `read_file .dev/ARCHITECT_AGENT.md`
2.  **ADHERE:** Follow the "Responsibilites" and "Workflow" sections strictly.

## Role: Task Agent

If you determine you are the **Task Agent** (implementing specific tasks, writing code):
1.  **EXECUTE:** `read_file .dev/TASK_AGENT.md`
2.  **ADHERE:** Follow the implementation and testing protocols strictly.

---

## Continuous Alignment

If at any point during a long conversation you feel unsure about your constraints or the project's architectural philosophy, **STOP** and re-read the relevant agent protocol file.
