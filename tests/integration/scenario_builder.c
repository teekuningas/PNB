#include "scenario_builder.h"
#include "fixtures.h"
#include "game_setup.h"
#include "game_analysis.h"
#include "game_manipulation.h"
#include "mutable_world.h"
#include "common_logic.h"
#include "referee.h"
#include "action_implementation.h"
#include "action_implementation.h"
#include "vector_math.h"
#include <stdlib.h>
#include <string.h>

ScenarioContext* create_scenario(void)
{
	ScenarioContext* ctx = malloc(sizeof(ScenarioContext));
	memset(ctx, 0, sizeof(ScenarioContext));

	// Create basic state
	ctx->state = setup_test_state();

	// Initialize a fresh game
	ctx->seed = 0;
	GameSetup setup = {0};
	setup.launchType = GAME_LAUNCH_NEW;
	setup.gameMode = GAME_MODE_NORMAL;
	setup.team1 = 0;
	setup.team2 = 1;
	setup.team1_control = 0;  // User controlled
	setup.team2_control = 2;  // AI controlled
	setup.halfInningsInPeriod = 4;
	setup.playsFirst = 0;

	initializeGameFromMenu(ctx->state, &setup, &ctx->seed);
	initGameAnalysis(&(ctx->state->localGameInfo->gameFlowState));

	// Initialize referee state properly
	initializeRefereeState(&ctx->state->localGameInfo->referee);

	ctx->currentFrame = 0;

	return ctx;
}

void place_runner_at_base(ScenarioContext* ctx, int playerIndex, BaseID base, float progressToNext)
{
	if (!ctx || !ctx->state || playerIndex < 0 || playerIndex >= PLAYERS_IN_TEAM + JOKER_COUNT) {
		return;
	}

	LocalGameInfo* game = ctx->state->localGameInfo;
	FieldPositions* field = ctx->state->fieldPositions;

	// Determine physical start and end positions
	Vector3D startPos, endPos;

	switch (base) {
	case BASE_HOME:
		startPos = field->pitchPlate;
		startPos.z = 1.0f; // Slightly in front of plate
		endPos = field->firstBase;
		break;
	case BASE_FIRST:
		startPos = field->firstBase;
		endPos = field->secondBase;
		break;
	case BASE_SECOND:
		startPos = field->secondBase;
		endPos = field->thirdBase;
		break;
	case BASE_THIRD:
		startPos = field->thirdBase;
		endPos = field->pitchPlate;
		endPos.z = 1.0f; // Home scoring position
		break;
	default:
		return;
	}

	// Interpolate physical position
	Vector3D position;
	position.x = startPos.x + (endPos.x - startPos.x) * progressToNext;
	position.y = 0.0f;
	position.z = startPos.z + (endPos.z - startPos.z) * progressToNext;

	// Set physical state
	game->playerInfo[playerIndex].tPI.location = position;
	game->playerInfo[playerIndex].tPI.lastLocation = position;
	game->playerInfo[playerIndex].tPI.homeLocation = startPos;

	// Set logical state - this is the KEY part for consistency
	if (progressToNext < 0.1f) {
		// At the base - has safety
		game->playerInfo[playerIndex].bTPI.baseId = base;
		game->playerInfo[playerIndex].bTPI.state = PLAYER_STATE_ON_BASE;
		game->referee.battingPlayers[playerIndex].currentSafetyBase = base;
	} else {
		// Between bases - running, no safety
		game->playerInfo[playerIndex].bTPI.baseId = base;  // Still logically at source base
		game->playerInfo[playerIndex].bTPI.state = PLAYER_STATE_RUNNING;
		game->referee.battingPlayers[playerIndex].currentSafetyBase = base;  // Had safety at source
	}

	// Set referee tracking
	game->referee.battingPlayers[playerIndex].baseAtPitchStart = base;
	game->referee.battingPlayers[playerIndex].hadSafetyAtPitchStart = 1;
	game->referee.battingPlayers[playerIndex].hasPendingWound = 0;
	game->referee.battingPlayers[playerIndex].woundingType = WOUNDING_TYPE_NONE;

	// Set runtime state
	game->playerRuntime[playerIndex].arrivedToBase = (progressToNext < 0.1f) ? 1 : 0;
	game->playerRuntime[playerIndex].goingForward = 1;
	game->playerRuntime[playerIndex].hasMadeRunOnThirdBase = 0;
}

void place_ball_at_location(ScenarioContext* ctx, Vector3D location)
{
	if (!ctx || !ctx->state) return;

	LocalGameInfo* game = ctx->state->localGameInfo;

	game->ballInfo.location = location;
	game->ballInfo.lastLocation = location;
	game->ballInfo.visible = 1;
	game->ballInfo.moving = 0;
	game->ballInfo.hasHitGround = 1;  // Ball on ground
	game->ballInfo.onGround = 1;
	game->pII.hasBallIndex = -1;  // No one has it
}

void give_ball_to_fielder(ScenarioContext* ctx, int fielderIndex)
{
	if (!ctx || !ctx->state) return;

	LocalGameInfo* game = ctx->state->localGameInfo;

	// Put ball at fielder's location
	game->ballInfo.location = game->playerInfo[fielderIndex].tPI.location;
	game->ballInfo.lastLocation = game->ballInfo.location;
	game->ballInfo.visible = 0;  // Ball not visible when held
	game->ballInfo.moving = 0;
	game->pII.hasBallIndex = fielderIndex;
}

void trigger_player_run_to_next_base(ScenarioContext* ctx, int playerIndex, BaseID fromBase)
{
	if (!ctx || !ctx->state) return;

	// Call the game's base-running machinery
	runToNextBase(ctx->state->localGameInfo, ctx->state->fieldPositions, playerIndex, fromBase);
}

void trigger_player_run_to_previous_base(ScenarioContext* ctx, int playerIndex, BaseID toBase)
{
	if (!ctx || !ctx->state) return;

	// Call the game's base-running machinery
	// Note: runToPreviousBase treats 'toBase' as the base we are retreating TO (e.g. retreating TO Base 2 from Base 3).
	// But the game logic's `runToPreviousBase` actually takes `BaseID base` as the "current base" or "base we are retreating FROM"?
	// Let's check common_logic.c:
	// void runToPreviousBase(LocalGameInfo* localGameInfo, FieldPositions* fieldPositions, int index, BaseID base)
	// if(base == BASE_HOME) ... target = ready pos
	// if(base == BASE_FIRST) ... target = firstBaseRun
	// So `base` is the destination base.

	runToPreviousBase(ctx->state->localGameInfo, ctx->state->fieldPositions, playerIndex, toBase);
}

void setup_batter_at_home(ScenarioContext* ctx, int playerIndex)
{
	if (!ctx || !ctx->state) return;

	LocalGameInfo* game = ctx->state->localGameInfo;

	// Physical state: at home plate
	game->playerInfo[playerIndex].tPI.location = ctx->state->fieldPositions->pitchPlate;
	game->playerInfo[playerIndex].tPI.location.z += 1.0f;  // Slightly in front
	game->playerInfo[playerIndex].tPI.homeLocation = game->playerInfo[playerIndex].tPI.location;

	// Logical state: ready to bat/run
	game->playerInfo[playerIndex].bTPI.baseId = BASE_HOME;
	game->playerInfo[playerIndex].bTPI.state = PLAYER_STATE_AT_BAT;

	// Referee state: fresh batter
	game->referee.battingPlayers[playerIndex].currentSafetyBase = BASE_NONE;
	game->referee.battingPlayers[playerIndex].baseAtPitchStart = BASE_HOME;
	game->referee.battingPlayers[playerIndex].hadSafetyAtPitchStart = 0;

	// Runtime state
	game->playerRuntime[playerIndex].goingForward = 0;
	game->playerRuntime[playerIndex].passedPathPoint = 0;
}

int simulate_until(ScenarioContext* ctx, int (*condition)(ScenarioContext*), int maxFrames)
{
	if (!ctx || !condition) return 0;

	for (int i = 0; i < maxFrames; i++) {
		gameAnalysis(ctx->state, &ctx->menu, &ctx->seed);
		gameManipulation(ctx->state);
		LocalGameInfo* game = ctx->state->localGameInfo;
		Referee_Update(ctx->state, &game->referee, &game->gameState, &game->gameModeState, &game->gameControl, &game->playerCounters, ctx->state->globalGameInfo);
		reconcileLegalAndPhysicalState(ctx->state);
		ctx->currentFrame++;

		if (condition(ctx)) {
			return i + 1;  // Return frames taken
		}
	}

	return maxFrames;  // Timed out
}

int simulate_frames(ScenarioContext* ctx, int maxFrames)
{
	if (!ctx || !ctx->state) return 0;

	// Safety cap: prevent infinite loops
	if (maxFrames > 10000) {
		printf("WARNING: simulate_frames capped at 10000 frames (requested %d)\n", maxFrames);
		maxFrames = 10000;
	}

	for (int i = 0; i < maxFrames; i++) {
		// Run game progression
		gameAnalysis(ctx->state, &ctx->menu, &ctx->seed);
		gameManipulation(ctx->state);

		// Milestone 14: Rules engine must run after physics to reconcile state
		LocalGameInfo* game = ctx->state->localGameInfo;
		Referee_Update(ctx->state, &game->referee, &game->gameState, &game->gameModeState, &game->gameControl, &game->playerCounters, ctx->state->globalGameInfo);
		reconcileLegalAndPhysicalState(ctx->state);

		ctx->currentFrame++;
	}

	return maxFrames;
}

void cleanup_scenario(ScenarioContext* ctx)
{
	if (!ctx) return;

	if (ctx->state) {
		cleanup_test_state(ctx->state);
	}

	free(ctx);
}

void throw_ball_to_base(ScenarioContext* ctx, Vector3D fromLocation, BaseID targetBase)
{
	if (!ctx || !ctx->state) return;

	LocalGameInfo* game = ctx->state->localGameInfo;
	FieldPositions* field = ctx->state->fieldPositions;

// Get target position based on base
	Vector3D targetPos;
	switch (targetBase) {
	case BASE_HOME:
		targetPos = field->pitcher;
		break;
	case BASE_FIRST:
		targetPos = field->firstBase;
		break;
	case BASE_SECOND:
		targetPos = field->secondBase;
		break;
	case BASE_THIRD:
		targetPos = field->thirdBase;
		break;
	default:
		return;
	}

// Calculate direction and distance (same as game's prepareThrow)
	float dx = targetPos.x - fromLocation.x;
	float dz = targetPos.z - fromLocation.z;
	float distance = sqrtf(dx*dx + dz*dz);

	if (distance < 0.1f) return;

// Normalize direction
	dx /= distance;
	dz /= distance;

// Use game's throwing formula (from throwing_system.c constants)
#define THROW_POWER_CONSTANT 0.65f
#define THROW_DISTANCE_CONSTANT 0.0012f

	float power = 1.0f;  // Full power
	float dy = 0.06f + distance * THROW_DISTANCE_CONSTANT;

// Place ball at starting location
	game->ballInfo.location = fromLocation;
	game->ballInfo.lastLocation = fromLocation;
	game->pII.hasBallIndex = -1;

// Use the game's actual sling function (sets velocity + flags)
	// Trigger fielder selection update (same as game does after throws)
	game->pRAI.refreshCatchAndChange = 1;
	game->pRAI.initPlayerSelection = 1;
	genericSlingBall(&game->ballInfo,
	                 dx * power * THROW_POWER_CONSTANT,
	                 dy,
	                 dz * power * THROW_POWER_CONSTANT);
}

void hit_fly_ball_to_location(ScenarioContext* ctx, Vector3D fromLocation, Vector3D targetLocation)
{
	if (!ctx || !ctx->state) return;

	LocalGameInfo* game = ctx->state->localGameInfo;

	// Calculate direction
	float dx = targetLocation.x - fromLocation.x;
	float dz = targetLocation.z - fromLocation.z;
	float dist = sqrtf(dx*dx + dz*dz);

	if (dist < 0.1f) return;

	// Flight parameters
	// We want a high arc (Koppi)
	// Flight time T roughly 150 frames (approx 3 seconds at 50fps logic?)
	// Actually UPDATE_INTERVAL is 20ms, so 50 updates per second.
	// 150 frames = 3 seconds.

	float frames = 150.0f;
	float gravity = GRAVITY; // 0.003f

	// v_x = dx / frames
	// v_z = dz / frames
	// v_y needed to land at same height after T frames:
	// 0 = v_y * T - 0.5 * g * T^2
	// v_y = 0.5 * g * T

	float vy = 0.5f * gravity * frames;
	float vx = dx / frames;
	float vz = dz / frames;

	// Set starting location
	game->ballInfo.location = fromLocation;
	game->ballInfo.lastLocation = fromLocation;
	game->pII.hasBallIndex = -1;

	// Launch
	genericSlingBall(&game->ballInfo, vx, vy, vz);

	// Ensure ball is in "fly ball" state so a catch triggers wounding
	game->ballInfo.hasHitGround = 0;
	game->gameEvents.catchMade = 0;
	game->pRAI.batHit = 1; // Crucial: signals this ball came from the bat
}
