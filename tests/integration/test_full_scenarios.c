#include "test_full_scenarios.h"
#include "test_helpers.h"
#include "scenario_builder.h"
#include <stdio.h>
#include <math.h>

/**
 * TEST 1: Runner scores from third base to home
 */
static int test_full_runner_scores_from_third(void)
{
	ScenarioContext* ctx = create_scenario();

	place_runner_at_base(ctx, 0, BASE_THIRD, 0.0f);
	ctx->state->localGameInfo->playerRuntime[0].passedPathPoint = 1;
	place_ball_at_location(ctx, (Vector3D){10.0f, 0.0f, -10.0f});
	ctx->state->localGameInfo->gameControl.checkForRun = 1;
	trigger_player_run_to_next_base(ctx, 0, BASE_THIRD);

	simulate_frames(ctx, 450);

	int runs = ctx->state->localGameInfo->gameState.runsInTheInning;
	cleanup_scenario(ctx);
	ASSERT_EQ(1, runs, "Runner should have scored from third base");
	return TEST_PASSED;
}

/**
 * TEST 2: Batter forced out at first base
 */
static int test_full_batter_forced_out_at_first(void)
{
	ScenarioContext* ctx = create_scenario();
	setup_batter_at_home(ctx, 0);

	Vector3D firstBase = ctx->state->fieldPositions->firstBase;
	ctx->state->localGameInfo->playerInfo[13].tPI.location = firstBase;
	ctx->state->localGameInfo->playerInfo[13].tPI.homeLocation = firstBase;

	Vector3D throwFrom = {5.0f, 1.5f, -10.0f};
	throw_ball_to_base(ctx, throwFrom, BASE_FIRST);

	ctx->state->localGameInfo->pRAI.batHit = 1;
	ctx->state->localGameInfo->pRAI.batterCanAdvance = 1;
	trigger_player_run_to_next_base(ctx, 0, BASE_HOME);

	int outs = 0;
	for (int frame = 0; frame <= 300; frame += 50) {
		if (frame > 0) simulate_frames(ctx, 50);
		outs = ctx->state->localGameInfo->gameState.outs;
		if (outs > 0) break;
	}

	cleanup_scenario(ctx);
	ASSERT_EQ(1, outs, "Batter should be forced out at first base");
	return TEST_PASSED;
}

/**
 * TEST 3: Runner wounded on fly ball
 */
static int test_full_fly_ball_runner_wounded(void)
{
	ScenarioContext* ctx = create_scenario();
	place_runner_at_base(ctx, 0, BASE_FIRST, 0.0f);
	setup_batter_at_home(ctx, 1);

	Vector3D away = {100.0f, 0.0f, 100.0f};
	ctx->state->localGameInfo->playerInfo[12].tPI.location = away;
	ctx->state->localGameInfo->playerInfo[13].tPI.location = away;

	int fielderIdx = 15;
	Vector3D fielderLoc = ctx->state->fieldPositions->thirdBase;
	ctx->state->localGameInfo->playerInfo[fielderIdx].tPI.location = fielderLoc;
	ctx->state->localGameInfo->playerInfo[fielderIdx].tPI.homeLocation = fielderLoc;

	trigger_player_run_to_next_base(ctx, 0, BASE_FIRST);
	hit_fly_ball_to_location(ctx, ctx->state->fieldPositions->pitchPlate, fielderLoc);

	int woundDetected = 0;
	for (int frame = 0; frame <= 800; frame += 50) {
		if (frame > 0) simulate_frames(ctx, 50);
		if (ctx->state->localGameInfo->playerInfo[0].bTPI.state == PLAYER_STATE_WOUNDED || 
		    ctx->state->localGameInfo->playerInfo[0].bTPI.baseId == BASE_NONE) {
			woundDetected = 1;
			break;
		}
	}

	cleanup_scenario(ctx);
	ASSERT_EQ(1, woundDetected, "Runner should be wounded");
	return TEST_PASSED;
}

/**
 * TEST 4: Runner Chain Reaction - No Catch
 * Runner A runs to base 2 where Runner B is standing.
 * Expected: A gets safety, B loses safety and starts running.
 */
static int test_full_runner_chain_reaction_no_catch(void)
{
	ScenarioContext* ctx = create_scenario();

	// Runner B (Lead) at 2nd base
	place_runner_at_base(ctx, 1, BASE_SECOND, 0.0f);
	// Runner A (Rear) at 60% to 2nd
	place_runner_at_base(ctx, 0, BASE_FIRST, 0.6f);
	setup_batter_at_home(ctx, 2);

	// Move fielders away
	Vector3D away = {100.0f, 0.0f, 100.0f};
	ctx->state->localGameInfo->playerInfo[12].tPI.location = away;
	ctx->state->localGameInfo->playerInfo[13].tPI.location = away;
	ctx->state->localGameInfo->playerInfo[16].tPI.location = away;

	trigger_player_run_to_next_base(ctx, 0, BASE_FIRST);
	
	// Hit ball to ground (no catch) - far away
	ctx->state->localGameInfo->ballInfo.location = ctx->state->fieldPositions->pitchPlate;
	ctx->state->localGameInfo->ballInfo.velocity.x = -0.5f;
	ctx->state->localGameInfo->ballInfo.velocity.y = 0.3f;
	ctx->state->localGameInfo->ballInfo.velocity.z = -0.5f;
	ctx->state->localGameInfo->ballInfo.moving = 1;
	ctx->state->localGameInfo->ballInfo.onGround = 0;
	ctx->state->localGameInfo->pRAI.batHit = 1;

	printf("  Chain reaction test (no catch)\n");

	int arrivalFrame = -1;
	int leadStartedRunning = -1;
	Vector3D startB = ctx->state->localGameInfo->playerInfo[1].tPI.location;

	for (int frame = 0; frame <= 400; frame++) {
		simulate_frames(ctx, 1);

		int stateB = ctx->state->localGameInfo->playerInfo[1].bTPI.state;
		int baseA = ctx->state->localGameInfo->playerInfo[0].bTPI.baseId;
		Vector3D posB = ctx->state->localGameInfo->playerInfo[1].tPI.location;

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

/**
 * TEST 5: Tuplahaava (Double Wound) - Sequence and Safety Verification WITH CATCH
 */
static int test_full_tuplahaava_double_wound(void)
{
	ScenarioContext* ctx = create_scenario();

	// Runner B (Lead) at 2nd base
	place_runner_at_base(ctx, 1, BASE_SECOND, 0.0f);
	// Runner A (Rear) at 60% to 2nd - slightly closer so they arrive a bit earlier after catch
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

	trigger_player_run_to_next_base(ctx, 0, BASE_FIRST);
	hit_fly_ball_to_location(ctx, ctx->state->fieldPositions->pitchPlate, fielderLoc);

	printf("  TUPLAHAAVA TRACE START\n");

	int catchFrame = -1, arrivalFrame = -1, woundFrame = -1, woundFrameB = -1;
	int leadRanAway = 0;
	Vector3D startB = ctx->state->localGameInfo->playerInfo[1].tPI.location;

	for (int frame = 0; frame <= 400; frame++) {
		simulate_frames(ctx, 1);

		int hasBall = ctx->state->localGameInfo->pII.hasBallIndex;
		int stateA = ctx->state->localGameInfo->playerInfo[0].bTPI.state;
		int stateB = ctx->state->localGameInfo->playerInfo[1].bTPI.state;
		int baseA = ctx->state->localGameInfo->playerInfo[0].bTPI.baseId;
		int baseB = ctx->state->localGameInfo->playerInfo[1].bTPI.baseId;
		int safeA = ctx->state->localGameInfo->referee.battingPlayers[0].currentSafetyBase;
		int safeB = ctx->state->localGameInfo->referee.battingPlayers[1].currentSafetyBase;
		Vector3D posB = ctx->state->localGameInfo->playerInfo[1].tPI.location;
		int pendingWoundA = ctx->state->localGameInfo->referee.battingPlayers[0].hasPendingWound;
		int pendingWoundB = ctx->state->localGameInfo->referee.battingPlayers[1].hasPendingWound;
		int woundTypeA = ctx->state->localGameInfo->referee.battingPlayers[0].woundingType;
		int woundTypeB = ctx->state->localGameInfo->referee.battingPlayers[1].woundingType;
		int markedA = ctx->state->localGameInfo->referee.woundingPlayersMarked[0];
		int markedB = ctx->state->localGameInfo->referee.woundingPlayersMarked[1];
		int goingFwdA = ctx->state->localGameInfo->playerRuntime[0].goingForward;
		int timer = ctx->state->localGameInfo->referee.woundingCatchTimer;

		// Verbose logging for transition frames
		if (frame % 10 == 0 || (hasBall != -1 && catchFrame == -1) || baseA == BASE_SECOND || stateA == PLAYER_STATE_WOUNDED || stateB == PLAYER_STATE_WOUNDED || arrivalFrame != -1 || (arrivalFrame != -1 && frame < arrivalFrame + 50)) {
			printf("  F%3d: A[St=%d Bs=%d Sf=%d PW=%d WT=%d Mk=%d Fwd=%d] B[St=%d Bs=%d Sf=%d PW=%d WT=%d Mk=%d] Ball=%d Tmr=%d\n", 
			       frame, stateA, baseA, safeA, pendingWoundA, woundTypeA, markedA, goingFwdA, 
			       stateB, baseB, safeB, pendingWoundB, woundTypeB, markedB, hasBall, timer);
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

	printf("  TUPLAHAAVA TRACE END: Catch=%d, Arrival=%d, WoundA=%d, WoundB=%d, LeadRan=%d\n", 
	       catchFrame, arrivalFrame, woundFrame, woundFrameB, leadRanAway);

	cleanup_scenario(ctx);

	ASSERT_EQ(1, (catchFrame != -1), "Ball should be caught");
	ASSERT_EQ(1, (arrivalFrame != -1 || woundFrame != -1), "Runner A should arrive at 2nd");
	ASSERT_EQ(0, leadRanAway, "Lead runner B should NOT have run away");
	ASSERT_EQ(1, (woundFrame != -1), "Wound should be applied to A");
	ASSERT_EQ(1, (woundFrameB != -1), "Wound should be applied to B");
	
	return TEST_PASSED;
}

/**
 * TEST 6: Tuplahaava - Late Arrival + Lead Runner Shared Safety
 * Scenario:
 * 1. Runner B starts at Base 2.
 * 2. Runner A starts at 60% towards Base 2.
 * 3. Fly ball hit and caught.
 * 4. B runs slightly towards Base 3 (30%) after catch.
 * 5. Timer expires. A becomes Pending Wound. B is safe (was at base at catch).
 * 6. A arrives at Base 2. Since A is Pending Wound, B (occupant) becomes Pending Wound (Tuplahaava).
 * 7. A is wounded upon arrival.
 * 8. B returns to Base 2 and should be wounded upon arrival.
 */
static int test_full_tuplahaava_late_arrival_between_bases(void)
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

void run_full_scenario_tests(void)
{
	printf("Running Full-Scenario Integration Tests...\n");
	RUN_TEST(test_full_runner_scores_from_third);
	RUN_TEST(test_full_batter_forced_out_at_first);
	RUN_TEST(test_full_fly_ball_runner_wounded);
	RUN_TEST(test_full_runner_chain_reaction_no_catch);
	RUN_TEST(test_full_tuplahaava_double_wound);
	RUN_TEST(test_full_tuplahaava_late_arrival_between_bases);
}
