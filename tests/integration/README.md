# Integration Testing: Contract Tests

## Philosophy

Contract tests prove that **pipeline stages cooperate correctly** at the frame level. Unlike scenario tests (which simulate full games), contract tests set a precise precondition, run exactly 1 frame through the real pipeline, and verify the immediate reaction.

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

The key difference: scenario tests run hundreds of frames to simulate baseball plays.
Contract tests run 1 frame to verify a single stage reaction.

## Writing a Contract Test

```c
int test_referee_reacts_to_event(void)
{
    // 1. Create a standard game state
    ScenarioContext* ctx = create_scenario();

    // 2. Place actors in known positions
    place_runner_at_base(ctx, 0, BASE_FIRST, 0.0f);
    setup_batter_at_home(ctx, 1);
    move_pitcher_away(ctx);

    // 3. Initialize legal state from physical positions
    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);

    // 4. Set the specific precondition being tested
    ctx->state->match->gameEvents.catchMade = 1;
    ctx->state->match->betweenPitchState.batOutcome = BAT_OUTCOME_HIT;

    // 5. Run exactly 1 pipeline frame
    simulate_frames(ctx, 1);

    // 6. Assert the contract
    ASSERT_EQ(1, ctx->state->match->referee.woundingEvaluationActive,
              "Wounding evaluation should start");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
```

## Current Tests

| Test | Contract Verified |
|------|-------------------|
| `test_clear_frame_events_completeness` | `clearFrameEvents` clears ALL fields; size guard detects struct growth |
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
devenv shell make integration_test   # Contract tests (9 tests)
devenv shell make scenario_test      # Scenario tests (15 tests)
devenv shell make test               # Unit tests (61 tests)
```
