# PNB - Pesäpallo Game

A 3D Finnish baseball (Pesäpallo) game in C with OpenGL. Currently undergoing architectural refactoring toward a functional pipeline.

## Status: Milestone 14 Complete ✅

**What works:** Game playable, 67 tests passing, referee logic decoupled  
**What's next:** Action system refactoring (M15)

## Quick Start

```bash
make main                                 # Build
./main                                    # Run
./main --debug-state crash.json          # Debug mode

devenv shell make test                    # Unit tests
devenv shell make integration_test        # Integration tests
```

## Current Architecture (Jan 2026)

**Completed:**
- ✅ M14: Referee split (pure analysis + state mutation)
- ✅ State validator with runtime checks
- ✅ Eliminated `baseControlIndex` → per-player safety tracking
- ✅ 67 tests (53 unit + 14 integration)

**In Progress:**
- 🚧 M15: Action system decoupling (batting, pitching, throwing)

**Next:**
- M16: Intent phase (decouple input from execution)
- M17: Full functional pipeline

## Codebase

```
src/
├── game/
│   ├── rules_pure/      Pure rule logic (referee.c, base_logic.c, etc)
│   ├── referee_apply.c  State mutation
│   ├── actions_messy/   TO REFACTOR (batting, pitching, throwing)
│   └── ai_messy/        TO REFACTOR (batting, catching AI)
├── core/                Platform, I/O, state_validator.c
├── menu/                Menu systems
└── tests/               67 tests

Stats: ~15k LOC, 28 game files, 6 pure rules, 2 pure actions
```

## Target Architecture

**"Linear Functional Loop":**
```
Input → Intent → Physics → Referee(Analyze) → Apply → Render
```

See [ARCHITECTURE.md](ARCHITECTURE.md) for details.

## Key Files

- `src/game/rules_pure/referee.c` - Pure rule analysis (310 LOC)
- `src/game/referee_apply.c` - State mutation (149 LOC)
- `src/core/state_validator.c` - Runtime invariant checker (199 LOC)
- `src/include/globals.h` - All types (773 LOC)

## Docs

- **[ARCHITECTURE.md](ARCHITECTURE.md)** - Technical deep dive
- **[SAANNOT.md](SAANNOT.md)** - Finnish Pesäpallo rules
- **[.dev/TODO.md](../.dev/TODO.md)** - Current tasks
- **[.dev/PLAN.md](../.dev/PLAN.md)** - Roadmap

## Tech Stack

C (C99), OpenGL 3.3+, GLFW, GLEW, Make, devenv (Nix)

---

**Updated:** 2026-01-07 | **Contact:** teekuningas
