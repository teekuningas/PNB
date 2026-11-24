#include "game_reset.h"
#include <math.h>
#include "field.h"
#include "team_data.h"
#include "rules_state.h"
#include "world.h"
#include "player_motion.h"
#include "actions/batting_system.h"
#include "frame_events.h"
#include "rng.h"
#include "rules_pure/player_utils.h"

static void initialize_ball_info(MatchSession* match);
static void initialize_inning_permanent_player_information(
    MatchSession* match, const Scoreboard* scoreboard, const TeamData* team_data
);
static void initialize_non_critical_player_information(MatchSession* match);
static void initialize_spatial_player_information(MatchSession* match, const FieldPositions* field_positions);
static void reset_flow_state(MatchSession* match);
static void setup_homerun_physical_state(
    MatchSession* match, const Scoreboard* scoreboard, const HomeRunContestState* hrcs,
    const FieldPositions* field_positions, int batterResumesInPlace
);

static void reset_physical_world(MatchSession* match, const FieldPositions* field_positions)
{
    initialize_ball_info(match);
    initialize_action_info(match);
    initialize_spatial_player_information(match, field_positions);
    initialize_non_critical_player_information(match);
}

// Recipe 1: Full physical + flow + team reset for new half-inning.
// Called by consolidation when referee signals END_INNING_STATE_RESETTING.
// NOTE: Referee has already cleared its own legal state at DETECTED→RESETTING.
// This function does NOT touch referee-owned state (ownership boundary).
void reset_for_new_half_inning(
    MatchSession* match, const FieldPositions* field_positions, const TeamData* team_data, const GameRulesState* rules
)
{
    reset_physical_world(match, field_positions);
    reset_flow_state(match);

    // Team setup
    initialize_inning_permanent_player_information(match, &rules->scoreboard, team_data);

    if (rules->scoreboard.period >= 4) {
        // The referee has already set the contest's pair counter for this half-inning (at the side change),
        // or the menu has (starting a contest).
        setup_homerun_physical_state(match, &rules->scoreboard, &rules->homerun_contest, field_positions, 0);
    }
}

// Recipe 2: Foul play — referee already restored legal state from snapshot
void reset_for_foul_play(MatchSession* match, const FieldPositions* field_positions, const GameRulesState* rules)
{
    reset_physical_world(match, field_positions);
    reset_flow_state(match);

    if (rules->scoreboard.period >= 4) {
        // Homerun Contest: same batter resumes after a foul/out-of-bounds, so keep them at the plate.
        setup_homerun_physical_state(match, &rules->scoreboard, &rules->homerun_contest, field_positions, 1);
    } else {
        // restorePlayersToRefereePositions: Restore players to their bases at the start of the pitch
        for (int j = 0; j < PLAYERS_IN_TEAM + JOKER_COUNT; j++) {
            if (rules->referee.batting_players[j].base_at_pitch_start != BASE_NONE) {
                BaseID restoreBase = rules->referee.batting_players[j].base_at_pitch_start;

                // 1. Restore Player State and ID (Physical/Logical State)
                if (restoreBase == BASE_HOME) {
                    match->players[j].batting.presence.state = PLAYER_STATE_AT_BAT;
                } else {
                    match->players[j].batting.presence.state = PLAYER_STATE_ON_BASE;
                }
                match->players[j].batting.presence.base_id = restoreBase;

                // 5. Handle the Batter - Physical State Only
                if (restoreBase == BASE_HOME) {
                    // Restore readiness for the next pitch.
                    prepare_batter(match);
                }

                // 6. Restore Physical Locations for field runners
                if (match->players[j].batting.presence.base_id == BASE_FIRST) {
                    match->players[j].body.location.x = field_positions->first_base_run.x;
                    match->players[j].body.location.z = field_positions->first_base_run.z;
                } else if (match->players[j].batting.presence.base_id == BASE_SECOND) {
                    match->players[j].body.location.x = field_positions->second_base_run.x;
                    match->players[j].body.location.z = field_positions->second_base_run.z;
                } else if (match->players[j].batting.presence.base_id == BASE_THIRD) {
                    match->players[j].body.location.x = field_positions->third_base_run.x;
                    match->players[j].body.location.z = field_positions->third_base_run.z;
                }
            } else {
                // Restore OUT/SCORED/WOUNDED so enforcement does not start their exit again.
                // This is physical state sync
                if (rules->referee.batting_players[j].status == PLAYER_STATUS_OUT) {
                    match->players[j].batting.presence.state = PLAYER_STATE_OUT;
                    match->players[j].batting.presence.base_id = BASE_NONE;
                } else if (rules->referee.batting_players[j].has_scored) {
                    match->players[j].batting.presence.state = PLAYER_STATE_SCORED;
                    match->players[j].batting.presence.base_id = BASE_NONE;
                } else if (rules->referee.batting_players[j].status == PLAYER_STATUS_WOUNDED) {
                    match->players[j].batting.presence.state = PLAYER_STATE_WOUNDED;
                    match->players[j].batting.presence.base_id = BASE_NONE;
                }
            }
        }
    }
}

// Recipe 3: Next HR pair — referee already cleared per-pair state
void reset_for_next_pair(
    MatchSession* match, const FieldPositions* field_positions, const Scoreboard* scoreboard,
    const HomeRunContestState* hrcs
)
{
    reset_physical_world(match, field_positions);
    reset_flow_state(match);
    setup_homerun_physical_state(match, scoreboard, hrcs, field_positions, 0);
}

// ---- the reset building blocks the recipes above compose ----

static void initialize_spatial_player_information(MatchSession* match, const FieldPositions* field_positions)
{
    int i;
    const Vector3D* fieldPosition;
    int battingTeamPlacement[PLAYERS_IN_TEAM + JOKER_COUNT];
    // create array that has 0-11 in random order to give batting players in home
    // circle a nicer visual feelin'
    for (i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        battingTeamPlacement[i] = -1;
    }
    i = 0;
    while (i < PLAYERS_IN_TEAM + JOKER_COUNT) {
        int random = seeded_rand(&match->rng_seed, PLAYERS_IN_TEAM + JOKER_COUNT);
        if (battingTeamPlacement[random] == -1) {
            battingTeamPlacement[random] = i;
            i++;
        }
    }
    // when out of bounds situation or inning ends, we swing the ball in the air and let the pitcher
    // catch it
    match->ball.velocity.x = BALL_INIT_SPEED_X;
    match->ball.velocity.y = BALL_INIT_SPEED_Y;
    match->ball.velocity.z = BALL_INIT_SPEED_Z;
    match->ball.location.x = BALL_INIT_LOCATION_X;
    match->ball.location.y = BALL_INIT_LOCATION_Y;
    match->ball.location.z = BALL_INIT_LOCATION_Z;

    match->ball.previous_location.x = match->ball.location.x;
    match->ball.previous_location.y = match->ball.location.y;
    match->ball.previous_location.z = match->ball.location.z;
    // Place the batting team around home.
    for (i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        float radiusFix = (float)(1.0f + fabs(5.5f - battingTeamPlacement[i]) / 20.0f);

        match->players[i].body.home_location.x =
            (float)(field_positions->pitch_plate.x +
                    (HOME_RADIUS)*radiusFix *
                        cos(PI - (battingTeamPlacement[i] + 1) * PI / (PLAYERS_IN_TEAM + JOKER_COUNT + 1)));
        match->players[i].body.home_location.y = BALL_HEIGHT_WITH_PLAYER;
        match->players[i].body.home_location.z =
            (float)(field_positions->pitch_plate.z +
                    (HOME_RADIUS)*radiusFix *
                        sin(PI - (battingTeamPlacement[i] + 1) * PI / (PLAYERS_IN_TEAM + JOKER_COUNT + 1)));
        match->players[i].body.location.x = match->players[i].body.home_location.x;
        match->players[i].body.location.y = match->players[i].body.home_location.y;
        match->players[i].body.location.z = match->players[i].body.home_location.z;
        match->players[i].body.orientation.x = match->ball.location.x - match->players[i].body.location.x;
        match->players[i].body.orientation.y = match->ball.location.y - match->players[i].body.location.y;
        match->players[i].body.orientation.z = match->ball.location.z - match->players[i].body.location.z;
        match->players[i].body.previous_location.x = match->players[i].body.location.x;
        match->players[i].body.previous_location.y = match->players[i].body.location.y;
        match->players[i].body.previous_location.z = match->players[i].body.location.z;
    }
    // Place fielders in their posts.
    for (i = 12; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {

        switch (i - 12) {
        case 0:
            fieldPosition = &(field_positions->pitcher);
            break;
        case 1:
            fieldPosition = &(field_positions->first_base);
            break;
        case 2:
            fieldPosition = &(field_positions->second_base);
            break;
        case 3:
            fieldPosition = &(field_positions->third_base);
            break;
        case 4:
            fieldPosition = &(field_positions->bottom_right_catcher);
            break;
        case 5:
            fieldPosition = &(field_positions->middle_left_catcher);
            break;
        case 6:
            fieldPosition = &(field_positions->middle_right_catcher);
            break;
        case 7:
            fieldPosition = &(field_positions->back_left_catcher);
            break;
        case 8:
            fieldPosition = &(field_positions->back_right_catcher);
            break;
        default:
            fieldPosition = &(field_positions->pitch_plate);
            break;
        }
        match->players[i].body.home_location.x = fieldPosition->x;
        match->players[i].body.home_location.y = fieldPosition->y;
        match->players[i].body.home_location.z = fieldPosition->z;

        match->players[i].body.location.x = match->players[i].body.home_location.x;
        match->players[i].body.location.y = match->players[i].body.home_location.y;
        match->players[i].body.location.z = match->players[i].body.home_location.z;

        match->players[i].body.orientation.x = match->ball.location.x - match->players[i].body.location.x;
        match->players[i].body.orientation.y = match->ball.location.y - match->players[i].body.location.y;
        match->players[i].body.orientation.z = match->ball.location.z - match->players[i].body.location.z;

        match->players[i].body.previous_location.x = match->players[i].body.location.x;
        match->players[i].body.previous_location.y = match->players[i].body.location.y;
        match->players[i].body.previous_location.z = match->players[i].body.location.z;
    }
    // these are set for every player.
    for (i = 0; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        match->players[i].body.velocity.x = 0.0f;
        match->players[i].body.velocity.y = 0.0f;
        match->players[i].body.velocity.z = 0.0f;
        match->players[i].body.target_location.x = 0.0f;
        match->players[i].body.target_location.y = 0.0f;
        match->players[i].body.target_location.z = 0.0f;
    }
}
// here we initialize players' stat information, and team information, whether they are joker or not,
// their number etc. all is information that is not going to be reinitialized when out of bounds -
// situation happens
static void initialize_inning_permanent_player_information(
    MatchSession* match, const Scoreboard* scoreboard, const TeamData* team_data
)
{
    int battingTeamIndex = get_batting_team_index(scoreboard);
    int i;

    // initialize batting team numbers and jokerness
    for (i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        if (i < PLAYERS_IN_TEAM) {
            match->players[scoreboard->teams[battingTeamIndex].batter_order[i]].batting.number = i + 1;
            match->players[scoreboard->teams[battingTeamIndex].batter_order[i]].batting.joker = JOKER_REGULAR;
        } else {
            match->players[scoreboard->teams[battingTeamIndex].batter_order[i]].batting.number = 0;
            match->players[scoreboard->teams[battingTeamIndex].batter_order[i]].batting.joker = JOKER_AVAILABLE;
        }
    }
    // initialize batting team
    for (i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        match->players[i].side = TEAM_BATTING;

        match->players[i].attributes.power =
            team_data[(scoreboard->teams[battingTeamIndex].value - 1)].players[i].power;
        match->players[i].attributes.speed =
            team_data[(scoreboard->teams[battingTeamIndex].value - 1)].players[i].speed;
    }
    // initialize fielders
    for (i = 12; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        match->players[i].side = TEAM_CATCHING;

        // here we set catcherOnBaseIndices and catcherReplacerOnBaseIndices.
        switch (i - 12) {
        case 0:
            match->fielding_posts.baseman[0] = i;
            break;
        case 1:
            match->fielding_posts.baseman[1] = i;
            break;
        case 2:
            match->fielding_posts.baseman[2] = i;
            break;
        case 3:
            match->fielding_posts.baseman[3] = i;
            break;
        case 4:
            match->fielding_posts.cover[0] = i;
            match->fielding_posts.cover[1] = i;
            break;
        case 5:
            match->fielding_posts.cover[3] = i;
            break;
        case 6:
            match->fielding_posts.cover[2] = i;
            break;
        }
    }
}
// information that can be flushed
static void initialize_non_critical_player_information(MatchSession* match)
{
    int i;
    for (i = 0; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        match->players[i].path.arrived_at_base = 0;
        match->players[i].path.waypoint = 0;
        match->players[i].path.going_forward = 0;

        match->players[i].events =
            (PlayerActivityEvents){.latest = PLAYER_ACTIVITY_RESET, .started = PLAYER_ACTIVITY_RESET};
        match->players[i].motion.moving = 0;
        match->players[i].motion.running = 0;
        match->players[i].motion.following_target = 0;
        match->players[i].motion.sync_previous_location = 1;

        match->players[i].fielding.throw_recovery_ticks = 0;

        if (i >= PLAYERS_IN_TEAM + JOKER_COUNT) {

            match->players[i].fielding.near_home = 1;
            match->players[i].fielding.replacement = REPLACEMENT_IDLE;
            match->players[i].fielding.replacement_base = BASE_NONE;
            match->players[i].fielding.busy_catching = 0;
        } else {
            match->players[i].batting.presence.state = PLAYER_STATE_IDLE;
            match->players[i].batting.presence.base_id = BASE_NONE;
        }
    }
}
// ball flags
static void initialize_ball_info(MatchSession* match)
{
    match->ball.visible = 1;
    match->ball.moving = 1;
    match->ball.flight_has_hit_ground = 0;
    match->ball.on_ground = 0;
    match->ball.hit_ground_to_unwound = 0;
    match->ball.needs_move_update = 0;
    match->ball.sync_previous_location = 0;
    match->ball.pitch_in_flight = 0;
    match->ball.thrown_to = BASE_NONE;
    match->ball.holder = -1;
    match->ball.released_by = -1;
}
// Empty everything a play can have left on the table that is not physics: the catching action in progress,
// the at-bat swing cycle, the runners' standing orders, a chase
// re-plan not yet answered, and the catching team's own state — who is controlled and who chases, where
// the controlled fielder was told to go, and the engine's prediction of where the ball can next be met. The last of
// those is not producer-set, but it is just as stale after a reset and it is cleared here so that ONE recipe owns the
// whole struct; the alternative was two recipes clearing two fields of it, which is how the channel slice lost a
// declaration. Each is cleared as a WHOLE STRUCT, never field by field — the field-by-field version this
// replaced had silently missed two, and a struct assignment cannot miss a field that gets added later.
//
// The message channel needs no counterpart here: it is drained to empty inside every tick, so a
// reset has nothing of it to clean up.
void initialize_action_info(MatchSession* match)
{
    match->catching = (CatchingTeamState){0};
    // Nobody is controlled until a fielder takes the ball, and until the ball's first flight is ranked the
    // chase order is the first five fielders by position.
    match->catching.controlled_fielder = -1;
    match->catching.ranking_cursor = -1;
    for (int k = 0; k < RANKED_FIELDERS_COUNT; k++) {
        match->catching.chase_ranking[k] = PLAYERS_IN_TEAM + JOKER_COUNT + k;
    }

    match->catching_action = (CatchingAction){0};

    match->at_bat = (AtBat){0};
    match->at_bat.swing.contact_frame = -1; // no ball in the air, so no frame to meet it on

    match->runner_commitments = (RunnerCommitments){0};

    match->chase_replan = CHASE_REPLAN_NONE;
}
// Resets flow control, subsystems, and frame events for a clean restart.
// Does NOT touch referee-owned state (BPS, HIS, RefereeState).
static void reset_flow_state(MatchSession* match)
{
    // No question is open after a reset, and there is no BALL whose free walk is still to be worked out.
    match->offers = (BattingOffers){0};
    match->free_walk = (FreeWalkEntitlement){.worked_out = 1, .runner = -1, .from = BASE_NONE};

    // Frame events (cleared every frame, but ensure clean start)
    clear_frame_events(&match->events);

    // The catching action in progress belongs to initialize_action_info, and the ball's flight to
    // initialize_ball_info (both in reset_physical_world, which every recipe runs before this) — one
    // owner per field.
    // NOTE: client-local input (run_press_window, pitch_widget — in ClientInputState) is deliberately
    // NOT reset here. A physical-world reset must not reach into the client-local input world, the same
    // separation as the reset-recipe rule: physical resets never touch referee state. Client input is initialized once
    // and self-clears during play (each gesture's own guards disengage when its preconditions fail), so a foul/inning
    // reset needs to do nothing to it.
    // The same holds for the AI's memory (AIControllerState), in both directions: a controller notices a reset by
    // observing the world it leaves behind, never by the engine reaching in to clear it.
}

static void setup_homerun_physical_state(
    MatchSession* match, const Scoreboard* scoreboard, const HomeRunContestState* hrcs,
    const FieldPositions* field_positions, int batterResumesInPlace
)
{
    int battingTeamIndex = get_batting_team_index(scoreboard);
    Vector3D target;
    int i;
    if (hrcs->pair_index < scoreboard->pair_count) {
        int runnerIndex = scoreboard->teams[battingTeamIndex].batter_runner_indices[1][hrcs->pair_index];
        int batterIndex = scoreboard->teams[battingTeamIndex].batter_runner_indices[0][hrcs->pair_index];
        // batter
        if (batterIndex != -1) {
            match->players[batterIndex].batting.presence.base_id = BASE_HOME;
            match->players[batterIndex].batting.presence.state = PLAYER_STATE_AT_BAT;
            match->players[batterIndex].batting.number = hrcs->pair_index + 1;
            // move player to default batter ready position
            target.x = (float)(field_positions->pitch_plate.x + cos(ZERO_BATTING_ANGLE) * BATTING_RADIUS);
            target.z = (float)(field_positions->pitch_plate.z - sin(ZERO_BATTING_ANGLE) * BATTING_RADIUS);
            // On a foul/out-of-bounds reset the same batter resumes, so start them already at
            // the plate — move_to_target then settles them straight into the ready stance
            // instead of walking them in from the reset position.
            if (batterResumesInPlace) {
                match->players[batterIndex].body.location.x = target.x;
                match->players[batterIndex].body.location.z = target.z;
                match->players[batterIndex].body.previous_location = match->players[batterIndex].body.location;
            }
            move_to_target(match->players, batterIndex, &target);
        }
        // runner
        if (runnerIndex != -1) {
            match->players[runnerIndex].batting.presence.base_id = BASE_THIRD;
            match->players[runnerIndex].batting.presence.state = PLAYER_STATE_ON_BASE;

            match->players[runnerIndex].body.location.x = field_positions->third_base_run.x;
            match->players[runnerIndex].body.location.y = field_positions->third_base_run.y;
            match->players[runnerIndex].body.location.z = field_positions->third_base_run.z;
            match->players[runnerIndex].body.previous_location.x = match->players[runnerIndex].body.location.x;
            match->players[runnerIndex].body.previous_location.y = match->players[runnerIndex].body.location.y;
            match->players[runnerIndex].body.previous_location.z = match->players[runnerIndex].body.location.z;
            match->players[runnerIndex].body.orientation.x = -match->players[runnerIndex].body.location.x;
            match->players[runnerIndex].body.orientation.y = 0.0f;
            match->players[runnerIndex].body.orientation.z = -match->players[runnerIndex].body.location.z;
        }

        // set other runners next to the third base.
        for (i = hrcs->pair_index + 1; i < scoreboard->pair_count; i++) {
            int index = scoreboard->teams[battingTeamIndex].batter_runner_indices[1][i];
            if (index != -1) {
                match->players[index].body.location.x =
                    field_positions->third_base_run.x - 2.0f - (i - (hrcs->pair_index + 1)) * 1.5f;
                match->players[index].body.location.y = field_positions->third_base_run.y;
                match->players[index].body.location.z = field_positions->third_base_run.z;
                match->players[index].body.previous_location.x = match->players[index].body.location.x;
                match->players[index].body.previous_location.y = match->players[index].body.location.y;
                match->players[index].body.previous_location.z = match->players[index].body.location.z;
                match->players[index].body.orientation.x = -match->players[index].body.location.x;
                match->players[index].body.orientation.y = 0.0f;
                match->players[index].body.orientation.z = -match->players[index].body.location.z;
            }
        }
    }
}
