#include "scenario_builder.h"
#include "fixtures.h"
#include "game_setup.h"
#include "game_consolidation.h"
#include "game_manipulation.h"
#include "mutable_world.h"
#include "common_logic.h"
#include "referee.h"
#include "action_implementation.h"
#include "action_invocations.h"
#include "vector_math.h"
#include <stdlib.h>
#include <string.h>

ScenarioContext* create_scenario(void)
{
    ScenarioContext* ctx = malloc(sizeof(ScenarioContext));
    memset(ctx, 0, sizeof(ScenarioContext));

    // Create basic state
    ctx->state = setup_test_state();

    // Initialize game setup (scoreboard, teams, batting order)
    ctx->seed = 0;
    GameSetup setup = {0};
    setup.launchType = GAME_LAUNCH_NEW;
    setup.gameMode = GAME_MODE_NORMAL;
    setup.team1 = 0;
    setup.team2 = 1;
    setup.team1_control = 0;
    setup.team2_control = 2;
    setup.halfInningsInPeriod = 4;
    setup.playsFirst = 0;

    initializeGameFromMenu(ctx->state, &setup, &ctx->seed);

    // In tests, we don't have a game loop that responds to changeScreen=1,
    // so manually call loadMutableWorldSettings to initialize player counters
    loadMutableWorldSettings(ctx->state, &ctx->seed);

    ctx->currentFrame = 0;

    return ctx;
}

void initialize_referee_from_physical_state(ScenarioContext* ctx)
{
    // Initialize referee by scanning the physical world
    // This replaces the old gameInitialized event pattern
    initialize_referee(ctx->state, &ctx->state->match->referee);
}

void snapshot_pitch_start_state(ScenarioContext* ctx)
{
    // Emit the pitchReleased event
    ctx->state->match->gameEvents.pitchReleased = 1;

    // Simulate ONE frame so referee snapshots baseAtPitchStart
    simulate_frames(ctx, 1);

    // Event should now be cleared
}

void place_runner_at_base(ScenarioContext* ctx, int playerIndex, BaseID base, float progressToNext)
{
    if (!ctx || !ctx->state || playerIndex < 0 || playerIndex >= PLAYERS_IN_TEAM + JOKER_COUNT) {
        return;
    }

    MatchSession* game = ctx->state->match;
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

    // ONLY set PHYSICAL state - let referee handle legal state
    game->playerInfo[playerIndex].tPI.location = position;
    game->playerInfo[playerIndex].tPI.lastLocation = position;
    game->playerInfo[playerIndex].tPI.homeLocation = startPos;

    // Set logical state
    if (progressToNext < 0.1f) {
        // At the base
        game->playerInfo[playerIndex].bTPI.baseId = base;
        game->playerInfo[playerIndex].bTPI.state = PLAYER_STATE_ON_BASE;
    } else {
        // Between bases - running
        game->playerInfo[playerIndex].bTPI.baseId = base; // Still logically at source base
        game->playerInfo[playerIndex].bTPI.state = PLAYER_STATE_RUNNING;
    }

    // Set runtime state
    game->playerRuntime[playerIndex].arrivedToBase = (progressToNext < 0.1f) ? 1 : 0;
    game->playerRuntime[playerIndex].goingForward = 1;
    game->playerRuntime[playerIndex].hasMadeRunOnThirdBase = 0;

    // DO NOT touch referee state - let referee infer it from events
}

void move_pitcher_away(ScenarioContext* ctx)
{
    if (!ctx || !ctx->state) return;

    // Move pitcher (Lukkari, player index 12) far away from home plate
    // so they don't catch fly balls or interfere with runners
    Vector3D away = {100.0f, 0.0f, 100.0f};
    ctx->state->match->playerInfo[12].tPI.location = away;
}

void give_ball_to_pitcher(ScenarioContext* ctx)
{
    if (!ctx || !ctx->state) return;

    MatchSession* game = ctx->state->match;
    int pitcherIdx = 12; // Lukkari stands at home plate in pesäpallo

    // Ensure pitcher is at their home location (near home plate)
    game->playerInfo[pitcherIdx].tPI.location = game->playerInfo[pitcherIdx].tPI.homeLocation;

    // Give them the ball and snap ball position to match
    game->pII.hasBallIndex = pitcherIdx;
    game->ballInfo.location = game->playerInfo[pitcherIdx].tPI.location;
    game->ballInfo.lastLocation = game->ballInfo.location;
    game->ballInfo.moving = 0;
    game->ballInfo.onGround = 1;
    game->ballInfo.velocity = (Vector3D){0.0f, 0.0f, 0.0f};
}

void place_ball_over_location(ScenarioContext* ctx, Vector3D targetLocation)
{
    if (!ctx || !ctx->state) return;

    MatchSession* game = ctx->state->match;

    // Place ball in the air above the target location
    // It will naturally fall and hit the ground at/near targetLocation
    Vector3D startLocation = targetLocation;
    startLocation.y = 5.0f; // 5 meters above target

    game->ballInfo.location = startLocation;
    game->ballInfo.lastLocation = startLocation;
    game->ballInfo.visible = 1;
    game->ballInfo.moving = 1;
    game->ballInfo.onGround = 0;
    game->ballInfo.currentFlightHasHitGround = 0;

    // Give it a small downward velocity to start falling
    game->ballInfo.velocity.x = 0.0f;
    game->ballInfo.velocity.y = -0.1f; // Small initial downward velocity
    game->ballInfo.velocity.z = 0.0f;

    game->pII.hasBallIndex = -1; // No one has it
    // DO NOT set batHit here - that should be done via actual bat swing simulation
}

void trigger_player_run_to_next_base(ScenarioContext* ctx, int playerIndex, BaseID fromBase)
{
    if (!ctx || !ctx->state) return;

    // Call the game's base-running machinery
    runToNextBase(ctx->state->match, ctx->state->fieldPositions, playerIndex, fromBase);
}

void trigger_player_run_to_previous_base(ScenarioContext* ctx, int playerIndex, BaseID toBase)
{
    if (!ctx || !ctx->state) return;

    // Call the game's base-running machinery
    // Note: runToPreviousBase treats 'toBase' as the base we are retreating TO (e.g. retreating TO Base 2 from Base 3).
    // But the game logic's `runToPreviousBase` actually takes `BaseID base` as the "current base" or "base we are
    // retreating FROM"? Let's check common_logic.c: void runToPreviousBase(MatchSession* match, FieldPositions*
    // fieldPositions, int index, BaseID base) if(base == BASE_HOME) ... target = ready pos if(base == BASE_FIRST) ...
    // target = firstBaseRun So `base` is the destination base.

    runToPreviousBase(ctx->state->match, ctx->state->fieldPositions, playerIndex, toBase);
}

void setup_batter_at_home(ScenarioContext* ctx, int playerIndex)
{
    if (!ctx || !ctx->state) return;

    MatchSession* game = ctx->state->match;

    // Physical state: at home plate
    game->playerInfo[playerIndex].tPI.location = ctx->state->fieldPositions->pitchPlate;
    game->playerInfo[playerIndex].tPI.location.z += 1.0f; // Slightly in front
    game->playerInfo[playerIndex].tPI.homeLocation = game->playerInfo[playerIndex].tPI.location;

    // Logical state: ready to bat/run
    game->playerInfo[playerIndex].bTPI.baseId = BASE_HOME;
    game->playerInfo[playerIndex].bTPI.state = PLAYER_STATE_AT_BAT;

    // Referee state is NOT set here - let initialize_referee_from_physical_state() handle it

    // Runtime state
    game->playerRuntime[playerIndex].goingForward = 0;
    game->playerRuntime[playerIndex].passedPathPoint = 0;
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
        // actionInvocations() is intentionally omitted here: tests control player/AI decisions
        // explicitly via scenario helpers, not through the normal input dispatch path.
        actionImplementation(ctx->state, &ctx->seed);
        gameManipulation(ctx->state);

        // Milestone 14: Rules engine must run after physics to reconcile state
        MatchSession* game = ctx->state->match;
        update_referee(
            ctx->state, &game->referee, &game->halfInningState, &game->betweenPitchState, &game->playerCounters,
            &ctx->state->match->scoreboard, &game->homeRunContestState
        );
        GameConsolidation_Update(ctx->state, &ctx->menu, &ctx->seed);

        // Foul Play Reset is now handled by GameConsolidation_Update. Manual logic removed.

        // Clear transient events for next frame (Critical for correct event loop)
        clearFrameEvents(&game->gameEvents);

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

    MatchSession* game = ctx->state->match;
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
    float distance = sqrtf(dx * dx + dz * dz);

    if (distance < 0.1f) return;

    // Normalize direction
    dx /= distance;
    dz /= distance;

// Use game's throwing formula (from throwing_system.c constants)
#define THROW_POWER_CONSTANT 0.65f
#define THROW_DISTANCE_CONSTANT 0.0012f

    float power = 1.0f; // Full power
    float dy = 0.06f + distance * THROW_DISTANCE_CONSTANT;

    // Place ball at starting location
    game->ballInfo.location = fromLocation;
    game->ballInfo.lastLocation = fromLocation;

    // CRITICAL: Set lastHadBallIndex to prevent self-catching
    // (Same as in throwing_system.c line 77)
    game->pII.lastHadBallIndex = game->pII.hasBallIndex;
    game->pII.hasBallIndex = -1;

    // Use the game's actual sling function (sets velocity + flags)
    // Trigger fielder selection update (same as game does after throws)
    game->pRAI.refreshCatchAndChange = 1;
    game->pRAI.initPlayerSelection = 1;
    genericSlingBall(&game->ballInfo, dx * power * THROW_POWER_CONSTANT, dy, dz * power * THROW_POWER_CONSTANT);
}

void hit_fly_ball_to_location_with_time(
    ScenarioContext* ctx, Vector3D fromLocation, Vector3D targetLocation, float flightFrames
)
{
    if (!ctx || !ctx->state) return;

    MatchSession* game = ctx->state->match;

    // Calculate direction
    float dx = targetLocation.x - fromLocation.x;
    float dz = targetLocation.z - fromLocation.z;
    float dist = sqrtf(dx * dx + dz * dz);

    if (dist < 0.1f) return;

    // Flight parameters
    float gravity = GRAVITY; // 0.003f

    // v_x = dx / frames
    // v_z = dz / frames
    // v_y needed to land at same height after T frames:
    // 0 = v_y * T - 0.5 * g * T^2
    // v_y = 0.5 * g * T

    float vy = 0.5f * gravity * flightFrames;
    float vx = dx / flightFrames;
    float vz = dz / flightFrames;

    // Set starting location
    game->ballInfo.location = fromLocation;
    game->ballInfo.lastLocation = fromLocation;
    game->pII.hasBallIndex = -1;

    // Launch
    genericSlingBall(&game->ballInfo, vx, vy, vz);

    // Ensure ball is in "fly ball" state so a catch triggers wounding
    game->ballInfo.currentFlightHasHitGround = 0;
    game->betweenPitchState.batOutcome = BAT_OUTCOME_HIT; // Crucial: signals this ball came from the bat
}

void hit_fly_ball_to_location(ScenarioContext* ctx, Vector3D fromLocation, Vector3D targetLocation)
{
    hit_fly_ball_to_location_with_time(ctx, fromLocation, targetLocation, 150.0f);
}

void perform_pitch(ScenarioContext* ctx, float targetX)
{
    if (!ctx || !ctx->state) return;

    MatchSession* game = ctx->state->match;
    FieldPositions* field = ctx->state->fieldPositions;

    // 1. Locate Pitcher
    Vector3D startPos = field->pitcher;

    // 2. Calculate Velocity
    // Pitch is mostly vertical (Y) + horizontal error (X). Z is usually negligible.
    float frames = 100.0f; // Flight time
    float gravity = GRAVITY;

    // Target Y is plate height (0). Start Y is slightly elevated?
    // Actually pitcher holds ball at some height. Let's assume start Y=1.5.
    startPos.y = 1.5f;

    // v_y to land at 0 after T frames:
    // 0 = y0 + v_y * T - 0.5 * g * T^2
    // v_y * T = 0.5 * g * T^2 - y0
    // v_y = 0.5 * g * T - y0 / T
    float vy = 0.5f * gravity * frames - startPos.y / frames;

    // v_x to reach targetX from startPos.x
    float vx = (targetX - startPos.x) / frames;

    // v_z (keep at plate Z)
    float vz = (field->pitchPlate.z - startPos.z) / frames;

    // 3. Set Ball State
    game->ballInfo.location = startPos;
    game->ballInfo.lastLocation = startPos;
    game->pII.hasBallIndex = -1;
    genericSlingBall(&game->ballInfo, vx, vy, vz);
    game->ballInfo.currentFlightHasHitGround = 0;

    // 4. Set Pitch State
    game->pRAI.pitchState = PITCH_STAGE_AIRBORNE;
    game->pRAI.batterCanAdvance = 1;
    game->gameEvents.pitchReleased = 1; // Signal event - referee will snapshot state automatically

    // Note: We emit pitchReleased event which triggers the referee to snapshot
    // baseAtPitchStart and strikesAtPitchStart automatically in update_initialization_events().
    // No need to manually set referee state here - that would violate Referee Supremacy.
}
