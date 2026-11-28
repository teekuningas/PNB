#include "actions/pitching_system.h"
#include "actions_pure/pitching_physics.h"
#include "field.h"
#include "rules_state.h"
#include "intents.h"
#include "world.h"
#include "actions/batting_system.h" // a pitch waits for a batter standing ready
#include "rules_pure/rules_strikes.h"
#include "player_motion.h"
#include "runner_paths.h"
#include "ball_flight.h"
#include "vector_math.h"
#include "base_control.h"

// May a pitch begin in the world as it stands? Asked by the INGEST gate of a declared power, against the
// world at the top of the tick, and answered with the rule that refuses it:
//   - nothing else is being done with the ball — no pitch already winding, no throw gathering
//   - §18(1): the batter has not already spent his three correct pitches. A batter who has is
//     permanently a runner — "ei ole enää lyöntivuorossa" — so there is nobody to pitch to. Stated
//     here rather than left to the side effect of the forced run, which held only for period < 4.
//   - the pitcher at the plate holds the ball, and the previous pitch has been judged
//   - a batter stands ready, and no free walk is waiting to be answered
//   - the pitcher is at the pitching plate
//   - §24: the lukkari holds pitch rights (Home → Field Base → Home since the previous batter
//     permanently became a runner; always granted at inning start and in the home-run contest)
Permission pitch_may_begin(const MatchSession* match, int correct_pitches_received, PitchRightsState pitch_rights)
{
    const int pitcher = match->fielding_posts.baseman[0];
    if (match->catching_action.kind != CATCHING_ACTION_NONE) {
        return (Permission){0, RULE_CATCHING_ACTION_IN_PROGRESS};
    }
    if (batter_has_become_runner_permanently(correct_pitches_received)) {
        return (Permission){0, RULE_PITCH_BATTER_IS_A_RUNNER};
    }
    if (pitcher == -1 || match->ball.holder != pitcher) {
        return (Permission){0, RULE_PITCH_NEEDS_BALL_AT_HOME};
    }
    if (match->ball.pitch_in_flight) {
        return (Permission){0, RULE_PITCH_PREVIOUS_UNRESOLVED};
    }
    if (!batter_stands_ready(&match->at_bat)) {
        return (Permission){0, RULE_PITCH_NEEDS_A_READY_BATTER};
    }
    if (match->offers.free_walk != 0) {
        return (Permission){0, RULE_PITCH_NOT_DURING_FREE_WALK_OFFER};
    }
    if (match->players[pitcher].fielding.near_home != 1) {
        return (Permission){0, RULE_PITCHER_OFF_THE_PLATE};
    }
    if (pitch_rights != PITCH_RIGHTS_GRANTED) {
        return (Permission){0, RULE_PITCH_NEEDS_PITCH_RIGHTS};
    }
    return (Permission){1, RULE_NONE};
}

int pitch_is_being_delivered(const MatchSession* match)
{
    return match->catching_action.kind == CATCHING_ACTION_PITCHING || match->ball.pitch_in_flight;
}

// Begin the engine-owned windup for a toss of `power`: set up the pitcher and the ball, start the
// deterministic clock. This is pure timing setup — the animation model is set downstream for the
// renderer, but nothing here (or later) reads animation to drive timing.
static void begin_windup(MatchSession* match, float power)
{
    int pitcher = match->ball.holder;

    if (match->players[pitcher].motion.moving == 1) {
        stop_movement(match->players, pitcher);
    }
    record_player_activity(&match->players[pitcher].events, PLAYER_ACTIVITY_PITCH_BEGUN, true);
    // force the pitcher exactly onto the pitching spot (a windup may begin from slightly off)
    match->players[pitcher].body.location.x = match->players[pitcher].body.home_location.x;
    match->players[pitcher].body.location.z = match->players[pitcher].body.home_location.z;
    match->players[pitcher].motion.sync_previous_location = 1;
    // face the plate
    match->players[pitcher].body.orientation.x = -1.0f;
    match->players[pitcher].body.orientation.z = 0.0f;
    // the ball rises from the centre of the plate
    vec3_set_xz(&(match->ball.location), 0.0f, 0.0f);

    match->catching_action =
        (CatchingAction){.kind = CATCHING_ACTION_PITCHING, .timer = 0, .as.pitch = {.power = power}};
}

// Release the real pitch — the outcome is a pure function of the DECLARED values (never a live meter):
// the ball leaves with velocity pitch_velocity_from_aim(power, direction).
static void release_pitch(MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions)
{
    int pitcher = match->ball.holder;
    const PitchWindup* pitch = &match->catching_action.as.pitch;
    Vector3D v = pitch_velocity_from_aim(pitch->power, pitch->direction);
    Vector3D target;
    int i;

    launch_pitched_ball(&(match->ball), v, pitcher);

    // nudge the pitcher aside as the ball leaves
    target.x = match->players[pitcher].body.location.x + PITCHER_MOVE_AWAY_OFFSET;
    target.z = match->players[pitcher].body.location.z;
    move_to_target(match->players, pitcher, &target);

    match->events.pitch_released = 1;
    record_player_activity(&match->players[pitcher].events, PLAYER_ACTIVITY_PITCH_RELEASED, true);

    // commit any base runners that armed to break on the pitch
    for (i = 1; i < BASE_COUNT; i++) {
        if (match->runner_commitments.armed[i] == 1) {
            int index = get_base_controller(match, referee, (BaseID)i);
            match->runner_commitments.armed[i] = 0;
            if (index != -1) {
                run_to_next_base(match, fieldPositions, index, (BaseID)i);
            }
        }
    }

    match->catching_action = (CatchingAction){0};
}

// Valesyöttö (a fake): the windup ran out and nobody aimed it. No real pitch — the pitcher keeps the
// ball, which returns to their position; no ball/strike is counted.
static void fake_pitch(MatchSession* match)
{
    int pitcher = match->ball.holder;

    match->ball.pitch_in_flight = 0;
    if (pitcher != -1) {
        match->ball.location.x = match->players[pitcher].body.location.x;
        match->ball.location.z = match->players[pitcher].body.location.z;
        record_player_activity(&match->players[pitcher].events, PLAYER_ACTIVITY_PITCH_FAKED, false);
    }
    match->catching_action = (CatchingAction){0};
}

// The pitch actualizer — the single engine entry point. A declared power begins the windup (the gate has
// already established that it may); an aim, whenever it arrives, is held on the windup until release; and
// the engine's clock decides the rest: aimed when the windup ends → a real pitch, unaimed → valesyöttö.
// Timing is the master; the animation only follows.
void update_pitch_actualization(
    MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions, const PitchValues* declared
)
{
    CatchingAction* action = &match->catching_action;

    // The gate has settled what these may do: a power only when nothing else is in progress, an aim only
    // while a pitch is winding up or begins this tick.
    if (declared->power_declared) {
        begin_windup(match, declared->power);
    }
    if (declared->aim_declared) {
        action->as.pitch.aimed = 1;
        action->as.pitch.direction = declared->direction;
    }
    if (declared->power_declared) return; // never begin and resolve in the same frame — the windup is at least one tick

    if (action->kind == CATCHING_ACTION_PITCHING) {
        action->timer++;
        if (action->timer >= pitch_windup_total_frames(action->as.pitch.power)) {
            if (action->as.pitch.aimed) {
                release_pitch(match, referee, fieldPositions);
            } else {
                fake_pitch(match);
            }
        }
    }
}
