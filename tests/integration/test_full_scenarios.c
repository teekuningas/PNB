#include "test_full_scenarios.h"
#include "test_helpers.h"
#include "scenario_builder.h"
#include <stdio.h>

/**
 * TEST 1: Runner scores from third base to home
 * 
 * Scenario:
 * - Runner has safety at third base
 * - Runner is triggered to run home (via game's runToNextBase machinery)
 * - Ball is on field (not blocking scoring)
 * - Game loop naturally moves player and detects arrival
 * 
 * Learning: In Pesäpallo, running from third to home requires going around
 * a "flag point" (runLeftPoint). The passedPathPoint mechanism tracks:
 *   0 = heading to flag
 *   1 = heading to home (past flag)
 *   2 = completed/scored
 * 
 * Key discovery: ~400 frames needed for realistic movement. Tests must allow
 * sufficient time for physical movement, not just logical state changes.
 */
static int test_full_runner_scores_from_third(void)
{
ScenarioContext* ctx = create_scenario();

// Setup: Runner at third base with safety
place_runner_at_base(ctx, 0, BASE_THIRD, 0.0f);

// For third→home, we must set passedPathPoint correctly
// Setting to 1 means "already past the flag, heading to home"
ctx->state->localGameInfo->playerRuntime[0].passedPathPoint = 1;

// Ball on field (not interfering)
place_ball_at_location(ctx, (Vector3D){10.0f, 0.0f, -10.0f});

// Enable run checking (referee will score runs when conditions met)
ctx->state->localGameInfo->gameControl.checkForRun = 1;

// Trigger running using game's base-running machinery
trigger_player_run_to_next_base(ctx, 0, BASE_THIRD);

// Simulate until scoring (allow ample time for physical movement)
simulate_frames(ctx, 450);

int runs = ctx->state->localGameInfo->gameState.runsInTheInning;

cleanup_scenario(ctx);
ASSERT_EQ(1, runs, "Runner should have scored from third base");
return TEST_PASSED;
}

/**
 * TEST 2: Batter forced out at first base (natural throw scenario)
 * 
 * Scenario:
 * - Batter hits the ball
 * - Batter starts running to first base
 * - Ball is thrown toward first base
 * - Fielder catches the ball
 * - Ball arrives at first before batter
 * - Referee detects force out (§33 Pesäkilpa)
 * 
 * This tests the FULL force-out scenario naturally:
 * - Ball physics (flying through air)
 * - Fielder positioning and catching
 * - Batter running mechanics
 * - Race condition (ball vs runner)
 * - Referee force-out detection
 * 
 * Key discovery: Natural scenarios are more robust than "snapshot" setups
 */
static int test_full_batter_forced_out_at_first(void)
{
ScenarioContext* ctx = create_scenario();

// Setup: Fresh batter at home
setup_batter_at_home(ctx, 0);

// Fielder (first baseman) at first base
Vector3D firstBase = ctx->state->fieldPositions->firstBase;
ctx->state->localGameInfo->playerInfo[13].tPI.location = firstBase;
ctx->state->localGameInfo->playerInfo[13].tPI.homeLocation = firstBase;

// Throw ball from outfield toward first base using game's calibrated throwing
Vector3D throwFrom = {5.0f, 1.5f, -10.0f};
throw_ball_to_base(ctx, throwFrom, BASE_FIRST);

// Ball was hit (triggers game logic)
ctx->state->localGameInfo->pRAI.batHit = 1;
ctx->state->localGameInfo->pRAI.batterCanAdvance = 1;

// Start batter running to first
trigger_player_run_to_next_base(ctx, 0, BASE_HOME);

printf("  SETUP: Batter at home, ball flying to first, both racing...\n");

// Simulate the race: ball flight + batter running + fielder catching
// We need enough frames for:
// - Ball to reach first base
// - Fielder to catch it
// - Referee to detect the force out
for (int frame = 0; frame <= 300; frame += 50) {
if (frame > 0) simulate_frames(ctx, 50);

int outs = ctx->state->localGameInfo->gameState.outs;
int hasBall = ctx->state->localGameInfo->pII.hasBallIndex;
Vector3D ballLoc = ctx->state->localGameInfo->ballInfo.location;
Vector3D batterLoc = ctx->state->localGameInfo->playerInfo[0].tPI.location;

printf("  Frame %d: outs=%d hasBall=%d ball=(%.1f,%.1f,%.1f) batter=(%.1f,%.1f)\n",
       frame, outs, hasBall, ballLoc.x, ballLoc.y, ballLoc.z, batterLoc.x, batterLoc.z);

if (outs > 0) {
printf("  SUCCESS: Force out detected!\n");
break;
}
}

int outs = ctx->state->localGameInfo->gameState.outs;

cleanup_scenario(ctx);
ASSERT_EQ(1, outs, "Batter should be forced out at first base");
return TEST_PASSED;
}

void run_full_scenario_tests(void)
{
printf("Running Full-Scenario Integration Tests...\n");
RUN_TEST(test_full_runner_scores_from_third);
RUN_TEST(test_full_batter_forced_out_at_first);
}
