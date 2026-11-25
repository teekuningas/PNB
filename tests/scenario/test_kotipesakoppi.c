#include "all_scenarios.h"
#include "ball_catch.h"
#include "rules_pure/base_control.h"
#include "scenario_builder.h"
#include "state_validator.h"
#include "test_helpers.h"
#include "world.h"

// §30 / §36: a pop-up near home plate caught on the fly by the lukkari (kotipesäkoppi) is a legal
// wounding catch — off-base runners are wounded, and ball_is_home holds immediately.
int test_kotipesakoppi_wounds_off_base_runner(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    GameRulesState* r = ctx->state->rules;
    int pitcher = m->fielding_posts.baseman[BASE_HOME];

    // Runner 0 on 1st breaking for 2nd; batter 1 at home.
    place_runner_at_base(ctx, 0, BASE_FIRST, 0.25f);
    setup_batter_at_home(ctx, 1);

    // Move every other fielder far away so only the lukkari can catch the home-plate pop-up.
    for (int i = PLAYERS_IN_TEAM + JOKER_COUNT; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        if (i == pitcher) continue;
        m->players[i].body.location = (Vector3D){40.0f, BALL_HEIGHT_WITH_PLAYER, -75.0f};
        m->players[i].body.home_location = m->players[i].body.location;
        m->players[i].fielding.near_home = 1;
    }

    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);
    // Simulate real pitch release having set released_by = pitcher before bat contact clears it.
    m->ball.released_by = pitcher;

    trigger_player_run_to_next_base(ctx, 0, BASE_FIRST);
    // High soft pop-up (85 frames hang time, peaks at y ≈ 4.16 above reach) landing at the lukkari's post.
    hit_fly_ball_to_location_with_time(
        ctx, ctx->state->field_positions->pitch_plate, m->players[pitcher].body.location, 85.0f
    );
    m->ball.catch_delay_ticks = BATTED_BALL_CATCH_DELAY_TICKS;

    state_validator_set_active(1);
    int lukkariCaught = 0;
    int catchFrame = -1;
    int ballWasHomeAtCatch = 0;
    for (int f = 0; f < 500; f++) {
        simulate_frames(ctx, 1);
        ASSERT_EQ(1, state_validator_check(ctx->state), "state must remain valid through kotipesäkoppi");
        if (!lukkariCaught && r->between_pitch.catch_has_been_made == 1 && m->ball.holder == pitcher) {
            lukkariCaught = 1;
            catchFrame = f;
            ballWasHomeAtCatch = ball_is_home(m, ctx->state->field_positions);
        }
        if (m->players[0].batting.presence.state == PLAYER_STATE_WOUNDED) break;
    }

    ASSERT_EQ(1, lukkariCaught, "lukkari must catch the home-plate pop-up on the fly (kotipesäkoppi)");
    ASSERT(catchFrame >= BATTED_BALL_CATCH_DELAY_TICKS, "pop-up is caught on descent, never during reaction delay");
    ASSERT_EQ(1, ballWasHomeAtCatch, "kotipesäkoppi catch occurs with ball at home base");
    ASSERT_EQ(
        PLAYER_STATE_WOUNDED, m->players[0].batting.presence.state, "off-base runner must be wounded on kotipesäkoppi"
    );
    ASSERT_EQ(0, r->half_inning.outs, "kotipesäkoppi wounds the runner without recording an out");

    state_validator_set_active(0);
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// §30 / §36: a close infielder standing within 15 units of the lukkari catches a short batted fly
// ball (previously blocked by PLAYER_TOO_CLOSE_TO_CATCH_LIMIT leaking from pitcher's released_by)
// and wounds an off-base runner.
int test_close_infield_koppi_within_15_units_of_lukkari(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    GameRulesState* r = ctx->state->rules;
    int pitcher = m->fielding_posts.baseman[BASE_HOME];
    int infielder = m->fielding_posts.cover[BASE_HOME];

    place_runner_at_base(ctx, 0, BASE_FIRST, 0.25f);
    setup_batter_at_home(ctx, 1);

    // Move all other fielders away; place infielder 10 units from the lukkari (< 15u throw guard).
    for (int i = PLAYERS_IN_TEAM + JOKER_COUNT; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        if (i == pitcher || i == infielder) continue;
        m->players[i].body.location = (Vector3D){40.0f, BALL_HEIGHT_WITH_PLAYER, -75.0f};
        m->players[i].body.home_location = m->players[i].body.location;
        m->players[i].fielding.near_home = 1;
    }
    Vector3D closeSpot = {m->players[pitcher].body.location.x, BALL_HEIGHT_WITH_PLAYER, -10.0f};
    m->players[infielder].body.location = closeSpot;
    m->players[infielder].body.home_location = closeSpot;
    m->players[infielder].fielding.near_home = 1;

    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);
    m->ball.released_by = pitcher;

    trigger_player_run_to_next_base(ctx, 0, BASE_FIRST);
    hit_fly_ball_to_location_with_time(ctx, ctx->state->field_positions->pitch_plate, closeSpot, 65.0f);
    m->ball.catch_delay_ticks = BATTED_BALL_CATCH_DELAY_TICKS;

    state_validator_set_active(1);
    int infielderCaught = 0;
    for (int f = 0; f < 500; f++) {
        simulate_frames(ctx, 1);
        ASSERT_EQ(1, state_validator_check(ctx->state), "state must remain valid through close-infield koppi");
        if (!infielderCaught && r->between_pitch.catch_has_been_made == 1 && m->ball.holder == infielder) {
            infielderCaught = 1;
        }
        if (m->players[0].batting.presence.state == PLAYER_STATE_WOUNDED) break;
    }

    ASSERT_EQ(1, infielderCaught, "close infielder (<15u from lukkari) must catch the fly ball");
    ASSERT_EQ(
        PLAYER_STATE_WOUNDED, m->players[0].batting.presence.state,
        "off-base runner must be wounded on close-infield koppi"
    );

    state_validator_set_active(0);
    cleanup_scenario(ctx);
    return TEST_PASSED;
}
