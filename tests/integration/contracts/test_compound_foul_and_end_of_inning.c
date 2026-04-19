#include "scenario_builder.h"
#include "test_helpers.h"
#include "contracts/all_contracts.h"
#include "referee.h"

int test_compound_foul_and_end_of_inning(void)
{
    ScenarioContext* ctx = create_scenario();

    // Set 2 strikes and 2 outs
    ctx->state->match->halfInningState.strikes = 2;
    ctx->state->match->halfInningState.outs = 2;

    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx); // This calls update_referee with pitchReleased=1

    // Set up foul ball hit on this exact frame
    ctx->state->match->gameEvents.ballHitByBat = 1;
    ctx->state->match->gameEvents.ballHitGround = 1;
    ctx->state->match->betweenPitchState.hasBallHitGround = 0;
    ctx->state->match->betweenPitchState.batOutcome = BAT_OUTCOME_HIT;

    // Put ball out of bounds
    ctx->state->match->ballInfo.location.x = 200.0f;

    // Run exactly 1 pipeline frame
    simulate_frames(ctx, 1);

    // Assert: We bypassed FOUL_STATE_DETECTED entirely because the inning is ending
    ASSERT_EQ(FOUL_STATE_NONE, ctx->state->match->referee.foulState, "Should not enter foul state when inning ends");

    // Assert: We jumped directly to END_INNING_STATE_DETECTED
    ASSERT_EQ(
        END_INNING_STATE_DETECTED, ctx->state->match->referee.endOfInningState,
        "Did not jump to END_INNING_STATE_DETECTED"
    );

    // Assert: The foul consequence (Strike 3 -> Out 3) was applied instantly
    ASSERT_EQ(3, ctx->state->match->halfInningState.strikes, "Strikes not incremented to 3");
    ASSERT_EQ(3, ctx->state->match->halfInningState.outs, "Outs not incremented to 3");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
