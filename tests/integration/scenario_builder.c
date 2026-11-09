#include "game_reset.h"
#include "rules_pure/player_utils.h"
#include <stdio.h>
#include <math.h>
#include "field.h"
#include "rules_state.h"
#include "world.h"
#include "scenario_builder.h"
#include "fixtures.h"
#include "game_setup.h"
#include "game_consolidation.h"
#include "simulate.h"
#include "ball_flight.h"
#include "frame_events.h"
#include "runner_paths.h"
#include "fielder_behaviours.h"
#include "referee.h"
#include "intent_channel.h"
#include "ingest.h"
#include "actualize.h"
#include "action_invocations.h"
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
    // The batting order is not decoration: it is what gives players their jersey numbers and their
    // JOKER_REGULAR / JOKER_AVAILABLE status (game_setup.c → initialize_inning_permanent_player_information),
    // and §12's side change is defined on it. Left zeroed — as it was until 2026-08-23 — every slot named
    // player 0, so nine "regulars" and three "jokers" were all the same person and no scenario test could
    // say anything true about the order. The sim and the fixture factories have always used this identity
    // order; the scenario tier now matches them.
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        setup.team1_batting_order[i] = i;
        setup.team2_batting_order[i] = i;
    }

    initialize_game_from_menu(ctx->state, &setup, &ctx->seed);

    // In tests, we don't have a game loop that responds to changeScreen=1,
    // so manually call reset_for_new_half_inning to initialize physical+flow+team,
    // then referee_reset_for_new_inning to clear all referee state (from-menu init).
    reset_for_new_half_inning(ctx->state->match, ctx->state->fieldPositions, ctx->state->teamData, ctx->state->rules);
    referee_reset_for_new_inning(
        &ctx->state->rules->referee, &ctx->state->rules->halfInningState, &ctx->state->rules->betweenPitchState
    );

    ctx->currentFrame = 0;

    return ctx;
}

void initialize_referee_from_physical_state(ScenarioContext* ctx)
{
    // Initialize referee by scanning the physical world
    // This replaces the old gameInitialized event pattern
    initialize_referee(ctx->state->match, &ctx->state->rules->scoreboard, &ctx->state->rules->referee);
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
    game->playerInfo[playerIndex].body.location = position;
    game->playerInfo[playerIndex].body.previous_location = position;
    game->playerInfo[playerIndex].body.home_location = startPos;

    // Set logical state
    if (progressToNext < 0.1f) {
        // At the base
        game->playerInfo[playerIndex].batting.presence.base_id = base;
        game->playerInfo[playerIndex].batting.presence.state = PLAYER_STATE_ON_BASE;
    } else {
        // Between bases - running
        game->playerInfo[playerIndex].batting.presence.base_id = base; // Still logically at source base
        game->playerInfo[playerIndex].batting.presence.state = PLAYER_STATE_RUNNING;
    }

    // Set runtime state
    game->playerInfo[playerIndex].path.arrived_at_base = (progressToNext < 0.1f) ? 1 : 0;
    game->playerInfo[playerIndex].path.going_forward = 1;

    // DO NOT touch referee state - let referee infer it from events
}

int scenario_joker(ScenarioContext* ctx, int k)
{
    int jokers[JOKER_COUNT];
    batting_team_jokers(&ctx->state->rules->scoreboard, jokers);
    return jokers[k];
}

void exhaust_jokers(ScenarioContext* ctx)
{
    if (!ctx || !ctx->state) return;

    MatchSession* game = ctx->state->match;
    for (int i = 0; i < JOKER_COUNT; i++) {
        game->playerInfo[scenario_joker(ctx, i)].batting.joker = JOKER_USED;
    }
    ctx->state->rules->halfInningState.jokersLeft = 0;
}

void move_pitcher_away(ScenarioContext* ctx)
{
    if (!ctx || !ctx->state) return;

    // Move pitcher (Lukkari, player index 12) far away from home plate
    // so they don't catch fly balls or interfere with runners
    Vector3D away = {100.0f, 0.0f, 100.0f};
    ctx->state->match->playerInfo[12].body.location = away;
}

void give_ball_to_pitcher(ScenarioContext* ctx)
{
    if (!ctx || !ctx->state) return;

    MatchSession* game = ctx->state->match;
    int pitcherIdx = 12; // Lukkari stands at home plate in pesäpallo

    // Ensure pitcher is at their home location (near home plate)
    game->playerInfo[pitcherIdx].body.location = game->playerInfo[pitcherIdx].body.home_location;

    // Give them the ball and snap ball position to match
    game->ballInfo.holder = pitcherIdx;
    game->ballInfo.location = game->playerInfo[pitcherIdx].body.location;
    game->ballInfo.lastLocation = game->ballInfo.location;
    game->ballInfo.moving = 0;
    game->ballInfo.onGround = 1;
    game->ballInfo.velocity = (Vector3D){0.0f, 0.0f, 0.0f};
}

void setup_pitcher_ready(ScenarioContext* ctx)
{
    if (!ctx || !ctx->state) return;

    MatchSession* game = ctx->state->match;
    const int pitcherIdx = 12; // Lukkari, the home catcher

    give_ball_to_pitcher(ctx); // ball in hand at the plate
    game->fielding_posts.baseman[0] = pitcherIdx; // the pitcher is the home catcher
    game->playerInfo[pitcherIdx].fielding.near_home = 1; // close enough to pitch
    game->at_bat.cycle = BATTING_CYCLE_AWAITING_PITCH; // a batter is in, standing ready
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

    game->ballInfo.holder = -1; // No one has it
    // DO NOT set batHit here - that should be done via actual bat swing simulation
}

void trigger_player_run_to_next_base(ScenarioContext* ctx, int playerIndex, BaseID fromBase)
{
    if (!ctx || !ctx->state) return;

    // Call the game's base-running machinery
    run_to_next_base(ctx->state->match, ctx->state->fieldPositions, playerIndex, fromBase);
}

void trigger_player_run_to_previous_base(ScenarioContext* ctx, int playerIndex, BaseID toBase)
{
    if (!ctx || !ctx->state) return;

    // Call the game's base-running machinery
    // Note: run_to_previous_base treats 'toBase' as the base we are retreating TO (e.g. retreating TO Base 2 from Base
    // 3). But the game logic's `run_to_previous_base` actually takes `BaseID base` as the "current base" or "base we
    // are retreating FROM"? Let's check runner_paths.c: void run_to_previous_base(MatchSession* match, FieldPositions*
    // fieldPositions, int index, BaseID base) if(base == BASE_HOME) ... target = ready pos if(base == BASE_FIRST) ...
    // target = firstBaseRun So `base` is the destination base.

    run_to_previous_base(ctx->state->match, ctx->state->fieldPositions, playerIndex, toBase);
}

void setup_batter_at_home(ScenarioContext* ctx, int playerIndex)
{
    if (!ctx || !ctx->state) return;

    MatchSession* game = ctx->state->match;

    // Physical state: at home plate
    game->playerInfo[playerIndex].body.location = ctx->state->fieldPositions->pitchPlate;
    game->playerInfo[playerIndex].body.location.z += 1.0f; // Slightly in front
    game->playerInfo[playerIndex].body.home_location = game->playerInfo[playerIndex].body.location;

    // Logical state: ready to bat/run
    game->playerInfo[playerIndex].batting.presence.base_id = BASE_HOME;
    game->playerInfo[playerIndex].batting.presence.state = PLAYER_STATE_AT_BAT;

    // Referee state is NOT set here - let initialize_referee_from_physical_state() handle it

    // Runtime state
    game->playerInfo[playerIndex].path.going_forward = 0;
    game->playerInfo[playerIndex].path.waypoint = 0;
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
        // action_invocations() is intentionally omitted here: in this tier the TEST is the producer.
        // It declares intent straight onto the channel (intent_push), and the INGEST gate
        // judges and drains it exactly as it would a human's or the AI's.
        {
            IngestedCommands ingested =
                ingest_intents(ctx->state->match, ctx->state->rules, ctx->state->fieldPositions, &ctx->state->channels);
            actualize(
                ctx->state->match, ctx->state->rules, ctx->state->fieldPositions, &ingested,
                &ctx->state->playSoundEffect
            );
        }
        simulate(
            ctx->state->match, ctx->state->fieldPositions, &ctx->state->rules->referee, &ctx->state->playSoundEffect
        );

        // Milestone 14: Rules engine must run after physics to reconcile state
        GameRulesState* rules = ctx->state->rules;
        update_referee(
            ctx->state->match, ctx->state->fieldPositions, &rules->referee, &rules->halfInningState,
            &rules->betweenPitchState, &rules->scoreboard, &rules->homeRunContestState, &ctx->state->announcement
        );
        ConsolidationOutput consolidation_output;
        consolidation_update(
            ctx->state->match, ctx->state->fieldPositions, ctx->state->teamData, ctx->state->gameConclusion, rules,
            &consolidation_output
        );
        // Handle screen transition in test context
        if (consolidation_output.request_screen_change) {
            ctx->state->screen = consolidation_output.target_screen;
            ctx->state->changeScreen = 1;
            ctx->state->updated = 0;
        }

        // Foul Play Reset is now handled by consolidation_update. Manual logic removed.

        // 5. Referee Finalize (RESETTING→NONE transitions)
        referee_finalize(ctx->state->match, &rules->referee, &rules->betweenPitchState);

        // Clear transient events for next frame (Critical for correct event loop)
        clear_frame_events(&ctx->state->match->gameEvents);

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

void scenario_throw_ball_to_base(ScenarioContext* ctx, Vector3D fromLocation, BaseID targetBase)
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

    // CRITICAL: Set released_by to prevent self-catching
    // (Same as in throwing_system.c line 77)
    game->ballInfo.released_by = game->ballInfo.holder;
    game->ballInfo.holder = -1;

    // Use the game's actual sling function (sets velocity + flags)
    // Trigger fielder selection update (same as game does after throws)
    request_chase_replan(game, CHASE_REPLAN_AND_HAND_OVER);
    generic_sling_ball(&game->ballInfo, dx * power * THROW_POWER_CONSTANT, dy, dz * power * THROW_POWER_CONSTANT);
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
    game->ballInfo.holder = -1;

    // Launch
    generic_sling_ball(&game->ballInfo, vx, vy, vz);

    // Ensure ball is in "fly ball" state so a catch triggers wounding
    game->ballInfo.currentFlightHasHitGround = 0;
    ctx->state->rules->betweenPitchState.batOutcome = BAT_OUTCOME_HIT; // Crucial: signals this ball came from the bat
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
    game->ballInfo.holder = -1;
    generic_sling_ball(&game->ballInfo, vx, vy, vz);
    game->ballInfo.currentFlightHasHitGround = 0;

    // 4. Set Pitch State
    game->ballInfo.pitch_in_flight = 1;
    game->at_bat.may_leave_home = 1;
    game->gameEvents.pitchReleased = 1; // Signal event - referee will snapshot state automatically

    // Note: We emit pitchReleased event which triggers the referee to snapshot
    // baseAtPitchStart and strikesAtPitchStart automatically in update_initialization_events().
    // No need to manually set referee state here - that would violate Referee Supremacy.
}
