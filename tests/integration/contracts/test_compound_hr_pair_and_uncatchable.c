#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "referee.h"

int test_compound_hr_pair_and_uncatchable(void)
{
    ScenarioContext* ctx = create_scenario();

    // Set up Homerun Contest (period 4)
    ctx->state->rules->scoreboard.period = 4;
    ctx->state->rules->scoreboard.pairCount = 5;
    ctx->state->rules->homeRunContestState.runnerBatterPairCounter = 0;

    // Set up uncatchable score: Inning 1 (bottom of 1st), Catching team (0) has 10, Batting team (1) has 0
    ctx->state->rules->scoreboard.inning = 1;
    ctx->state->rules->scoreboard.teams[0].runs = 10;
    ctx->state->rules->scoreboard.teams[1].runs = 0;

    // Setup physical players to trigger the pair end
    // Pair 0 is active (Runner 0, Batter 1)
    setup_batter_at_home(ctx, 1);
    place_runner_at_base(ctx, 0, BASE_THIRD, 0.0f);

    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);

    // Give ball to pitcher at home so ballHome=1 survives the pipeline frame.
    // (Setting ballHome directly gets overwritten by gameManipulation.)
    give_ball_to_pitcher(ctx);

    // Clear all batting players' safety so pair-end conditions trigger
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        ctx->state->rules->referee.battingPlayers[i].currentSafetyBase = BASE_NONE;
        ctx->state->match->playerInfo[i].bTPI.baseId = BASE_NONE;
    }

    // Run exactly 1 pipeline frame
    simulate_frames(ctx, 1);

    ASSERT_EQ(
        END_INNING_STATE_DETECTED, ctx->state->rules->referee.endOfInningState,
        "Did not skip to END_INNING_STATE_DETECTED"
    );
    ASSERT_EQ(HR_PAIR_STATE_NONE, ctx->state->rules->referee.nextPairTransitionState, "Pair state machine was active");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
