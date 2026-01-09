#include "../scenario_builder.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include <stdio.h>
#include <math.h>

/**
 * TEST 6: Tuplahaava - Late Arrival + Lead Runner Shared Safety
 */
int test_full_tuplahaava_late_arrival_between_bases(void)
{
	ScenarioContext* ctx = create_scenario();

	// Runner B (Lead) at 2nd base
	place_runner_at_base(ctx, 1, BASE_SECOND, 0.0f);
	// Runner A (Rear) at 60% to 2nd to ensure they arrive after catch
	place_runner_at_base(ctx, 0, BASE_FIRST, 0.6f);
	setup_batter_at_home(ctx, 2);

	Vector3D away = {100.0f, 0.0f, 100.0f};
	ctx->state->localGameInfo->playerInfo[12].tPI.location = away;
	ctx->state->localGameInfo->playerInfo[13].tPI.location = away;

	int fielderIdx = 16;
	Vector3D fielderLoc = ctx->state->fieldPositions->backLeftPoint;
	fielderLoc.x += 5.0f; fielderLoc.z += 5.0f;
	ctx->state->localGameInfo->playerInfo[fielderIdx].tPI.location = fielderLoc;
	ctx->state->localGameInfo->playerInfo[fielderIdx].tPI.homeLocation = fielderLoc;

	trigger_player_run_to_next_base(ctx, 0, BASE_FIRST); // A runs to 2nd
	
hit_fly_ball_to_location(ctx, ctx->state->fieldPositions->pitchPlate, fielderLoc);

	printf("  TUPLAHAAVA SHARED SAFETY TRACE START\n");

	int catchFrame = -1, timerExpiredFrame = -1, arrivalAFrame = -1, woundAFrame = -1, woundBFrame = -1;
	int bRanForward = 0, bReturnTriggered = 0;
	
	for (int frame = 0; frame <= 800; frame++) {
		simulate_frames(ctx, 1);

		int hasBall = ctx->state->localGameInfo->pII.hasBallIndex;
		int stateA = ctx->state->localGameInfo->playerInfo[0].bTPI.state;
		int stateB = ctx->state->localGameInfo->playerInfo[1].bTPI.state;
		int baseA = ctx->state->localGameInfo->playerInfo[0].bTPI.baseId;
		int timer = ctx->state->localGameInfo->referee.woundingCatchTimer;
		int baseB = ctx->state->localGameInfo->playerInfo[1].bTPI.baseId;
		int safeA = ctx->state->localGameInfo->referee.battingPlayers[0].currentSafetyBase;
		int safeB = ctx->state->localGameInfo->referee.battingPlayers[1].currentSafetyBase;
		int pendingWoundA = ctx->state->localGameInfo->referee.battingPlayers[0].hasPendingWound;
		int pendingWoundB = ctx->state->localGameInfo->referee.battingPlayers[1].hasPendingWound;
		int typeB = ctx->state->localGameInfo->referee.battingPlayers[1].woundingType;

		        // Verbose logging
				if (frame % 20 == 0 || stateA == PLAYER_STATE_WOUNDED || stateB == PLAYER_STATE_WOUNDED || arrivalAFrame == frame) {
					printf("  F%3d: A[St=%d Bs=%d Sf=%d PW=%d] B[St=%d Bs=%d Sf=%d PW=%d Ty=%d] Tmr=%d\n", 
					       frame, stateA, baseA, safeA, pendingWoundA,
					       stateB, baseB, safeB, pendingWoundB, typeB, timer);
				}
		
				if (hasBall != -1 && catchFrame == -1) {
					catchFrame = frame;
					printf("  !!! CATCH at frame %d\n", frame);
				}
		
				if (catchFrame != -1 && bRanForward == 0 && frame > catchFrame + 10) {
					trigger_player_run_to_next_base(ctx, 1, BASE_SECOND);
					bRanForward = 1;
					printf("  !!! B runs forward towards 3rd at frame %d\n", frame);
				}
				
				if (catchFrame != -1 && timer == -1 && timerExpiredFrame == -1 && frame > catchFrame + 10) {
					timerExpiredFrame = frame;
					printf("  !!! TIMER EXPIRED at frame %d\n", frame);
				}
		
				if (baseA == BASE_SECOND && arrivalAFrame == -1) {
					arrivalAFrame = frame;
					printf("  !!! ARRIVAL of A at 2nd at frame %d\n", frame);
				}
		
				if (stateA == PLAYER_STATE_WOUNDED && woundAFrame == -1) {
					woundAFrame = frame;
					printf("  !!! WOUND of A at frame %d\n", frame);
				}
		
				// Trigger B to return ONLY after A is confirmed wounded
				if (stateA == PLAYER_STATE_WOUNDED && bReturnTriggered == 0) {
					trigger_player_run_to_previous_base(ctx, 1, BASE_SECOND);
					bReturnTriggered = 1;
					printf("  !!! B starts running back to 2nd at frame %d\n", frame);
				}
		
				if (stateB == PLAYER_STATE_WOUNDED && woundBFrame == -1) {
					woundBFrame = frame;
					printf("  !!! WOUND of B at frame %d\n", frame);
				}
		
				if (stateA == PLAYER_STATE_WOUNDED && stateB == PLAYER_STATE_WOUNDED) break;
			}
		
			printf("  TUPLAHAAVA TRACE END: Catch=%d, TimerExp=%d, ArrA=%d, WoundA=%d, WoundB=%d\n", 
			       catchFrame, timerExpiredFrame, arrivalAFrame, woundAFrame, woundBFrame);

	cleanup_scenario(ctx);

	ASSERT_EQ(1, (catchFrame != -1), "Ball should be caught");
	ASSERT_EQ(1, (timerExpiredFrame != -1), "Timer should expire");
	ASSERT_EQ(1, (arrivalAFrame != -1), "A should arrive at 2nd");
	ASSERT_EQ(1, (woundAFrame != -1), "Wound should be applied to A");
	ASSERT_EQ(1, (woundBFrame != -1), "Wound should be applied to B upon return (Tuplahaava)");
	ASSERT_EQ(1, (woundBFrame > woundAFrame), "B should be wounded AFTER A");
	
	return TEST_PASSED;
}