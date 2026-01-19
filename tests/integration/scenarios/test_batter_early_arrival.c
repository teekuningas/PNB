#include "../scenario_builder.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include "referee.h"
#include "common_logic.h"
#include "base_control.h"
#include <stdio.h>
#include <math.h>

/*
  Test: Batter arrives at 1st base BEFORE catch is made.

  Finnish Rules: A runner should be WOUNDED if they are not at their original base
  when the catch happens, even if they arrive at the next base before the catch.

  Scenario:
  1. Batter starts at Home.
  2. Batter hits a high fly ball.
  3. Batter runs to 1st and arrives there before the catch.
  4. Ball is caught (koppi) while batter is already at 1st base.
  5. Expected: Batter should be WOUNDED because they left their original base (Home)
     before the catch was made.

  This test checks if the wounding logic correctly handles the case where a runner
  arrives at the next base before the catch happens.
*/

int test_batter_arrives_before_catch(void)
{
	ScenarioContext* ctx = create_scenario();
	MatchSession* match = ctx->state->match;

	printf("\n=== BATTER EARLY ARRIVAL TEST ===\n");

	// 1. Setup: Only batter at Home, no other runners
	setup_batter_at_home(ctx, 0);
	match->pII.batterSelectionIndex = 0;

	// Ensure batter is tracked in the referee system
	match->referee.battingPlayers[0].baseAtPitchStart = BASE_HOME;
	match->referee.battingPlayers[0].currentSafetyBase = BASE_HOME;

	// 2. Move pitcher and catcher away
	Vector3D away = {100.0f, 0.0f, 100.0f};
	match->playerInfo[12].tPI.location = away;
	match->playerInfo[13].tPI.location = away;

	// 3. Position fielder at third base area
	int fielderIdx = 16; // Center fielder
	Vector3D fielderLoc = ctx->state->fieldPositions->thirdBase;
	match->playerInfo[fielderIdx].tPI.location = fielderLoc;
	match->playerInfo[fielderIdx].tPI.homeLocation = fielderLoc;

	// 4. Trigger batter to run
	match->pRAI.batterCanAdvance = 1;
	trigger_player_run_to_next_base(ctx, 0, BASE_HOME);    // Batter: Home → 1st

	// 5. Hit a VERY HIGH fly ball to third base (long flight time = 300 frames)
	// This gives the batter time to reach 1st before the catch
	Vector3D pitchLocation = ctx->state->fieldPositions->pitchPlate;
	hit_fly_ball_to_location_with_time(ctx, pitchLocation, fielderLoc, 300.0f);

	printf("Setup: Batter at Home, Very high fly ball to third base at (%.1f, %.1f, %.1f)\n",
	       fielderLoc.x, fielderLoc.y, fielderLoc.z);
	printf("  Flight time: 300 frames (~6 seconds)\n");

	printf("Initial state:\n");
	printf("  Batter: baseId=%d, state=%d, position=(%.1f, %.1f)\n",
	       match->playerInfo[0].bTPI.baseId,
	       match->playerInfo[0].bTPI.state,
	       match->playerInfo[0].tPI.location.x,
	       match->playerInfo[0].tPI.location.z);
	printf("  Referee: baseAtPitchStart=%d, currentSafetyBase=%d\n",
	       match->referee.battingPlayers[0].baseAtPitchStart,
	       match->referee.battingPlayers[0].currentSafetyBase);
	printf("  Ball: moving=%d, location=(%.1f, %.1f, %.1f)\n",
	       match->ballInfo.moving,
	       match->ballInfo.location.x,
	       match->ballInfo.location.y,
	       match->ballInfo.location.z);
	printf("  PRAI: batterCanAdvance=%d, batterReady=%d, batHit=%d\n",
	       match->pRAI.batterCanAdvance,
	       match->pRAI.batterReady,
	       match->pRAI.batHit);

	// 6. Simulate and track events
	int catchFrame = -1;
	int batterArrivalFrame = -1;
	int woundFrame = -1;
	int batterState = -1;
	int batterSafety = -1;

	for (int frame = 0; frame <= 800; frame++) {
		simulate_frames(ctx, 1);

		int hasBall = match->pII.hasBallIndex;
		int batterBase = match->playerInfo[0].bTPI.baseId;
		batterState = match->playerInfo[0].bTPI.state;
		batterSafety = match->referee.battingPlayers[0].currentSafetyBase;
		int batterBaseAtPitchStart = match->referee.battingPlayers[0].baseAtPitchStart;
		int pendingWound = match->referee.battingPlayers[0].pendingWoundState;

		// Track catch
		if (hasBall != -1 && catchFrame == -1) {
			catchFrame = frame;
			printf("Frame %d: CATCH made\n", frame);
			printf("  Batter: baseId=%d, state=%d, safety=%d, baseAtPitchStart=%d, pendingWound=%d\n",
			       batterBase, batterState, batterSafety, batterBaseAtPitchStart, pendingWound);
			printf("  Position: (%.1f, %.1f)\n",
			       match->playerInfo[0].tPI.location.x,
			       match->playerInfo[0].tPI.location.z);
		}

		// Track batter arrival at 1st
		if (batterBase == BASE_FIRST && batterArrivalFrame == -1) {
			batterArrivalFrame = frame;
			printf("Frame %d: Batter arrived at 1st\n", frame);
			printf("  State=%d, safety=%d, baseAtPitchStart=%d, pendingWound=%d\n",
			       batterState, batterSafety, batterBaseAtPitchStart, pendingWound);
			printf("  Position: (%.1f, %.1f)\n",
			       match->playerInfo[0].tPI.location.x,
			       match->playerInfo[0].tPI.location.z);
		}

		// Track wounding
		if (batterState == PLAYER_STATE_WOUNDED && woundFrame == -1) {
			woundFrame = frame;
			printf("Frame %d: Batter WOUNDED\n", frame);
			printf("  BaseId=%d, safety=%d, pendingWound=%d\n",
			       batterBase, batterSafety, pendingWound);
		}

		// Track when safety changes
		static int lastSafety = -2;
		if (batterSafety != lastSafety && frame > 0) {
			printf("Frame %d: Safety changed from %d to %d (baseId=%d, state=%d)\n",
			       frame, lastSafety, batterSafety, batterBase, batterState);
			lastSafety = batterSafety;
		}
		if (frame == 0) lastSafety = batterSafety;

		// Debug logging every 50 frames or on key events
		if (frame % 50 == 0 || frame == catchFrame || frame == batterArrivalFrame || frame == woundFrame ||
		        (batterArrivalFrame != -1 && frame >= batterArrivalFrame - 5 && frame <= batterArrivalFrame + 10)) {
			int ballAtBase = get_ball_at_base_index(ctx->state);
			printf("  F%3d: Batter[St=%d Bs=%d Sf=%d BsStart=%d PW=%d] Ball[At=%d Player=%d] Pos=(%.1f,%.1f)\n",
			       frame, batterState, batterBase, batterSafety, batterBaseAtPitchStart,
			       pendingWound, ballAtBase, hasBall,
			       match->playerInfo[0].tPI.location.x,
			       match->playerInfo[0].tPI.location.z);
		}

		// Stop when wounded or after reasonable time
		if (woundFrame != -1 || frame > 700) break;
	}

	printf("\n=== EVENT SUMMARY ===\n");
	printf("Batter arrival at 1st: %d\n", batterArrivalFrame);
	printf("Catch: %d\n", catchFrame);
	printf("Batter WOUNDED: %d\n", woundFrame);
	printf("\n=== FINAL STATE ===\n");
	printf("Batter: state=%d, baseId=%d, safety=%d\n",
	       batterState,
	       match->playerInfo[0].bTPI.baseId,
	       batterSafety);
	printf("=====================\n\n");

	cleanup_scenario(ctx);

	// Assertions
	ASSERT_EQ(1, (catchFrame != -1), "Ball should be caught");
	ASSERT_EQ(1, (batterArrivalFrame != -1), "Batter should arrive at 1st");
	ASSERT_EQ(1, (batterArrivalFrame < catchFrame), "Batter should arrive BEFORE catch");
	ASSERT_EQ(1, (woundFrame != -1), "Batter should be WOUNDED (left original base before catch)");

	return TEST_PASSED;
}
