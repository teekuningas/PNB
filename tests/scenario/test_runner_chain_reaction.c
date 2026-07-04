#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include <stdio.h>
#include <math.h>

/**
 * TEST 4: Runner Chain Reaction - No Catch
 * Runner A runs to base 2 where Runner B is standing.
 * Expected: A gets safety, B loses safety and starts running.
 */
int test_runner_chain_reaction_no_catch(void)
{
    ScenarioContext* ctx = create_scenario();

    // Runner B (Lead) at 2nd base
    place_runner_at_base(ctx, 1, BASE_SECOND, 0.0f);
    // Runner A (Rear) at 60% to 2nd
    place_runner_at_base(ctx, 0, BASE_FIRST, 0.6f);
    setup_batter_at_home(ctx, 2);

    // Move fielders away so they don't interfere
    move_pitcher_away(ctx);
    Vector3D away = {100.0f, 0.0f, 100.0f};
    ctx->state->match->playerInfo[16].tPI.location = away;

    // Initialize referee state from physical setup
    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);

    trigger_player_run_to_next_base(ctx, 0, BASE_FIRST);

    // Drop ball in outfield (will hit ground, no catch)
    Vector3D ballTarget = {30.0f, 0.0f, 40.0f};
    place_ball_over_location(ctx, ballTarget);

    printf("  Chain reaction test (no catch)\n");

    int arrivalFrame = -1;
    int leadStartedRunning = -1;
    Vector3D startB = ctx->state->match->playerInfo[1].tPI.location;

    for (int frame = 0; frame <= 400; frame++) {
        simulate_frames(ctx, 1);

        int stateB = ctx->state->match->playerInfo[1].bTPI.state;
        int baseA = ctx->state->match->playerInfo[0].bTPI.baseId;
        Vector3D posB = ctx->state->match->playerInfo[1].tPI.location;

        if (baseA == BASE_SECOND && arrivalFrame == -1) {
            arrivalFrame = frame;
        }

        if (leadStartedRunning == -1 && stateB == PLAYER_STATE_RUNNING) {
            leadStartedRunning = frame;
        }

        // Stop when B has moved significantly or enough frames passed
        float dist = sqrtf(powf(posB.x - startB.x, 2) + powf(posB.z - startB.z, 2));
        if (dist > 5.0f || frame > 300) break;
    }

    cleanup_scenario(ctx);

    ASSERT_EQ(1, (arrivalFrame != -1), "Runner A should arrive at 2nd");
    ASSERT_EQ(1, (leadStartedRunning != -1), "Runner B should start running after A arrives");

    return TEST_PASSED;
}
