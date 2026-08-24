#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include <stdio.h>
#include "execute_actions.h" // intent_push

/**
 * TEST: Free Walk Resolution
 *
 * Verifies that accepting a free walk:
 * 1. Triggers player movement (Action)
 * 2. Grants safety at the next base (Referee)
 * 3. Player physically runs and arrives at the next base
 */
int test_free_walk_resolution(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* game = ctx->state->match;
    GameRulesState* rules = ctx->state->rules;

    // 1. Setup: Runner at 1st base (physical state)
    int runnerIdx = 0;
    place_runner_at_base(ctx, runnerIdx, BASE_FIRST, 0.0f);

    // 2. Initialize referee from physical state
    initialize_referee_from_physical_state(ctx);

    printf(
        "[TEST] Initial state: Player %d at base %d, safety=%d\n", runnerIdx, game->playerInfo[runnerIdx].bTPI.baseId,
        rules->referee.battingPlayers[runnerIdx].currentSafetyBase
    );

    // 3. Setup: Free Walk Offer (artificial - normally from game_consolidation.c)
    game->flowControl.freeWalkIndex = runnerIdx;
    game->flowControl.freeWalkBase = BASE_FIRST;
    game->flowControl.waitingForFreeWalkDecision = 1;

    printf("[TEST] Offering Free Walk to Player %d from Base %d\n", runnerIdx, BASE_FIRST);

    // 3b. Wait a frame (simulate UI showing the offer before player responds)
    simulate_frames(ctx, 1);

    // 4. Action: Accept Free Walk — declared as a message on the batting team's channel, the same way
    // a human's key or the AI's decision reaches the engine.
    intent_push(
        &ctx->state->channels.batting, (IntentMessage){.kind = INTENT_TAKE_FREE_WALK, .as.free_walk = {.accept = 1}}
    );

    // 5. Process acceptance (1 frame to trigger event and start movement)
    simulate_frames(ctx, 1);

    printf(
        "[TEST] After acceptance: State=%d (Expected %d), SafetyBase=%d (Expected %d)\n",
        game->playerInfo[runnerIdx].bTPI.state, PLAYER_STATE_ADVANCING_FREELY,
        rules->referee.battingPlayers[runnerIdx].currentSafetyBase, BASE_SECOND
    );

    // CHECK 1: Action system started movement
    ASSERT_EQ(
        PLAYER_STATE_ADVANCING_FREELY, game->playerInfo[runnerIdx].bTPI.state, "Player state should be ADVANCING_FREELY"
    );

    // CHECK 2: Referee granted safety at NEXT base (2nd) immediately
    BaseID safetyBase = rules->referee.battingPlayers[runnerIdx].currentSafetyBase;
    ASSERT_EQ(BASE_SECOND, safetyBase, "Referee should grant safety at SECOND base immediately upon event");

    // CHECK 3: Base at pitch start updated (for future foul play resolution)
    BaseID startBase = rules->referee.battingPlayers[runnerIdx].baseAtPitchStart;
    ASSERT_EQ(BASE_SECOND, startBase, "BaseAtPitchStart should be updated to SECOND base");

    // 6. Wait for runner to physically arrive at second base
    printf("[TEST] Waiting for runner to arrive at 2nd base...\n");
    int arrivedAt2nd = 0;
    for (int frame = 0; frame < 500; frame++) {
        simulate_frames(ctx, 1);
        if (game->playerInfo[runnerIdx].bTPI.baseId == BASE_SECOND) {
            arrivedAt2nd = 1;
            printf("[TEST] Runner arrived at 2nd base (frame %d)\n", frame);
            break;
        }
    }

    // CHECK 4: Runner physically arrived at destination
    ASSERT_EQ(1, arrivedAt2nd, "Runner should physically arrive at 2nd base");
    ASSERT_EQ(BASE_SECOND, game->playerInfo[runnerIdx].bTPI.baseId, "Runner baseId should be SECOND");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
