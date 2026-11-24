#include "all_contracts.h"
#include "actions/batting_system.h"
#include "body_step.h"
#include "frame_events.h"
#include "player_motion.h"
#include "rules_pure/player_utils.h"
#include "runner_paths.h"
#include "scenario_builder.h"
#include "simulate.h"
#include "state_validator.h"
#include "test_helpers.h"
#include "vector_math.h"

static ScenarioContext* third_base_runner(void)
{
    ScenarioContext* ctx = create_scenario();
    PlayerInfo* p = &ctx->state->match->players[0];
    p->batting.presence = (BattingPresence){PLAYER_STATE_ON_BASE, BASE_THIRD};
    p->body.location = ctx->state->field_positions->third_base_run;
    p->body.previous_location = p->body.location;
    return ctx;
}

int test_third_home_route_visits_flag_before_physical_arrival(void)
{
    ScenarioContext* ctx = third_base_runner();
    MatchSession* m = ctx->state->match;
    const FieldPositions* field = ctx->state->field_positions;
    PlayerInfo* p = &m->players[0];
    int passed_flag = 0, arrivals = 0;
    run_to_next_base(m, field, 0, BASE_THIRD);
    ASSERT_FLOAT_EQ(field->run_left_point.x, p->body.target_location.x, 1e-6f, "the route starts toward the flag");
    ASSERT_FLOAT_EQ(
        field->run_left_point.z, p->body.target_location.z, 1e-6f, "the first leg is not a direct run home"
    );

    // These are physical steps only; reaching home here does not adjudicate a legal run.
    for (int tick = 0; tick < 2000 && p->motion.moving; tick++) {
        player_location_orientation_and_targets(m, field);
        if (!passed_flag && m->players[0].path.waypoint == 1) {
            passed_flag = 1;
            ASSERT(
                vec3_distance_xz(&p->body.location, &field->run_left_point) < TARGET_ACHIEVED_THRESHOLD,
                "the body actually reaches the flag before the route changes"
            );
            ASSERT_EQ(BASE_THIRD, p->batting.presence.base_id, "passing the flag is not arriving home");
            ASSERT_EQ(0, m->players[0].path.arrived_at_base, "the intermediate waypoint raises no arrival");
            ASSERT_FLOAT_EQ(field->home_run_point.x, p->body.target_location.x, 1e-6f, "the second leg targets home");
            ASSERT_FLOAT_EQ(field->home_run_point.z, p->body.target_location.z, 1e-6f, "home is the scoring approach");
        }
        if (m->players[0].path.arrived_at_base) {
            arrivals++;
            ASSERT_EQ(1, passed_flag, "home is reached only after the flag");
            ASSERT_EQ(BASE_HOME_SCORED, p->batting.presence.base_id, "the arrival is a physical home marker");
            ASSERT_EQ(2, m->players[0].path.waypoint, "home completes the second leg");
            ASSERT(
                vec3_distance_xz(&p->body.location, &field->home_run_point) < TARGET_ACHIEVED_THRESHOLD,
                "arrival requires the body to reach home"
            );
        }
        base_runner_movements_on_base_arrivals(m);
        ASSERT_EQ(0, m->players[0].path.arrived_at_base, "each physical arrival is consumed immediately");
    }
    ASSERT_EQ(1, passed_flag, "the complete route crosses its intermediate waypoint");
    ASSERT_EQ(1, arrivals, "there is exactly one physical base arrival");
    ASSERT_EQ(1, m->events.homeward_runs_started, "the second leg does not start another camera cue");
    ASSERT_EQ(0, p->motion.moving, "the runner finishes the walk to its own home place");
    ASSERT_EQ(0, p->motion.running, "the final walk releases the running facing lock");
    ASSERT(
        vec3_distance_xz(&p->body.location, &p->body.home_location) < TARGET_ACHIEVED_THRESHOLD,
        "the full physical journey ends at this player's home place"
    );
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

int test_third_home_retreat_returns_via_flag_to_third(void)
{
    ScenarioContext* ctx = third_base_runner();
    MatchSession* m = ctx->state->match;
    const FieldPositions* field = ctx->state->field_positions;
    PlayerInfo* p = &m->players[0];
    run_to_next_base(m, field, 0, BASE_THIRD);
    for (int tick = 0; tick < 1000 && m->players[0].path.waypoint == 0; tick++)
        player_location_orientation_and_targets(m, field);
    ASSERT_EQ(1, m->players[0].path.waypoint, "first physically pass the flag");
    for (int tick = 0; tick < 10; tick++)
        player_location_orientation_and_targets(m, field);
    ASSERT(
        vec3_distance_xz(&p->body.location, &field->run_left_point) > TARGET_ACHIEVED_THRESHOLD,
        "the recall starts beyond the flag, not already on it"
    );

    run_to_previous_base(m, field, 0, BASE_THIRD);
    ASSERT_EQ(0, m->players[0].path.going_forward, "recall reverses the physical route");
    ASSERT_FLOAT_EQ(field->run_left_point.x, p->body.target_location.x, 1e-6f, "retreat first returns to the flag");
    ASSERT_FLOAT_EQ(field->run_left_point.z, p->body.target_location.z, 1e-6f, "retreat cannot cut directly to third");
    int returned_past_flag = 0;
    for (int tick = 0; tick < 2000 && p->motion.moving; tick++) {
        player_location_orientation_and_targets(m, field);
        if (!returned_past_flag && m->players[0].path.waypoint == 0) {
            returned_past_flag = 1;
            ASSERT(
                vec3_distance_xz(&p->body.location, &field->run_left_point) < TARGET_ACHIEVED_THRESHOLD,
                "the reverse route changes leg at the flag"
            );
            ASSERT_FLOAT_EQ(field->third_base_run.x, p->body.target_location.x, 1e-6f, "the next leg returns to third");
            ASSERT_FLOAT_EQ(
                field->third_base_run.z, p->body.target_location.z, 1e-6f, "third is the retreat destination"
            );
        }
    }
    ASSERT_EQ(1, returned_past_flag, "the reverse journey visits both legs");
    ASSERT_EQ(0, p->motion.moving, "the retreat stops at third");
    ASSERT_EQ(0, p->motion.running, "the completed retreat releases the facing lock");
    ASSERT_EQ(PLAYER_STATE_ON_BASE, p->batting.presence.state, "physical presence returns to on-base");
    ASSERT_EQ(BASE_THIRD, p->batting.presence.base_id, "the source base remains third throughout retreat");
    ASSERT(
        vec3_distance_xz(&p->body.location, &field->third_base_run) < TARGET_ACHIEVED_THRESHOLD,
        "the runner physically returns to third"
    );
    ASSERT_EQ(0, m->events.player_arrived_at_base, "retreat is not a new forward base arrival");
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

int test_terminal_players_complete_all_exit_legs(void)
{
    const PlayerUnitState states[] = {PLAYER_STATE_OUT, PLAYER_STATE_SCORED, PLAYER_STATE_WOUNDED};
    for (int state = 0; state < 3; state++) {
        for (int side = -1; side <= 1; side += 2) {
            ScenarioContext* ctx = create_scenario();
            MatchSession* m = ctx->state->match;
            const FieldPositions* field = ctx->state->field_positions;
            PlayerInfo* p = &m->players[0];
            p->batting.presence = (BattingPresence){states[state], BASE_NONE};
            p->body.location = (Vector3D){side * 10.0f, 0.0f, -25.0f};
            p->motion.running = 1;
            m->players[0].path.waypoint = 2;
            move_player_out(m->players, field, 0);
            Vector3D outside = side < 0 ? field->left_point : field->right_point;
            outside.x += side * 5.0f;
            outside.z += 10.0f;
            ASSERT_EQ(0, m->players[0].path.waypoint, "exit restarts the waypoint sequence");
            ASSERT_EQ(0, p->motion.running, "even a wounded runner exits walking, not still running bases");
            ASSERT_FLOAT_EQ(outside.x, p->body.target_location.x, 1e-6f, "exit chooses the body's side of the field");
            ASSERT_FLOAT_EQ(outside.z, p->body.target_location.z, 1e-6f, "the first leg goes outside the playing area");
            int saw_circle = 0, saw_home = 0;
            for (int tick = 0; tick < 4000 && p->motion.moving; tick++) {
                player_location_orientation_and_targets(m, field);
                if (!saw_circle && m->players[0].path.waypoint == 1) {
                    saw_circle = 1;
                    ASSERT(
                        vec3_distance_xz(&p->body.location, &outside) < TARGET_ACHIEVED_THRESHOLD,
                        "the first transition happens outside the field"
                    );
                    ASSERT_FLOAT_EQ(field->pitch_plate.x, p->body.target_location.x, 1e-6f, "then walk to the circle");
                    ASSERT_FLOAT_EQ(
                        field->pitch_plate.z + HOME_RADIUS, p->body.target_location.z, 1e-6f,
                        "the middle leg reaches the front of the home circle"
                    );
                }
                if (!saw_home && m->players[0].path.waypoint == 2) {
                    saw_home = 1;
                    ASSERT_EQ(1, saw_circle, "exit cannot skip the middle leg");
                    ASSERT_FLOAT_EQ(p->body.home_location.x, p->body.target_location.x, 1e-6f, "last, go to own place");
                    ASSERT_FLOAT_EQ(
                        p->body.home_location.z, p->body.target_location.z, 1e-6f, "not another slot's place"
                    );
                }
            }
            ASSERT_EQ(1, saw_home, "all three exit legs complete");
            ASSERT_EQ(0, p->motion.moving, "the terminal player stops at home");
            ASSERT_EQ(states[state], p->batting.presence.state, "walking off does not rewrite the terminal presence");
            ASSERT(
                vec3_distance_xz(&p->body.location, &p->body.home_location) < TARGET_ACHIEVED_THRESHOLD,
                "the final resting place is reached physically"
            );
            ASSERT_EQ(0, m->events.player_arrived_at_base, "exit waypoints are not base arrivals");
            cleanup_scenario(ctx);
        }
    }
    return TEST_PASSED;
}

int test_simulate_raises_and_consumes_each_runner_arrival(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    const FieldPositions* field = ctx->state->field_positions;
    give_ball_to_pitcher(ctx);
    for (int i = 0; i < 2; i++) {
        PlayerInfo* p = &m->players[i];
        BaseID base = i == 0 ? BASE_HOME : BASE_FIRST;
        Vector3D target = i == 0 ? field->first_base_run : field->second_base_run;
        p->batting.presence = (BattingPresence){PLAYER_STATE_ON_BASE, base};
        p->body.location = target;
        p->body.location.x -= 0.05f;
        run_to_next_base(m, field, i, base);
    }
    clear_frame_events(&m->events);
    MatchSession before = *m;

    player_location_orientation_and_targets(m, field);
    ASSERT_EQ(1, m->events.player_arrived_at_base, "the body substage raises the aggregate event");
    for (int i = 0; i < 2; i++)
        ASSERT_EQ(1, m->players[i].path.arrived_at_base, "both per-player latches exist before consumption");
    base_runner_movements_on_base_arrivals(m);
    for (int i = 0; i < 2; i++)
        ASSERT_EQ(0, m->players[i].path.arrived_at_base, "the consumer drains every arriving runner");

    *m = before;
    simulate(m, field, &ctx->state->play_sound_effect);
    ASSERT_EQ(BASE_FIRST, m->players[0].batting.presence.base_id, "the real SIMULATE advances the first runner");
    ASSERT_EQ(BASE_SECOND, m->players[1].batting.presence.base_id, "the real SIMULATE advances the other runner");
    for (int i = 0; i < 2; i++)
        ASSERT_EQ(0, m->players[i].path.arrived_at_base, "SIMULATE consumes the latches it just raised");
    ASSERT_EQ(1, m->events.player_arrived_at_base, "the aggregate event remains for ADJUDICATE");
    clear_frame_events(&m->events);
    ASSERT_EQ(0, m->events.player_arrived_at_base, "only CLEAR drains the aggregate event");
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

int test_batter_retreat_restores_at_bat_and_validator_rejects_on_base_at_home(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    const FieldPositions* field = ctx->state->field_positions;
    setup_batter_at_home(ctx, 0);
    initialize_referee_from_physical_state(ctx);

    // Batter sets off toward 1st (closing the batting cycle and leaving AT_BAT -> RUNNING),
    // takes a few steps, then retreats back to BASE_HOME.
    run_to_next_base(m, field, 0, BASE_HOME);
    ASSERT_EQ(PLAYER_STATE_RUNNING, m->players[0].batting.presence.state, "leaving home puts batter in RUNNING");
    ASSERT_EQ(BATTING_CYCLE_CLOSED, m->at_bat.cycle, "leaving home closes the batting cycle");
    for (int tick = 0; tick < 15; tick++) {
        player_location_orientation_and_targets(m, field);
    }

    run_to_previous_base(m, field, 0, BASE_HOME);
    ASSERT_EQ(0, m->players[0].path.going_forward, "retreat runs backward toward home");
    for (int tick = 0; tick < 500 && m->players[0].motion.moving; tick++) {
        player_location_orientation_and_targets(m, field);
    }

    ASSERT_EQ(0, m->players[0].motion.moving, "retreating batter stops at the ready spot");
    ASSERT_EQ(BASE_HOME, m->players[0].batting.presence.base_id, "base remains BASE_HOME");
    ASSERT_EQ(
        PLAYER_STATE_AT_BAT, m->players[0].batting.presence.state,
        "returning to BASE_HOME restores AT_BAT, never ON_BASE"
    );
    ASSERT_EQ(0, get_active_batter_index(m), "get_active_batter_index finds the returned batter");
    ASSERT_EQ(BATTING_CYCLE_ARRIVED, m->at_bat.cycle, "prepare_batter re-arms the batting cycle");
    ASSERT_EQ(1, batter_stands_ready(&m->at_bat), "returned batter stands ready for the next pitch");

    // Prove the state validator accepts the restored state and rejects the illegal ON_BASE@HOME state.
    state_validator_set_active(1);
    ASSERT_EQ(1, state_validator_check(ctx->state), "restored AT_BAT@HOME state is valid");

    m->players[0].batting.presence.state = PLAYER_STATE_ON_BASE;
    ASSERT_EQ(0, state_validator_check(ctx->state), "state validator must reject PLAYER_STATE_ON_BASE on BASE_HOME");
    state_validator_set_active(0);

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
