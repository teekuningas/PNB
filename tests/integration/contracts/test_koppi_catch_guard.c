#include "all_contracts.h"
#include "actions/batting_system.h"
#include "actions_pure/swing_geometry.h"
#include "ball_catch.h"
#include "ball_flight.h"
#include "field.h"
#include "fielder_behaviours.h"
#include "intent_channel.h"
#include "intents.h"
#include "scenario_builder.h"
#include "simulate.h"
#include "test_helpers.h"
#include "world.h"

// 1. Bat contact clears the pitcher's released_by and arms the physical catch-reaction delay.
int test_batted_ball_clears_released_by_and_arms_catch_delay(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    int pitcher = m->fielding_posts.baseman[BASE_HOME];

    setup_batter_at_home(ctx, 0);
    m->players[0].attributes.power = 3;
    prepare_batter(ctx->state->match);
    initialize_referee_from_physical_state(ctx);

    // Give the pitcher the ball at home and deliver a real pitch via intents.
    give_ball_to_pitcher(ctx);
    m->players[pitcher].fielding.near_home = 1;
    intent_push(&ctx->state->channels.catching, (IntentMessage){.kind = INTENT_PITCH_POWER, .as.pitch_power = {0.5f}});
    intent_push(&ctx->state->channels.catching, (IntentMessage){.kind = INTENT_PITCH_AIM, .as.pitch_aim = {0.0f}});

    // Advance until pitch is released, then declare a level swing.
    int released = 0;
    for (int f = 0; f < 200; f++) {
        simulate_frames(ctx, 1);
        if (m->ball.pitch_in_flight) {
            released = 1;
            break;
        }
    }
    ASSERT_EQ(1, released, "setup: pitch must release into flight");
    ASSERT_EQ(pitcher, m->ball.released_by, "setup: pitched ball names the lukkari as released_by");

    intent_push(&ctx->state->channels.batting, (IntentMessage){.kind = INTENT_SWING_POWER, .as.swing_power = {0.75f}});
    intent_push(
        &ctx->state->channels.batting,
        (IntentMessage){.kind = INTENT_SWING_VERTICAL, .as.swing_vertical = {SWING_VERTICAL_FOCAL}}
    );

    int hit = 0;
    for (int f = 0; f < 200; f++) {
        simulate_frames(ctx, 1);
        if (ctx->state->rules->between_pitch.bat_outcome == BAT_OUTCOME_HIT) {
            hit = 1;
            break;
        }
    }
    ASSERT_EQ(1, hit, "setup: batter must make contact");
    ASSERT_EQ(-1, m->ball.released_by, "bat contact must clear released_by so pitcher self-catch guard does not leak");
    ASSERT_EQ(0, m->ball.pitch_in_flight, "bat contact ends pitch_in_flight");
    ASSERT_EQ(BASE_NONE, m->ball.thrown_to, "batted ball is not a throw");
    // The contact tick's own SIMULATE already decremented catch_delay_ticks once.
    ASSERT_EQ(
        BATTED_BALL_CATCH_DELAY_TICKS - 1, m->ball.catch_delay_ticks,
        "bat contact arms catch_delay_ticks (decremented once on the contact tick)"
    );

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 2. Physical catch-reaction delay blocks immediate off-the-bat clips on every frame of the delay,
//    then opens for BOTH the lukkari (kotipesäkoppi) and close infielders (< 15 units from the lukkari).
int test_catch_delay_blocks_immediate_clip_then_allows_lukkari_and_close_infield_catch(void)
{
    // Part A: Lukkari inside reach of a newly batted ball cannot catch during the delay, then catches at tick N.
    {
        ScenarioContext* ctx = create_scenario();
        MatchSession* m = ctx->state->match;
        int pitcher = m->fielding_posts.baseman[BASE_HOME];

        // Move every other fielder far away so only the lukkari can catch.
        for (int i = PLAYERS_IN_TEAM + JOKER_COUNT; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            if (i == pitcher) continue;
            m->players[i].body.location = (Vector3D){40.0f, BALL_HEIGHT_WITH_PLAYER, -70.0f};
            m->players[i].body.home_location = m->players[i].body.location;
            m->players[i].fielding.near_home = 1;
        }
        m->players[pitcher].body.location = (Vector3D){2.0f, BALL_HEIGHT_WITH_PLAYER, 0.0f};
        m->players[pitcher].body.home_location = m->players[pitcher].body.location;
        m->players[pitcher].fielding.near_home = 1;

        // Hang a freshly batted ball right inside the lukkari's catch sphere with the full reaction delay.
        m->ball.location = (Vector3D){2.0f, 1.0f, 0.0f};
        m->ball.previous_location = m->ball.location;
        launch_batted_ball(&m->ball, (Vector3D){0.0f, 0.0f, 0.0f}, BATTED_BALL_CATCH_DELAY_TICKS);
        m->ball.moving = 0; // keep stationary in air to isolate the delay clock
        m->chase_replan = CHASE_REPLAN_NONE;

        for (int t = 0; t < BATTED_BALL_CATCH_DELAY_TICKS; t++) {
            simulate(m, ctx->state->field_positions, &ctx->state->play_sound_effect);
            ASSERT_EQ(-1, m->ball.holder, "lukkari must not catch during catch_delay_ticks");
            ASSERT_EQ(0, m->events.catch_made, "no catch event during catch_delay_ticks");
        }
        ASSERT_EQ(0, m->ball.catch_delay_ticks, "delay must expire after BATTED_BALL_CATCH_DELAY_TICKS ticks");

        simulate(m, ctx->state->field_positions, &ctx->state->play_sound_effect);
        ASSERT_EQ(pitcher, m->ball.holder, "lukkari catches the batted ball as soon as catch_delay_ticks expires");
        ASSERT_EQ(1, m->events.catch_made, "catch_made event raised on lukkari's kotipesäkoppi");

        cleanup_scenario(ctx);
    }

    // Part B: A close infielder within 15 units of the lukkari can catch a batted fly ball cleanly.
    {
        ScenarioContext* ctx = create_scenario();
        MatchSession* m = ctx->state->match;
        int pitcher = m->fielding_posts.baseman[BASE_HOME];
        int infielder = m->fielding_posts.cover[BASE_HOME]; // bottom_right_catcher

        for (int i = PLAYERS_IN_TEAM + JOKER_COUNT; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
            m->players[i].body.location = (Vector3D){40.0f, BALL_HEIGHT_WITH_PLAYER, -70.0f};
            m->players[i].body.home_location = m->players[i].body.location;
            m->players[i].fielding.near_home = 1;
        }
        m->players[pitcher].body.location = (Vector3D){1.5f, BALL_HEIGHT_WITH_PLAYER, 0.0f};
        m->players[pitcher].body.home_location = m->players[pitcher].body.location;
        // 8 units from pitcher — strictly inside PLAYER_TOO_CLOSE_TO_CATCH_LIMIT (15.0f).
        m->players[infielder].body.location = (Vector3D){1.5f, BALL_HEIGHT_WITH_PLAYER, -8.0f};
        m->players[infielder].body.home_location = m->players[infielder].body.location;

        m->ball.location = (Vector3D){1.5f, 1.0f, -8.0f};
        m->ball.previous_location = m->ball.location;
        launch_batted_ball(&m->ball, (Vector3D){0.0f, 0.0f, 0.0f}, 0);
        m->ball.moving = 0;
        m->chase_replan = CHASE_REPLAN_NONE;

        simulate(m, ctx->state->field_positions, &ctx->state->play_sound_effect);
        ASSERT_EQ(infielder, m->ball.holder, "close infielder (<15u from lukkari) must catch batted fly ball");
        ASSERT_EQ(1, m->events.catch_made, "close infielder catch raises catch_made");

        cleanup_scenario(ctx);
    }

    return TEST_PASSED;
}

// 3. §25(4): during pitch_in_flight the lukkari stays at their evaded spot even when uncontrolled,
//    and cover[0] (bottom_right_catcher) stays at his post instead of marching toward home plate;
//    once the lukkari genuinely leaves HOME_RADIUS after the pitch, cover[0] activates.
int test_lukkari_stays_evaded_and_cover0_stays_put_during_pitch_in_flight(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    int pitcher = m->fielding_posts.baseman[BASE_HOME];
    int cover0 = m->fielding_posts.cover[BASE_HOME];

    setup_batter_at_home(ctx, 0);
    prepare_batter(m);
    initialize_referee_from_physical_state(ctx);
    give_ball_to_pitcher(ctx);
    m->players[pitcher].fielding.near_home = 1;

    Vector3D cover0_home = m->players[cover0].body.home_location;

    // Throw a high pitch so pitch_in_flight lasts > 100 frames.
    intent_push(&ctx->state->channels.catching, (IntentMessage){.kind = INTENT_PITCH_POWER, .as.pitch_power = {0.8f}});
    intent_push(&ctx->state->channels.catching, (IntentMessage){.kind = INTENT_PITCH_AIM, .as.pitch_aim = {0.0f}});

    int released = 0;
    for (int f = 0; f < 200; f++) {
        simulate_frames(ctx, 1);
        if (m->ball.pitch_in_flight) {
            released = 1;
            break;
        }
    }
    ASSERT_EQ(1, released, "setup: pitch must release");

    // Let the lukkari finish walking to the evaded spot (~35 frames).
    for (int f = 0; f < 40; f++) {
        simulate_frames(ctx, 1);
        ASSERT_EQ(1, m->ball.pitch_in_flight, "setup: pitch still in flight while lukkari evades");
    }
    ASSERT_EQ(0, m->players[pitcher].motion.moving, "lukkari should have arrived at evaded spot");
    float evaded_x = m->players[pitcher].body.location.x;
    ASSERT(evaded_x > m->players[pitcher].body.home_location.x + 1.5f, "lukkari stepped aside from the plate");

    // Drop control off the lukkari so move_idling_players_to_home_location is the only guard tested.
    m->catching.controlled_fielder = -1;

    for (int f = 0; f < 30 && m->ball.pitch_in_flight; f++) {
        simulate_frames(ctx, 1);
        ASSERT_FLOAT_EQ(
            evaded_x, m->players[pitcher].body.location.x, 1e-4f,
            "lukkari must remain at evaded spot throughout pitch_in_flight (§25(4))"
        );
        ASSERT_EQ(
            REPLACEMENT_IDLE, m->players[cover0].fielding.replacement,
            "cover[0] must not be pulled off his post during pitch_in_flight"
        );
        ASSERT_FLOAT_EQ(
            cover0_home.x, m->players[cover0].body.location.x, 1e-4f, "cover[0] stays at his formation post x"
        );
        ASSERT_FLOAT_EQ(
            cover0_home.z, m->players[cover0].body.location.z, 1e-4f, "cover[0] stays at his formation post z"
        );
    }

    // Now end pitch_in_flight and verify:
    // (a) evaded spot inside HOME_RADIUS still does not pull cover[0];
    // (b) genuinely leaving HOME_RADIUS DOES pull cover[0].
    m->ball.pitch_in_flight = 0;
    basemen_replacements(m);
    ASSERT_EQ(
        REPLACEMENT_IDLE, m->players[cover0].fielding.replacement,
        "lukkari stepping aside inside HOME_RADIUS must not pull cover[0]"
    );

    m->players[pitcher].body.location.z = -HOME_RADIUS - 3.0f;
    m->players[pitcher].fielding.near_home = 0;
    basemen_replacements(m);
    ASSERT_EQ(
        REPLACEMENT_ACTIVE, m->players[cover0].fielding.replacement,
        "cover[0] activates when lukkari genuinely leaves HOME_RADIUS"
    );
    ASSERT_EQ(BASE_HOME, m->players[cover0].fielding.replacement_base, "cover[0] targets BASE_HOME");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
