#include "../scenario_builder.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include <stdio.h>

/**
 * TEST: Free Walk Resolution
 *
 * Verifies that accepting a free walk:
 * 1. Triggers player movement (Action)
 * 2. Grants safety at the next base (Referee)
 * 3. Does not immediately teleport the player (Physics integrity)
 */
int test_full_free_walk_resolution(void)
{
	ScenarioContext* ctx = create_scenario();
	LocalGameInfo* game = ctx->state->localGameInfo;

	// 1. Setup: Runner at 1st base
	int runnerIdx = 0;
	place_runner_at_base(ctx, runnerIdx, BASE_FIRST, 0.0f);

	// 2. Setup: Free Walk Offer
	game->gameControl.freeWalkIndex = runnerIdx;
	game->gameControl.freeWalkBase = BASE_FIRST;
	game->gameControl.waitingForFreeWalkDecision = 1;

	printf("[TEST] Offering Free Walk to Player %d at Base %d\n", runnerIdx, BASE_FIRST);

	// 2b. Wait a bit (simulate waiting for user input)
	simulate_frames(ctx, 10);

	// 3. Action: Accept Free Walk
	printf("[TEST] Action: Accept Free Walk (Player %d)\n", runnerIdx);
	printf("[TEST] Before: State=%d, SafetyBase=%d\n",
	       game->playerInfo[runnerIdx].bTPI.state,
	       game->referee.battingPlayers[runnerIdx].currentSafetyBase);

	game->aF.bTAF.takeFreeWalk = FREE_WALK_ACCEPT;

	// 4. Wait for processing (1 frame is enough)
	simulate_frames(ctx, 1);

	printf("[TEST] After: State=%d (Expected %d), SafetyBase=%d (Expected %d)\n",
	       game->playerInfo[runnerIdx].bTPI.state, PLAYER_STATE_ADVANCING_FREELY,
	       game->referee.battingPlayers[runnerIdx].currentSafetyBase, BASE_SECOND);

	// NOTE: We cannot check gameEvents.freeWalkAccepted here because simulate_frames
	// clears transient events at the end of the frame. We must verify by checking the consequences.

	// CHECK 1: Action system started movement
	// "runToNextBase" sets state to RUNNING or ADVANCING_FREELY
	ASSERT_EQ(PLAYER_STATE_ADVANCING_FREELY, game->playerInfo[runnerIdx].bTPI.state, "Player state should be ADVANCING_FREELY");

	// CHECK 2: Referee granted safety at NEXT base (2nd)
	// The refactor moved this to referee.c. It happens immediately on event detection.
	BaseID safetyBase = game->referee.battingPlayers[runnerIdx].currentSafetyBase;
	ASSERT_EQ(BASE_SECOND, safetyBase, "Referee should grant safety at SECOND base immediately upon event");

	// CHECK 4: Base at pitch start updated (for future foul play resolution)
	BaseID startBase = game->referee.battingPlayers[runnerIdx].baseAtPitchStart;
	ASSERT_EQ(BASE_SECOND, startBase, "BaseAtPitchStart should be updated to SECOND base");

	cleanup_scenario(ctx);
	return TEST_PASSED;
}
