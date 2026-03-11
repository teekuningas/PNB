# PNB Deep Investigation Protocol

You are working on a very large, highly complex, and strictly architected C codebase. Standard context-saving measures or superficial investigation WILL result in failure here. You must prioritize deep, accurate understanding over minimizing token usage.

## 1. The "No Guesswork" Mandate

**Override Context Saving:** While your general system instructions might encourage you to minimize file reads to save context, **for this specific project, you are explicitly authorized and required to read full files when investigating.** If you need to understand how a subsystem works, read the relevant source files in their entirety. Do not try to guess logic or memory layouts based on a few lines of `grep` output.

## 2. Embrace the Complexity (Marvel and Dig)

This codebase is a complex, living machine. Treat it with curiosity and respect. 
*   **Follow the Thread:** When you find a struct, don't just look at where it's used right now; ask yourself, "Who owns this? Where is it initialized? What happens to it between frames?"
*   **Read the `struct` Definitions:** In C, data is king. You cannot understand the logic without intimately knowing the memory layout in `globals.h`.
*   **Trace Dependencies:** Follow the data flow. If a function calls another function in a different file, or modifies a struct defined elsewhere, go read those files too. Keep digging until you reach the bottom of the call stack or the root of the data structure.

## 3. Investigative Workflow

When asked to plan a feature, analyze a problem, or review a step in the refactoring plan:
1.  **Initial Scan:** Start with `grep_search` to find relevant keywords, structs, and function names.
2.  **Full Context:** Identify the core files involved and use `read_file` to read them completely. You need to see the `#include`s, the file-scoped statics, and the full flow of logic.
3.  **Double-Check:** Always verify your assumptions. Prove it by thoroughly searching the entire codebase.

## 4. Independent Verification

Never blindly trust existing planning documents, markdown files, or even your own past summaries. 
*   Read the plans, but then **verify their claims against the actual, current codebase.** 
*   If a plan says "Variable X is only used in File Y", you must verify if that is still true today. Plans become outdated; the compiler does not lie.

## 5. First-Principles Planning

Before making any code changes, formulate a sound rationale based on your deep reading.
*   Explain *why* a change makes sense, not just *what* the change is. Relate your decisions to standard software engineering principles (e.g., Data-Oriented Design, Single Source of Truth, Const-Correctness).
*   If something feels "hacky" or violates the established patterns of the file, pause and investigate further. There is usually a reason the code was written that way.

## 6. Falsifiability

Your execution plans must be precise and testable. Define exactly what files will change, how they will change, and how the C compiler or the existing tests will prove your change is correct and safe.

## 7. Proactive Autonomy (Self-Confidence)

You do not need to ask for permission to begin your investigation or to use your tools. Once you have read these instructions and any relevant planning documents, you MUST immediately start using `grep_search`, `read_file`, and other tools to verify the codebase state and investigate the next steps. Do not wait for the user to say "go ahead and look." Trust your own analysis, be confident, and take the initiative to dig into the code right away.