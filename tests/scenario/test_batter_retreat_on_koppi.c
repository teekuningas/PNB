#include "scenario_builder.h"
#include "world.h"
#include "rules_state.h"
#include "actions/batting_system.h"
#include "actions/pitching_system.h"
#include "rules_pure/player_utils.h"
#include "state_validator.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include <stdio.h>

// Regression for the playtest deadlock (batter-bug.log): a batter on strike 1 runs partway
// toward 1st on a fly ball, retreats safely back to BASE_HOME before the koppi is caught, and
// must return to PLAYER_STATE_AT_BAT with readiness restored so play resumes instead of landing
// in PLAYER_STATE_ON_BASE@HOME while offers.batter_seat deadlocks against home control.
int test_batter_retreats_to_home_on_koppi(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    GameRulesState* r = ctx->state->rules;

    setup_batter_at_home(ctx, 0);
    r->half_inning.strikes = 1;

    // Place a fielder at 3rd base to catch the fly ball and move pitcher aside until after the catch.
    int fielderIdx = 15;
    Vector3D fielderLoc = ctx->state->field_positions->third_base;
    m->players[fielderIdx].body.location = fielderLoc;
    m->players[fielderIdx].body.home_location = fielderLoc;
    move_pitcher_away(ctx);

    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);
    r->half_inning.strikes = 1;

    // Batter hits a high fly ball (150 frames in air) and sets off toward 1st.
    hit_fly_ball_to_location_with_time(ctx, ctx->state->field_positions->pitch_plate, fielderLoc, 150.0f);
    trigger_player_run_to_next_base(ctx, 0, BASE_HOME);

    state_validator_set_active(1);
    for (int f = 0; f < 25; f++) {
        simulate_frames(ctx, 1);
        ASSERT_EQ(1, state_validator_check(ctx->state), "state must remain valid while batter runs out");
    }

    // Batter sees the fly ball and retreats safely back to BASE_HOME before the catch.
    trigger_player_run_to_previous_base(ctx, 0, BASE_HOME);

    int catchSeen = 0;
    for (int f = 0; f < 300; f++) {
        simulate_frames(ctx, 1);
        ASSERT_EQ(1, state_validator_check(ctx->state), "state must remain valid through retreat and koppi");
        if (r->between_pitch.catch_has_been_made == 1) {
            catchSeen = 1;
        }
    }

    ASSERT_EQ(1, catchSeen, "setup: fly ball must be caught on the fly (koppi)");
    ASSERT_EQ(BASE_HOME, m->players[0].batting.presence.base_id, "batter remains at BASE_HOME");
    ASSERT_EQ(
        PLAYER_STATE_AT_BAT, m->players[0].batting.presence.state,
        "batter returning safely to home must be AT_BAT, not ON_BASE"
    );
    ASSERT_EQ(0, get_active_batter_index(m), "returned batter is the active batter");
    ASSERT_EQ(0, m->offers.batter_seat, "batter seat offer must not stay open with batter at home");
    ASSERT_EQ(1, batter_stands_ready(&m->at_bat), "returned batter must stand ready for the next pitch");

    // Return the ball to the pitcher at home and confirm the next pitch may legally begin.
    give_ball_to_pitcher(ctx);
    m->players[m->fielding_posts.baseman[0]].fielding.near_home = 1;
    ASSERT_EQ(
        1, pitch_may_begin(m, r->half_inning.strikes).allowed,
        "pitcher must be able to deliver the next pitch after a safe home retreat"
    );

    state_validator_set_active(0);
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// Second half of the same bug: if offers.batter_seat ALREADY opened while the batter was off home
// (because the ball hit the ground while the batter was in PLAYER_STATE_RUNNING), retreating safely
// back to BASE_HOME before the ball reaches home must close the stale offer and restore readiness.
int test_batter_retreat_closes_stale_seat_offer(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    GameRulesState* r = ctx->state->rules;

    setup_batter_at_home(ctx, 0);
    move_pitcher_away(ctx);

    // Move all fielders away from the outfield landing point so the ball bounces fair cleanly.
    for (int i = PLAYERS_IN_TEAM + JOKER_COUNT; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        m->players[i].body.location = (Vector3D){40.0f, 0.0f, -80.0f};
        m->players[i].body.home_location = (Vector3D){40.0f, 0.0f, -80.0f};
        m->players[i].fielding.near_home = 1;
    }

    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);
    r->half_inning.strikes = 1;

    // Fair ball drops in the center field while the batter runs partway toward 1st.
    r->between_pitch.bat_outcome = BAT_OUTCOME_HIT;
    place_ball_over_location(ctx, (Vector3D){0.0f, 0.0f, -35.0f});
    trigger_player_run_to_next_base(ctx, 0, BASE_HOME);

    state_validator_set_active(1);
    int offerOpened = 0;
    for (int f = 0; f < 100 && !offerOpened; f++) {
        simulate_frames(ctx, 1);
        ASSERT_EQ(1, state_validator_check(ctx->state), "state must remain valid while ball bounces");
        if (m->offers.batter_seat == 1) {
            offerOpened = 1;
        }
    }
    ASSERT_EQ(1, offerOpened, "setup: offers.batter_seat must open while batter is off home after ball lands");
    ASSERT_EQ(PLAYER_STATE_RUNNING, m->players[0].batting.presence.state, "setup: batter is still running off home");

    // Now the batter retreats safely back to BASE_HOME while still holding home safety.
    trigger_player_run_to_previous_base(ctx, 0, BASE_HOME);
    for (int f = 0; f < 200 && m->players[0].motion.moving; f++) {
        simulate_frames(ctx, 1);
        ASSERT_EQ(1, state_validator_check(ctx->state), "state must remain valid during retreat");
    }
    // Tick one extra frame so update_batting opens the cycle and consolidation runs after arrival.
    simulate_frames(ctx, 2);
    ASSERT_EQ(1, state_validator_check(ctx->state), "state must remain valid after retreat settles");

    ASSERT_EQ(PLAYER_STATE_AT_BAT, m->players[0].batting.presence.state, "retreat restores PLAYER_STATE_AT_BAT");
    ASSERT_EQ(0, get_active_batter_index(m), "returned batter is active");
    ASSERT_EQ(0, m->offers.batter_seat, "stale offers.batter_seat must close once the batter returns to BASE_HOME");
    ASSERT_EQ(1, batter_stands_ready(&m->at_bat), "returned batter stands ready");

    give_ball_to_pitcher(ctx);
    m->players[m->fielding_posts.baseman[0]].fielding.near_home = 1;
    ASSERT_EQ(
        1, pitch_may_begin(m, r->half_inning.strikes).allowed,
        "pitcher may pitch once the batter returns and the stale seat offer closes"
    );

    state_validator_set_active(0);
    cleanup_scenario(ctx);
    return TEST_PASSED;
}
