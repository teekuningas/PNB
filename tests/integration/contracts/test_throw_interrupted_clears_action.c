#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"

/**
 * CONTRACT: When a throw is interrupted (throwGoingOn cleared externally),
 * currentCatchingAction auto-clears to NONE within one frame.
 *
 * This is the regression test for Bug #8 (stuck actionKeyLock).
 * The old system used a manual mutex (actionKeyLock) that could get
 * permanently stuck if the throw completion path was bypassed.
 * The new system uses currentCatchingAction which auto-clears when
 * throwGoingOn == 0, making stuck states impossible.
 *
 * Scenario:
 *   1. A fielder holds the ball
 *   2. Human player starts a throw (ACTION_TRIGGER_START)
 *   3. Throw enters loading phase (throwGoingOn = 1, currentCatchingAction = THROWING)
 *   4. External interrupt: throwGoingOn is cleared (simulates ball caught by another player)
 *   5. Next frame: currentCatchingAction auto-clears to NONE
 *   6. Player can now pitch/drop/change (not locked out)
 */
int test_interrupted_throw_clears_action_state(void)
{
    ScenarioContext* ctx = create_scenario();

    // Setup: fielder 15 holds the ball
    int fielderIdx = 15;
    ctx->state->match->pII.hasBallIndex = fielderIdx;
    ctx->state->match->pII.controlIndex = fielderIdx;
    ctx->state->match->playerInfo[fielderIdx].tPI.location.x = 5.0f;
    ctx->state->match->playerInfo[fielderIdx].tPI.location.z = 5.0f;

    // Place target base far enough away for throw to succeed
    // (throwDistance > THROW_TO_BASE_DISTANCE check in throw_load)

    // Simulate: human presses throw to base 1
    ctx->state->match->aF.cTAF.throwToBase[1] = ACTION_TRIGGER_START;

    // Run one frame — execute_actions should process the throw start
    simulate_frames(ctx, 1);

    // Verify throw started successfully
    ASSERT_EQ(
        CATCHING_ACTION_THROWING, ctx->state->match->pendingActionState.currentCatchingAction,
        "After throw start, currentCatchingAction should be THROWING"
    );
    ASSERT_EQ(1, ctx->state->match->pendingActionState.throwGoingOn, "throwGoingOn should be 1 during throw");

    // Simulate external interrupt: something clears throwGoingOn
    // (e.g., game_manipulation catches ball, or ball hits ground)
    ctx->state->match->pendingActionState.throwGoingOn = 0;

    // Run one more frame — auto-clear should fire
    simulate_frames(ctx, 1);

    // THE KEY ASSERTION: action state is cleared, player is not stuck
    ASSERT_EQ(
        CATCHING_ACTION_NONE, ctx->state->match->pendingActionState.currentCatchingAction,
        "After interrupt, currentCatchingAction should auto-clear to NONE (Bug #8 fix)"
    );

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
