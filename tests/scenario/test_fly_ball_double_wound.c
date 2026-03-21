#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include <stdio.h>
#include <math.h>

/**
 * TEST 5: Tuplahaava (Double Wound) - Sequence and Safety Verification WITH CATCH
 */
int test_fly_ball_double_wound(void)
{
    ScenarioContext* ctx = create_scenario();

    // Runner B (Lead) at 2nd base
    place_runner_at_base(ctx, 1, BASE_SECOND, 0.0f);
    // Runner A (Rear) at 60% to 2nd - slightly closer so they arrive a bit earlier after catch
    place_runner_at_base(ctx, 0, BASE_FIRST, 0.6f);
    setup_batter_at_home(ctx, 2);

    move_pitcher_away(ctx);

    int fielderIdx = 16;
    Vector3D fielderLoc = ctx->state->fieldPositions->backLeftPoint;
    fielderLoc.x += 5.0f;
    fielderLoc.z += 5.0f;
    ctx->state->match->playerInfo[fielderIdx].tPI.location = fielderLoc;
    ctx->state->match->playerInfo[fielderIdx].tPI.homeLocation = fielderLoc;

    // Initialize referee state from physical setup
    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);

    trigger_player_run_to_next_base(ctx, 0, BASE_FIRST);
    hit_fly_ball_to_location(ctx, ctx->state->fieldPositions->pitchPlate, fielderLoc);

    printf("  TUPLAHAAVA TRACE START\n");

    int catchFrame = -1, arrivalFrame = -1, woundFrame = -1, woundFrameB = -1;
    int leadRanAway = 0;
    Vector3D startB = ctx->state->match->playerInfo[1].tPI.location;

    for (int frame = 0; frame <= 400; frame++) {
        simulate_frames(ctx, 1);

        int hasBall = ctx->state->match->pII.hasBallIndex;
        int stateA = ctx->state->match->playerInfo[0].bTPI.state;
        int stateB = ctx->state->match->playerInfo[1].bTPI.state;
        int baseA = ctx->state->match->playerInfo[0].bTPI.baseId;
        int baseB = ctx->state->match->playerInfo[1].bTPI.baseId;
        int safeA = ctx->state->match->referee.battingPlayers[0].currentSafetyBase;
        int safeB = ctx->state->match->referee.battingPlayers[1].currentSafetyBase;
        Vector3D posB = ctx->state->match->playerInfo[1].tPI.location;
        int statusA = ctx->state->match->referee.battingPlayers[0].status;
        int statusB = ctx->state->match->referee.battingPlayers[1].status;
        int goingFwdA = ctx->state->match->playerRuntime[0].goingForward;
        int timer = ctx->state->match->referee.woundingEvaluationTimer;

        // Verbose logging for transition frames
        if (frame % 10 == 0 || (hasBall != -1 && catchFrame == -1) || baseA == BASE_SECOND ||
            stateA == PLAYER_STATE_WOUNDED || stateB == PLAYER_STATE_WOUNDED || arrivalFrame != -1 ||
            (arrivalFrame != -1 && frame < arrivalFrame + 50)) {
            printf(
                "  F%3d: A[St=%d Bs=%d Sf=%d Status=%d Fwd=%d] B[St=%d Bs=%d Sf=%d Status=%d] Ball=%d Tmr=%d\n", frame,
                stateA, baseA, safeA, statusA, goingFwdA, stateB, baseB, safeB, statusB, hasBall, timer
            );
        }

        if (hasBall != -1 && catchFrame == -1) {
            catchFrame = frame;
            printf("  !!! CATCH at frame %d\n", frame);
        }
        if (baseA == BASE_SECOND && arrivalFrame == -1) {
            arrivalFrame = frame;
            printf("  !!! ARRIVAL of A at 2nd at frame %d\n", frame);
        }
        if (stateA == PLAYER_STATE_WOUNDED && woundFrame == -1) {
            woundFrame = frame;
            printf("  !!! WOUND of A at frame %d\n", frame);
        }
        if (stateB == PLAYER_STATE_WOUNDED && woundFrameB == -1) {
            woundFrameB = frame;
            printf("  !!! WOUND of B at frame %d\n", frame);
        }

        float dist = sqrtf(powf(posB.x - startB.x, 2) + powf(posB.z - startB.z, 2));
        if (dist > 2.0f && stateB == PLAYER_STATE_RUNNING) leadRanAway = 1;

        if (stateA == PLAYER_STATE_WOUNDED && stateB == PLAYER_STATE_WOUNDED) break;
    }

    printf(
        "  TUPLAHAAVA TRACE END: Catch=%d, Arrival=%d, WoundA=%d, WoundB=%d, LeadRan=%d\n", catchFrame, arrivalFrame,
        woundFrame, woundFrameB, leadRanAway
    );

    cleanup_scenario(ctx);

    ASSERT_EQ(1, (catchFrame != -1), "Ball should be caught");
    ASSERT_EQ(1, (arrivalFrame != -1 || woundFrame != -1), "Runner A should arrive at 2nd");
    ASSERT_EQ(0, leadRanAway, "Lead runner B should NOT have run away");
    ASSERT_EQ(1, (woundFrame != -1), "Wound should be applied to A");
    ASSERT_EQ(1, (woundFrameB != -1), "Wound should be applied to B");

    return TEST_PASSED;
}
