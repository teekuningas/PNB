#include "scenario_builder.h"
#include "world.h"
#include "rules_state.h"
#include "intents.h"
#include "intent_channel.h"
#include "actions/pitching_system.h"
#include "actions/batting_system.h"
#include "rules_pure/player_utils.h"
#include "state_validator.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include <stdio.h>

static void push_throw(ScenarioContext* ctx, BaseID target, float power)
{
    intent_push(
        &ctx->state->channels.catching,
        (IntentMessage){.kind = INTENT_THROW_TARGET, .as.throw_target = {.base = target}}
    );
    intent_push(
        &ctx->state->channels.catching, (IntentMessage){.kind = INTENT_THROW_POWER, .as.throw_power = {.power = power}}
    );
}

static void push_pitch(ScenarioContext* ctx, float power, float direction)
{
    intent_push(
        &ctx->state->channels.catching, (IntentMessage){.kind = INTENT_PITCH_POWER, .as.pitch_power = {.power = power}}
    );
    intent_push(
        &ctx->state->channels.catching,
        (IntentMessage){.kind = INTENT_PITCH_AIM, .as.pitch_aim = {.direction = direction}}
    );
}

// Tick until the ball is held by `expected_holder` (or budget runs out).
static int tick_until_holder(ScenarioContext* ctx, int expected_holder, int budget)
{
    for (int f = 0; f < budget; f++) {
        simulate_frames(ctx, 1);
        if (!state_validator_check(ctx->state)) return 0;
        if (ctx->state->match->ball.holder == expected_holder) return 1;
    }
    return 0;
}

/**
 * §24 Scenario 1: Strict Home -> Field Base -> Home round-trip.
 *
 * Batter 0 hits a fair ball and reaches 1st safely (§18(2)). An outfielder throws to 1st, and 1B
 * throws to home (Field Base -> Home, with NO prior Home visit!). Batter 1 enters and stands ready.
 * Pitching MUST be refused (RULE_PITCH_NEEDS_PITCH_RIGHTS) until the lukkari throws Home -> 1st -> Home.
 */
int test_pitch_rights_requires_home_base_home_round_trip(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    GameRulesState* r = ctx->state->rules;
    const FieldPositions* fp = ctx->state->field_positions;

    setup_batter_at_home(ctx, 0);
    initialize_referee_from_physical_state(ctx);
    r->scoreboard.teams[get_batting_team_index(&r->scoreboard)].batter_order_index = 1;
    snapshot_pitch_start_state(ctx);

    state_validator_set_active(1);

    // Batter 0 hits a fair ground ball to deep outfield (fielder 19) and runs safely to 1st.
    r->between_pitch.bat_outcome = BAT_OUTCOME_HIT;
    r->between_pitch.has_ball_hit_ground = 1;
    m->ball.holder = 19;
    m->ball.location = m->players[19].body.location;
    m->catching.controlled_fielder = 19;
    trigger_player_run_to_next_base(ctx, 0, BASE_HOME);

    int reached_first = 0;
    for (int f = 0; f < 400; f++) {
        simulate_frames(ctx, 1);
        ASSERT_EQ(1, state_validator_check(ctx->state), "valid state while Batter 0 runs to 1st");
        if (r->referee.batting_players[0].current_safety_base == BASE_FIRST) {
            reached_first = 1;
            break;
        }
    }
    ASSERT_EQ(1, reached_first, "setup: Batter 0 safely reached 1st base (§18(2))");
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_HOME, (int)r->half_inning.pitch_rights, "reaching 1st revokes pitch rights to NEED_HOME"
    );

    // Outfielder 19 throws to 1st base (1B = 13) -> still NEED_HOME (never visited Home first!).
    push_throw(ctx, BASE_FIRST, 0.8f);
    ASSERT(tick_until_holder(ctx, 13, 300), "1B catches throw from outfield");
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_HOME, (int)r->half_inning.pitch_rights, "Field Base BEFORE Home must NOT advance NEED_HOME"
    );

    // 1B throws to Home (lukkari = 12) -> advances NEED_HOME -> NEED_FIELD_BASE!
    push_throw(ctx, BASE_HOME, 0.8f);
    ASSERT(tick_until_holder(ctx, 12, 300), "lukkari catches throw from 1st");
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_FIELD_BASE, (int)r->half_inning.pitch_rights,
        "ball arriving at Home advances NEED_HOME -> NEED_FIELD_BASE"
    );

    // Seat Batter 1 and let them walk to the plate.
    ASSERT_EQ(1, m->offers.batter_seat, "batter seat offer is open for Batter 1");
    intent_push(
        &ctx->state->channels.batting, (IntentMessage){.kind = INTENT_SELECT_BATTER, .as.select_batter = {.index = 1}}
    );
    int batter_ready = 0;
    for (int f = 0; f < 400; f++) {
        simulate_frames(ctx, 1);
        ASSERT_EQ(1, state_validator_check(ctx->state), "valid state while Batter 1 walks up");
        if (batter_stands_ready(&m->at_bat) && m->players[12].fielding.near_home == 1) {
            batter_ready = 1;
            break;
        }
    }
    ASSERT_EQ(1, batter_ready, "setup: Batter 1 stands ready at the plate");

    // Attempt to pitch WITHOUT Home -> Field Base -> Home: MUST be refused with RULE_PITCH_NEEDS_PITCH_RIGHTS!
    ASSERT_EQ(
        RULE_PITCH_NEEDS_PITCH_RIGHTS,
        (int)pitch_may_begin(m, r->half_inning.strikes, r->half_inning.pitch_rights).denied_by,
        "pitch_may_begin refuses with RULE_PITCH_NEEDS_PITCH_RIGHTS"
    );
    push_pitch(ctx, 0.5f, 0.0f);
    simulate_frames(ctx, 5);
    ASSERT_EQ(CATCHING_ACTION_NONE, (int)m->catching_action.kind, "gate refuses pitch before round-trip");
    ASSERT_EQ(12, m->ball.holder, "lukkari still holds the ball");

    // Now lukkari throws Home -> 1st (NEED_RETURN_HOME) -> Home (GRANTED).
    push_throw(ctx, BASE_FIRST, 0.8f);
    ASSERT(tick_until_holder(ctx, 13, 300), "1B catches pitch-rights toss from home");
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_RETURN_HOME, (int)r->half_inning.pitch_rights,
        "ball at 1st advances NEED_FIELD_BASE -> NEED_RETURN_HOME"
    );

    // Ensure 1B returns the ball cleanly back to lukkari at home.
    (void)fp;
    push_throw(ctx, BASE_HOME, 0.8f);
    ASSERT(tick_until_holder(ctx, 12, 300), "lukkari catches return throw from 1st");
    ASSERT_EQ(PITCH_RIGHTS_GRANTED, (int)r->half_inning.pitch_rights, "return to Home grants §24 pitch rights");

    // Wait for lukkari to settle on plate if needed, then pitch — MUST succeed!
    for (int f = 0; f < 100 && m->players[12].fielding.near_home != 1; f++) {
        simulate_frames(ctx, 1);
    }
    push_pitch(ctx, 0.5f, 0.0f);
    int pitch_released = 0;
    for (int f = 0; f < 120; f++) {
        simulate_frames(ctx, 1);
        ASSERT_EQ(1, state_validator_check(ctx->state), "valid state through granted pitch");
        if (m->ball.pitch_in_flight) {
            pitch_released = 1;
            break;
        }
    }
    ASSERT_EQ(1, pitch_released, "after Home -> 1st -> Home, pitch succeeds cleanly");

    state_validator_set_active(0);
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

/**
 * §24 Scenario 2: Bunt putout at 1st (Home -> 1st -> Home) earns pitch rights BEFORE Batter 1 enters.
 *
 * Batter 0 runs off home; lukkari fields the ball at home (§18(5) strips home safety and advances
 * NEED_HOME -> NEED_FIELD_BASE on the same frame), throws to 1st to burn Batter 0 (NEED_RETURN_HOME),
 * and 1B returns the ball to lukkari (GRANTED). Batter 1 is seated AFTER the round-trip completes
 * and pitch rights MUST remain GRANTED without requiring a second toss!
 */
int test_pitch_rights_earned_during_bunt_putout_survives_next_batter(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    GameRulesState* r = ctx->state->rules;

    setup_batter_at_home(ctx, 0);
    initialize_referee_from_physical_state(ctx);
    r->scoreboard.teams[get_batting_team_index(&r->scoreboard)].batter_order_index = 1;
    snapshot_pitch_start_state(ctx);

    state_validator_set_active(1);

    // Batter 0 runs off home on a fair bunt; lukkari picks up the ball inside home area.
    r->between_pitch.bat_outcome = BAT_OUTCOME_HIT;
    r->between_pitch.has_ball_hit_ground = 1;
    trigger_player_run_to_next_base(ctx, 0, BASE_HOME);
    simulate_frames(ctx, 10);

    give_ball_to_pitcher(ctx);
    simulate_frames(ctx, 1);
    ASSERT_EQ(1, state_validator_check(ctx->state), "valid state when lukkari holds bunt at home");
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_FIELD_BASE, (int)r->half_inning.pitch_rights,
        "§18(5): lukkari holding ball at home while batter is off home -> NEED_FIELD_BASE"
    );

    // Lukkari throws to 1st (burns Batter 0, NEED_RETURN_HOME), 1B throws back to lukkari (GRANTED).
    push_throw(ctx, BASE_FIRST, 0.9f);
    ASSERT(tick_until_holder(ctx, 13, 300), "1B catches bunt throw");
    ASSERT_EQ(PITCH_RIGHTS_NEED_RETURN_HOME, (int)r->half_inning.pitch_rights, "1st base -> NEED_RETURN_HOME");

    push_throw(ctx, BASE_HOME, 0.9f);
    ASSERT(tick_until_holder(ctx, 12, 300), "lukkari catches return throw");
    ASSERT_EQ(PITCH_RIGHTS_GRANTED, (int)r->half_inning.pitch_rights, "return home -> GRANTED");

    // Seat Batter 1 AFTER pitch rights were already earned: must stay GRANTED and allow pitching!
    ASSERT_EQ(1, m->offers.batter_seat, "batter seat offer is open");
    intent_push(
        &ctx->state->channels.batting, (IntentMessage){.kind = INTENT_SELECT_BATTER, .as.select_batter = {.index = 1}}
    );
    int batter_ready = 0;
    for (int f = 0; f < 400; f++) {
        simulate_frames(ctx, 1);
        ASSERT_EQ(1, state_validator_check(ctx->state), "valid state while Batter 1 walks up");
        if (batter_stands_ready(&m->at_bat) && m->players[12].fielding.near_home == 1) {
            batter_ready = 1;
            break;
        }
    }
    ASSERT_EQ(1, batter_ready, "Batter 1 stands ready");
    ASSERT_EQ(
        PITCH_RIGHTS_GRANTED, (int)r->half_inning.pitch_rights,
        "seating Batter 1 after completed Home -> 1st -> Home must keep GRANTED"
    );
    ASSERT_EQ(
        1, pitch_may_begin(m, r->half_inning.strikes, r->half_inning.pitch_rights).allowed,
        "pitching to Batter 1 is immediately allowed"
    );

    state_validator_set_active(0);
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

/**
 * §24 Scenario 3: Foul resets.
 *
 * (A) 1st-strike foul where the batter actually reaches 1st base BEFORE the foul lands out of
 *     bounds: pitch rights temporarily revoke on arrival at 1st, then MUST restore to GRANTED on
 *     the foul reset because the same batter returns to home.
 * (B) 3rd-strike foul: the batter burns (§18(6)), and after the foul reset tosses the ball to the
 *     lukkari at home, pitch rights MUST be NEED_FIELD_BASE (not GRANTED) for the next batter.
 */
int test_pitch_rights_preserved_on_early_foul_and_revoked_on_third_strike_foul(void)
{
    // --- Part A: 1st-strike foul with batter reaching 1st before foul bounce ---
    {
        ScenarioContext* ctx = create_scenario();
        MatchSession* m = ctx->state->match;
        GameRulesState* r = ctx->state->rules;

        setup_batter_at_home(ctx, 0);
        initialize_referee_from_physical_state(ctx);
        snapshot_pitch_start_state(ctx);
        move_pitcher_away(ctx);

        // High foul fly ball (300 frames flight so Batter 0 reaches 1st ~frame 237 while it is still airborne).
        Vector3D home = ctx->state->field_positions->pitch_plate;
        Vector3D foulTarget = {200.0f, 0.0f, 200.0f};
        hit_fly_ball_to_location_with_time(ctx, home, foulTarget, 300.0f);
        m->events.ball_hit_by_bat = 1;
        trigger_player_run_to_next_base(ctx, 0, BASE_HOME);

        int reached_first_mid_foul = 0;
        int saw_foul = 0;
        int reset_done = 0;
        for (int f = 0; f < 800; f++) {
            simulate_frames(ctx, 1);
            if (!saw_foul && r->referee.batting_players[0].current_safety_base == BASE_FIRST) {
                reached_first_mid_foul = 1;
            }
            if (r->referee.foul_state == FOUL_STATE_DETECTED) {
                saw_foul = 1;
            } else if (saw_foul && r->referee.foul_state == FOUL_STATE_NONE) {
                reset_done = 1;
                break;
            }
        }

        ASSERT_EQ(1, reached_first_mid_foul, "setup: Batter 0 reached 1st before foul ball landed");
        ASSERT_EQ(1, saw_foul, "foul detected");
        ASSERT_EQ(1, reset_done, "foul reset completed");
        ASSERT_EQ(
            PITCH_RIGHTS_GRANTED, (int)r->half_inning.pitch_rights,
            "1st-strike foul reset restores pitch rights to GRANTED even if batter touched 1st mid-play"
        );
        cleanup_scenario(ctx);
    }

    // --- Part B: 3rd-strike foul burns the batter and requires fresh pitch rights ---
    {
        ScenarioContext* ctx = create_scenario();
        MatchSession* m = ctx->state->match;
        GameRulesState* r = ctx->state->rules;

        setup_batter_at_home(ctx, 0);
        r->half_inning.strikes = 2;
        initialize_referee_from_physical_state(ctx);
        snapshot_pitch_start_state(ctx);
        move_pitcher_away(ctx);

        Vector3D home = ctx->state->field_positions->pitch_plate;
        Vector3D foulTarget = {60.0f, 0.0f, -20.0f}; // inside fence, outside right foul line
        hit_fly_ball_to_location_with_time(ctx, home, foulTarget, 80.0f);
        m->events.ball_hit_by_bat = 1;

        int saw_foul = 0;
        int reset_done = 0;
        for (int f = 0; f < 600; f++) {
            simulate_frames(ctx, 1);
            if (r->referee.foul_state == FOUL_STATE_DETECTED) {
                saw_foul = 1;
            } else if (saw_foul && r->referee.foul_state == FOUL_STATE_NONE) {
                reset_done = 1;
                break;
            }
        }
        ASSERT_EQ(1, saw_foul, "3rd-strike foul detected");
        ASSERT_EQ(1, reset_done, "3rd-strike foul reset completed");
        ASSERT_EQ(1, r->half_inning.outs, "3rd-strike foul recorded an out");
        ASSERT_EQ(
            PITCH_RIGHTS_NEED_HOME, (int)r->half_inning.pitch_rights,
            "3rd-strike foul reset leaves pitch rights at NEED_HOME until lukkari catches the reset ball"
        );

        // Let the lukkari catch the reset toss at home -> NEED_FIELD_BASE (NOT GRANTED).
        ASSERT(tick_until_holder(ctx, 12, 200), "lukkari catches reset ball at home");
        ASSERT_EQ(
            PITCH_RIGHTS_NEED_FIELD_BASE, (int)r->half_inning.pitch_rights,
            "after 3rd-strike foul reset catch at home, lukkari must still throw to a field base"
        );
        cleanup_scenario(ctx);
    }

    return TEST_PASSED;
}
