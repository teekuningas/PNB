# Junior Developer Handbook

This document contains the strict operational rules for executing tasks in the PNB codebase.

## 1. The Golden Rules
*   **Never Break the Build:** Run `make main` after *every* file modification. If it fails, fix it immediately.
*   **Atomic Steps:** Do exactly what the `TODO.md` asks. Do not "fix" other things you see along the way.
*   **No New Global Includes:** Do not include `globals.h` in new header files. Use forward declarations if possible, or include only specific pure headers.

## 2. Coding Standards
*   **Purity:** New utility functions MUST be pure.
    *   ❌ `void calculate_distance(StateInfo* state)`
    *   ✅ `float geometry_distance(Vector3D a, Vector3D b)`
*   **Style:** Match the existing C style (K&R-ish).
    *   Tabs for indentation (or 4 spaces, check surrounding code).
    *   `camelCase` for variables, `snake_case` for file names and pure functions.
*   **Includes:** NO relative paths with `..`. The build system handles include paths.
    *   ❌ `#include "../core/geometry.h"`
    *   ✅ `#include "geometry.h"`
*   **Safety:** Always check pointers for `NULL` if they are passed as arguments (unless performance critical and internal).

## 3. Refactoring Patterns
When extracting code from `src/game/` to `src/core/` or `src/physics/`:
1.  **Copy First:** Copy the logic to the new pure file.
2.  **Verify Pure:** Ensure the new function depends ONLY on its arguments.
3.  **Update Build:** Add the new `.o` file to `Makefile` **immediately**.
3a. **Check Includes:** If you created a new directory (e.g., `src/physics`), ensure it is added to the `IDIR` variable in `Makefile` (e.g., `-I./src/physics`).
4.  **Replace:** In the old file, include the new header and replace the logic with a call to the new function.

## 4. Troubleshooting
*   *Linker Error (undefined reference):* You probably forgot to add the new `.o` file to the `_OBJ` list in `Makefile`.
*   *Implicit Declaration:* You forgot to include the header file (`.h`) in the C file using the function.
