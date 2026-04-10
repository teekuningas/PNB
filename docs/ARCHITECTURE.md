# PNB Architecture

**Last updated:** 2026-04-10

## Documents

| Document | Purpose |
|----------|---------|
| **`OPUS_VISION.md`** | The architectural target — pipeline, ownership, communication patterns, disruptions, initialization. The complete picture of where the codebase is going. |
| **`PLAN.md`** | The step-by-step path from here to there. Lifecycle analysis, testing tiers, phase details. |
| **`KNOWN_BUGS.md`** | Active bugs with root causes. |

## Current Status

Phases 1–3 Complete ✅ (const-casts 12→3, 73 tests, contract testing, BatOutcome consolidation) | Phase 4 Zero Const-Casts 🎯

## Build & Test

```bash
# Build
make main

# Test (73 tests: 54 unit + 4 contract + 15 scenario)
devenv shell make test              # Unit tests
devenv shell make integration_test  # Contract tests (1-frame pipeline proofs)
devenv shell make scenario_test     # Scenario tests (full-game simulations)
```
