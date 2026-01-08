# Full-Scenario Integration Testing

## Philosophy

These tests simulate **realistic game scenarios** from start to finish, rather than manually manipulating internal state to create "snapshot" conditions.

### Why Full-Scenario?

1. **Robustness:** Tests survive refactoring because they use public APIs
2. **Learning:** Writing tests reveals missing documentation and coupled flags
3. **Realism:** Tests validate actual gameplay, not contrived states
4. **Design Tool:** Test difficulty indicates architectural problems

## Test Patterns

### Pattern 1: Movement Tests (Long Duration)

Tests that involve physical player movement require ~300-500 frames:

```c
static int test_runner_scores_from_third(void) {
    ScenarioContext* ctx = create_scenario();
    
    // Setup initial state
    place_runner_at_base(ctx, 0, BASE_THIRD, 0.0f);
    ctx->state->localGameInfo->playerRuntime[0].passedPathPoint = 1;
    
    // Trigger movement
    trigger_player_run_to_next_base(ctx, 0, BASE_THIRD);
    
    // Allow time for physical completion
    simulate_frames(ctx, 450);
    
    // Verify outcome
    ASSERT_EQ(1, ctx->state->localGameInfo->gameState.runsInTheInning);
}
```

### Pattern 2: Logic Tests (Single Frame)

Tests of referee logic/decisions can run in 1-10 frames:

```c
static int test_force_out(void) {
    ScenarioContext* ctx = create_scenario();
    
    // Setup "moment of truth" state
    setup_batter_at_home(ctx, 0);
    ctx->state->localGameInfo->playerInfo[0].bTPI.state = PLAYER_STATE_RUNNING;
    give_ball_to_fielder(ctx, 13);  // First baseman has ball
    
    // Single analysis cycle
    simulate_frames(ctx, 1);
    
    // Verify decision
    ASSERT_EQ(1, ctx->state->localGameInfo->gameState.outs);
}
```

## Available Helpers

### Scenario Creation
- `create_scenario()` - Fresh game state
- `cleanup_scenario(ctx)` - Free resources

### Player Setup
- `place_runner_at_base(ctx, playerIdx, base, progress)` - Position with safety
- `setup_batter_at_home(ctx, playerIdx)` - Fresh batter, no safety

### Ball Setup  
- `place_ball_at_location(ctx, location)` - Ball on field
- `give_ball_to_fielder(ctx, fielderIdx)` - Ball in hands

### Actions
- `trigger_player_run_to_next_base(ctx, playerIdx, fromBase)` - Initiate running

### Simulation
- `simulate_frames(ctx, count)` - Run N frames
- `simulate_until(ctx, condition, maxFrames)` - Run until success/timeout

## Special Cases to Remember

### Third Base → Home (Flag Point)

Running from third to home requires the `passedPathPoint` flag:

```c
// 0 = heading to flag point
// 1 = heading to home (already past flag)  
// 2 = completed/scored
ctx->state->localGameInfo->playerRuntime[playerIdx].passedPathPoint = 1;
```

This is specific to third→home because the path goes around a flag.

### State Synchronization

Setting up a running player requires coordinating multiple subsystems:

```c
// Physical
playerInfo[i].tPI.location = ...
playerInfo[i].cPI.moving = 1

// Logical
playerInfo[i].bTPI.baseId = BASE_THIRD
playerInfo[i].bTPI.state = PLAYER_STATE_RUNNING

// Referee (authority)
referee.battingPlayers[i].currentSafetyBase = BASE_THIRD
referee.battingPlayers[i].baseAtPitchStart = BASE_THIRD

// Runtime
playerRuntime[i].goingForward = 1
```

Use helpers like `place_runner_at_base()` to handle this automatically.

## Writing New Tests

### Step 1: Identify the Scenario

What real gameplay situation are you testing?
- "Runner on third scores when ball is in outfield"
- "Batter forced out when ball beats them to first"
- "Runner wounded on fly ball catch"

### Step 2: Minimal Setup

Use helpers to create the **starting condition**, not the end state:

```c
// GOOD: Setup starting state
place_runner_at_base(ctx, 0, BASE_THIRD, 0.0f);
trigger_player_run_to_next_base(ctx, 0, BASE_THIRD);

// BAD: Manually force end state
ctx->state->localGameInfo->playerInfo[0].bTPI.baseId = BASE_HOME_SCORED;
```

### Step 3: Run the Game

Let natural game progression handle the scenario:

```c
simulate_frames(ctx, 450);  // Or use simulate_until()
```

### Step 4: Verify Outcome

Check the **logical result**, not intermediate flags:

```c
// GOOD: Check what matters
ASSERT_EQ(1, ctx->state->localGameInfo->gameState.runsInTheInning);

// BAD: Check implementation details  
ASSERT_EQ(1, ctx->state->localGameInfo->playerRuntime[0].arrivedToBase);
```

## Debugging Test Failures

### Player Not Moving?

Check:
1. Did you call `trigger_player_run_to_next_base()`?
2. Is `cPI.moving = 1`?
3. Is `targetLocation` set?
4. For third→home: Is `passedPathPoint` correct?

### Referee Not Detecting?

Check:
1. Is `checkForRun = 1` (for runs)?
2. Is ball state correct (hasHitGround, etc.)?
3. Is `woundingCatchTimer = -1` (not blocking)?
4. Did referee state get initialized properly?

### Not Enough Time?

Movement takes time! Try:
- Increasing frame count (450+ for full third→home)
- Using `simulate_until()` with a condition
- Adding progress logging to see where player stops

## Migration from Old Tests

Old "snapshot" tests should be rewritten, not just fixed:

1. **Understand the intent:** What scenario was being tested?
2. **Identify starting state:** Where does the scenario actually begin?
3. **Use helpers:** Don't manually set flags
4. **Run naturally:** Let game loop handle progression
5. **Verify outcome:** Check results, not intermediate state

See `TESTING_INSIGHTS.md` for architectural lessons learned.
