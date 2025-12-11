# Task Agent Protocol

## Execute these 8 steps in order

1. **Read TODO.md** - Identify the first line with `- [ ]` in `.dev/TODO.md`

2. **Execute the task** - Do exactly what that line says, nothing more

3. **Verify** - Run `make main` (and `make test` if applicable) to ensure your changes work

4. **Commit your work**
   ```bash
   git add .
   git commit -m "brief description"
   ```

5. **Get the commit hash** - You need this for step 6
   ```bash
   git rev-parse --short HEAD
   ```
   This outputs something like: `a1b2c3d`

6. **Update TODO.md** - Edit `.dev/TODO.md` to mark the task complete
   
   Change this:
   ```
   - [ ] Create cat.txt with meow
   ```
   
   To this (using the hash from step 5):
   ```
   - [x] Create cat.txt with meow (commit: a1b2c3d)
   ```

7. **Commit the TODO update**
   ```bash
   git add .dev/TODO.md
   git commit -m "Update TODO with completion"
   ```

8. **Stop** - Do not continue to the next task

## Important Rules

- Complete all 8 steps in order
- Step 5 output is required for step 6
- The commit hash must be included in TODO.md
- Do one task per run, then stop
- If the task is unclear, stop and report the issue
- Never modify code unrelated to the task

## PNB-Specific Guidelines

### Build Verification
- **Never Break the Build:** Run `make main` after every file modification
- If build fails, fix it immediately before proceeding

### Coding Standards
- **Purity:** New utility functions MUST be pure
  - ❌ `void calculate_distance(StateInfo* state)`
  - ✅ `float geometry_distance(Vector3D a, Vector3D b)`
- **Style:** Match existing C style (K&R-ish)
  - Tabs for indentation (or 4 spaces, check surrounding code)
  - `camelCase` for variables, `snake_case` for file names and pure functions
- **Includes:** NO relative paths with `..`. The build system handles include paths
  - ❌ `#include "../core/geometry.h"`
  - ✅ `#include "geometry.h"`
- **No New Global Includes:** Do not include `globals.h` in new header files

### Refactoring Patterns
When extracting code from `src/game/` to `src/core/` or `src/physics/`:
1. **Copy First:** Copy the logic to the new pure file
2. **Verify Pure:** Ensure the new function depends ONLY on its arguments
3. **Update Build:** Add the new `.o` file to `Makefile` immediately
4. **Check Includes:** If you created a new directory (e.g., `src/physics`), ensure it is added to `IDIR` variable in `Makefile`
5. **Replace:** Include the new header and replace the logic with a call to the new function

### Troubleshooting
- *Linker Error (undefined reference):* Add the new `.o` file to the `_OBJ` list in `Makefile`
- *Implicit Declaration:* Include the header file (`.h`) in the C file using the function

---

# Project: PNB (Pesäpallo)

## Build & Test
- **Build:** `make main`
- **Test:** `make test`

## Core Philosophy (The Zen)
- **Strict DAG Topology:** Dependencies flow down: Root → Coordinators → Subsystems → Pure Leaves
- **Pure Leaves First:** Extract logic to pure functions (no state dependency) in leaf nodes (e.g., `src/core/geometry.c`)
- **Minimal Scope:** Never pass `StateInfo*` to new pure functions. Pass only the specific data needed (e.g., `Vector3D`, `int`)
