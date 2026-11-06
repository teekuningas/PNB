# Integration Testing: Contract Tests

## Philosophy

Contract tests prove that **pipeline stages cooperate correctly** at the frame level. Unlike scenario tests (which simulate full plays), contract tests set a precise precondition, run exactly 1 frame through the real pipeline, and verify the immediate reaction.

They answer questions like:
- "When `catchMade` fires, does the referee start wounding evaluation?"
- "When `pitchReleased` fires, does the referee snapshot the legal baseline?"
- "When the ball lands out of bounds after a hit, does the referee detect foul play?"

### Why 1-Frame?

1. **Precision:** Tests exactly one contract between stages, not emergent behavior
2. **Speed:** Milliseconds per test, not hundreds of frames
3. **Catches subtle bugs:** Frame-timing issues that scenario tests can miss
4. **Enables safe refactoring:** Proves stage cooperation survives structural changes

## Test Infrastructure

Contract tests share the same infrastructure as scenario tests:
- `fixtures.c/h` — State allocation and cleanup
- `scenario_builder.c/h` — `create_scenario()`, `simulate_frames()`, player placement

The key difference: scenario tests run hundreds of frames to simulate pesäpallo plays.
Contract tests run 1 frame to verify a single stage reaction.

## Writing a Contract Test

Start from a nearby test, such as
[`test_referee_reacts_to_catch.c`](contracts/test_referee_reacts_to_catch.c),
rather than copying a second example that can drift from the executable one:

1. Create a `ScenarioContext`, place the actors, and establish the legal pitch-start snapshot.
2. Set the precise precondition. Physical state and frame events live in `ctx->state->match`;
   legal state lives in `ctx->state->rules`, never inside `match`.
3. Stabilize unrelated physics, then call `simulate_frames(ctx, 1)`.
4. Assert the immediate verdict and relevant non-effects, and clean up the scenario.
5. Ablate the mechanism the test defends and require the test to fail.

## Examples (not the full inventory)

| Test | Contract Verified |
|------|-------------------|
| `test_clear_frame_events_completeness` | `clear_frame_events` clears ALL fields; size guard detects struct growth |
| `test_referee_starts_wounding_on_catch` | `catchMade` + conditions → wounding evaluation + WOUND_MARKED |
| `test_referee_snapshots_on_pitch_released` | `pitchReleased` → baseAtPitchStart, strikesAtPitchStart, betweenPitch cleared |
| `test_foul_detected_on_out_of_bounds_hit` | `ballHitGround` + batOutcome=HIT + OOB → foulState = DETECTED |
| `test_no_pending_runs_during_end_of_inning` | Pending runs blocked when endOfInningState != NONE |
| `test_no_free_walk_runs_during_end_of_inning` | Free walk runs blocked when endOfInningState != NONE |
| `test_compound_foul_and_end_of_inning` | Foul strike-3 → skip foul timer, go directly to END_INNING_STATE_DETECTED |
| `test_compound_hr_pair_and_uncatchable` | HR pair end + uncatchable → skip pair SM, go directly to END_INNING_STATE_DETECTED |
| `test_bat_outcome_promotion` | `ballHitByBat`/`ballMissedByBat` → batOutcome promoted; `pitchReleased` → batOutcome reset |

## Running

```bash
nix develop --command make integration_test   # Contract tests
nix develop --command make scenario_test      # Scenario tests
nix develop --command make test               # Unit tests
```

The runners report the current counts; `contracts/all_contracts.h` is the contract inventory.
