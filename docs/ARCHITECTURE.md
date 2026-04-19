# PNB Architecture

**Last updated:** 2026-04-14

## Documents

| Document | Purpose |
|----------|---------|
| **`OPUS_VISION.md`** | The architectural target — pipeline, ownership, communication patterns, disruptions, initialization. Sections I-X: the post-Phase-8 target. Section XI: client-server vision. **Section XII: verified readiness analysis** (serialization, headless peer, AI coupling). |
| **`PLAN.md`** | The step-by-step path from here to there. Lifecycle analysis, testing tiers, phase details. Phases 6-8 are the active plan. Future Work documents the path toward headless peer and intent layer. |
| **`KNOWN_BUGS.md`** | Active bugs with root causes. |

## Current Status

Phases 1–6.5 Complete ✅ (const-casts 12→0, 85 tests, contract testing, BatOutcome consolidation, pitchResult promotion, compiler-enforced ownership, get_batting_team_index extracted, should_period_end extracted, Bug #1+#2 fixed, end-of-inning run guards, compound reset detection, endPeriod exclusively referee-owned, Gather-Decide-Act for state machines) | Phase 7 Initialization Ownership 🎯

## Build & Test

```bash
# Build
make main

# Test (85 tests: 62 unit + 8 contract + 15 scenario)
devenv shell make test              # Unit tests
devenv shell make integration_test  # Contract tests (1-frame pipeline proofs)
devenv shell make scenario_test     # Scenario tests (full-game simulations)
```
