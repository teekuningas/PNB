#include "scenario_builder.h"
#include "field.h"
#include "rules_state.h"
#include "intents.h"
#include "world.h"
#include "ai/ai_memory.h"
#include "ai/catching_ai.h"
#include "intent_channel.h"
#include "ingest.h"
#include "actualize.h"
#include "referee.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include <string.h>

static void tick_ingest_actualize(ScenarioContext* ctx)
{
    IngestedCommands ingested =
        ingest_intents(ctx->state->match, ctx->state->rules, ctx->state->field_positions, &ctx->state->channels);
    actualize(
        ctx->state->match, ctx->state->rules, ctx->state->field_positions, &ingested, &ctx->state->play_sound_effect
    );
}

static void adjudicate_one_frame(ScenarioContext* ctx)
{
    StateInfo* st = ctx->state;
    update_referee(
        st->match, st->field_positions, &st->rules->referee, &st->rules->half_inning, &st->rules->between_pitch,
        &st->rules->scoreboard, &st->rules->homerun_contest, &st->announcement
    );
    st->match->events = (GameEvents){0};
}

static void place_ball_held_at(ScenarioContext* ctx, int holder, Vector3D loc)
{
    ctx->state->match->ball.holder = holder;
    ctx->state->match->ball.location = loc;
    ctx->state->match->players[holder].body.location = loc;
}

// 1. INGEST gate refuses INTENT_PITCH_POWER when §24 pitch rights are not GRANTED, leaving World untouched.
int test_pitch_rights_gate_refuses_without_rights(void)
{
    const PitchRightsState missing[3] = {
        PITCH_RIGHTS_NEED_HOME, PITCH_RIGHTS_NEED_FIELD_BASE, PITCH_RIGHTS_NEED_RETURN_HOME
    };

    for (int i = 0; i < 3; i++) {
        ScenarioContext* ctx = create_scenario();
        setup_pitcher_ready(ctx);
        ctx->state->rules->half_inning.pitch_rights = missing[i];

        MatchSession before;
        memcpy(&before, ctx->state->match, sizeof(MatchSession));

        intent_push(
            &ctx->state->channels.catching, (IntentMessage){.kind = INTENT_PITCH_POWER, .as.pitch_power = {0.5f}}
        );
        intent_push(&ctx->state->channels.catching, (IntentMessage){.kind = INTENT_PITCH_AIM, .as.pitch_aim = {0.0f}});
        tick_ingest_actualize(ctx);

        ASSERT_EQ(
            CATCHING_ACTION_NONE, (int)ctx->state->match->catching_action.kind,
            "§24: gate refuses pitch when pitch_rights != GRANTED"
        );
        ASSERT(
            memcmp(&before, ctx->state->match, sizeof(MatchSession)) == 0, "refused pitch leaves MatchSession intact"
        );
        cleanup_scenario(ctx);
    }

    // Control: once GRANTED, the identical pitch intent begins a windup.
    ScenarioContext* ctx = create_scenario();
    setup_pitcher_ready(ctx);
    ctx->state->rules->half_inning.pitch_rights = PITCH_RIGHTS_GRANTED;
    intent_push(&ctx->state->channels.catching, (IntentMessage){.kind = INTENT_PITCH_POWER, .as.pitch_power = {0.5f}});
    intent_push(&ctx->state->channels.catching, (IntentMessage){.kind = INTENT_PITCH_AIM, .as.pitch_aim = {0.0f}});
    tick_ingest_actualize(ctx);
    ASSERT_EQ(
        CATCHING_ACTION_PITCHING, (int)ctx->state->match->catching_action.kind,
        "control: GRANTED pitch rights admits pitch windup"
    );
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 2. Referee §24 lifecycle: starts GRANTED, revokes once on §18, enforces strict Home -> Base -> Home,
//    survives batter_entered, restores across 1st/2nd-strike foul resets, revokes on 3rd-strike foul,
//    and stays unconditionally GRANTED in the home-run contest (§8).
int test_referee_pitch_rights_lifecycle(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    GameRulesState* r = ctx->state->rules;
    const FieldPositions* fp = ctx->state->field_positions;

    ASSERT_EQ(PITCH_RIGHTS_GRANTED, (int)r->half_inning.pitch_rights, "half-inning starts GRANTED");

    // Seat Batter 0 at home; strike 1, strike 2, ball 1, and field-runner walk keep pitch_rights GRANTED.
    setup_batter_at_home(ctx, 0);
    m->events.batter_entered = 1;
    adjudicate_one_frame(ctx);
    ASSERT_EQ(PITCH_RIGHTS_GRANTED, (int)r->half_inning.pitch_rights, "Batter 1 entering keeps GRANTED");
    ASSERT_EQ(1, r->half_inning.batter_in_turn, "Batter 1 activates batter_in_turn");

    // Strike 1 and strike 2 keep pitch_rights GRANTED.
    m->events.ball_missed_by_bat = 1;
    adjudicate_one_frame(ctx);
    m->events.ball_missed_by_bat = 1;
    adjudicate_one_frame(ctx);
    ASSERT_EQ(2, r->half_inning.strikes, "two strikes counted");
    ASSERT_EQ(PITCH_RIGHTS_GRANTED, (int)r->half_inning.pitch_rights, "two strikes still keep GRANTED");

    // Field-runner free walk (from 1st -> 2nd) does NOT revoke pitch rights for the continuing batter.
    place_runner_at_base(ctx, 1, BASE_FIRST, 0.0f);
    r->referee.batting_players[1].current_safety_base = BASE_FIRST;
    m->free_walk = (FreeWalkEntitlement){.runner = 1, .from = BASE_FIRST};
    m->events.free_walk_accepted = 1;
    adjudicate_one_frame(ctx);
    ASSERT_EQ(PITCH_RIGHTS_GRANTED, (int)r->half_inning.pitch_rights, "field-runner free walk preserves GRANTED");

    // §18(1): 3rd strike while ball is loose revokes GRANTED -> NEED_HOME.
    m->ball.holder = -1;
    m->events.ball_missed_by_bat = 1;
    adjudicate_one_frame(ctx);
    ASSERT_EQ(3, r->half_inning.strikes, "3rd strike recorded");
    ASSERT_EQ(PITCH_RIGHTS_NEED_HOME, (int)r->half_inning.pitch_rights, "§18(1): 3rd strike revokes to NEED_HOME");
    ASSERT_EQ(0, r->half_inning.batter_in_turn, "batter_in_turn cleared on §18");

    // Strict Home -> Base -> Home: ball at 1st BEFORE Home does NOT advance NEED_HOME!
    place_ball_held_at(ctx, 13, fp->first_base);
    adjudicate_one_frame(ctx);
    ASSERT_EQ(PITCH_RIGHTS_NEED_HOME, (int)r->half_inning.pitch_rights, "1st base before Home does NOT advance");

    // Ball arrives at Home -> NEED_FIELD_BASE.
    place_ball_held_at(ctx, 12, fp->pitcher);
    adjudicate_one_frame(ctx);
    ASSERT_EQ(PITCH_RIGHTS_NEED_FIELD_BASE, (int)r->half_inning.pitch_rights, "Home advances to NEED_FIELD_BASE");

    // Batter 2 entering MID-ROUND-TRIP preserves NEED_FIELD_BASE (does not regress to NEED_HOME or jump to GRANTED).
    m->players[0].batting.presence.base_id = BASE_FIRST;
    m->players[0].batting.presence.state = PLAYER_STATE_ON_BASE;
    r->referee.batting_players[0].current_safety_base = BASE_FIRST;
    setup_batter_at_home(ctx, 2);
    m->events.batter_entered = 1;
    adjudicate_one_frame(ctx);
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_FIELD_BASE, (int)r->half_inning.pitch_rights, "Batter 2 entry preserves NEED_FIELD_BASE"
    );

    // Ball visits 2nd base -> NEED_RETURN_HOME.
    place_ball_held_at(ctx, 14, fp->second_base);
    adjudicate_one_frame(ctx);
    ASSERT_EQ(PITCH_RIGHTS_NEED_RETURN_HOME, (int)r->half_inning.pitch_rights, "2nd base advances to NEED_RETURN_HOME");

    // Ball returns to Home -> GRANTED!
    place_ball_held_at(ctx, 12, fp->pitcher);
    adjudicate_one_frame(ctx);
    ASSERT_EQ(PITCH_RIGHTS_GRANTED, (int)r->half_inning.pitch_rights, "return to Home grants pitch rights");

    // §18(3): Batter 2 accepts free walk from BASE_HOME while lukkari already holds ball at Home ->
    // revokes to NEED_HOME and immediately advances to NEED_FIELD_BASE on the same tick!
    m->free_walk = (FreeWalkEntitlement){.runner = 2, .from = BASE_HOME};
    m->events.free_walk_accepted = 1;
    adjudicate_one_frame(ctx);
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_FIELD_BASE, (int)r->half_inning.pitch_rights,
        "§18(3) free walk from home with ball at home advances immediately to NEED_FIELD_BASE"
    );

    // Complete round-trip BEFORE Batter 3 enters: Batter 3 entering afterward keeps GRANTED!
    place_ball_held_at(ctx, 15, fp->third_base);
    adjudicate_one_frame(ctx);
    ASSERT_EQ(PITCH_RIGHTS_NEED_RETURN_HOME, (int)r->half_inning.pitch_rights, "3rd base advances to NEED_RETURN_HOME");
    place_ball_held_at(ctx, 12, fp->pitcher);
    adjudicate_one_frame(ctx);
    ASSERT_EQ(PITCH_RIGHTS_GRANTED, (int)r->half_inning.pitch_rights, "return home grants before Batter 3 enters");

    setup_batter_at_home(ctx, 3);
    m->events.batter_entered = 1;
    adjudicate_one_frame(ctx);
    ASSERT_EQ(
        PITCH_RIGHTS_GRANTED, (int)r->half_inning.pitch_rights,
        "Batter 3 entering after completed round-trip keeps GRANTED"
    );

    // §8 Home-run contest exemption: period >= 4 keeps pitch_rights GRANTED even after 3rd strike.
    r->scoreboard.period = 4;
    r->half_inning.strikes = 2;
    m->ball.holder = -1;
    m->events.ball_missed_by_bat = 1;
    adjudicate_one_frame(ctx);
    ASSERT_EQ(
        PITCH_RIGHTS_GRANTED, (int)r->half_inning.pitch_rights,
        "§8: home-run contest stays GRANTED even after batter spends 3rd strike"
    );

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 3. AI throw-permission guard: when a non-baseman carrier stands inside THROW_TO_BASE_DISTANCE of
//    the target base while the baseman holds the post, update_catching_ai must NOT spam a refused
//    INTENT_THROW_TARGET (it backs out first), and must emit the throw once outside the radius.
int test_ai_throw_guard_never_emits_too_close_throw(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    GameRulesState* r = ctx->state->rules;
    const FieldPositions* fp = ctx->state->field_positions;
    AIControllerState ai = {0};

    // Non-baseman fielder 16 carries the ball 0.3u away from 1st base (< THROW_TO_BASE_DISTANCE = 1.0f),
    // while 1B (13) is standing on 1st base (near_home == 1) and pitch_rights == NEED_FIELD_BASE so
    // the AI targets BASE_FIRST.
    const int carrier = 16;
    m->ball.holder = carrier;
    m->catching.controlled_fielder = carrier;
    m->players[13].fielding.near_home = 1;
    m->players[carrier].body.location = (Vector3D){fp->first_base.x + 0.3f, BALL_HEIGHT_WITH_PLAYER, fp->first_base.z};
    m->ball.location = m->players[carrier].body.location;
    r->half_inning.pitch_rights = PITCH_RIGHTS_NEED_FIELD_BASE;

    IntentChannel ch = {0};
    update_catching_ai(m, r, fp, &ai, &ch);

    int saw_throw = 0;
    int saw_move = 0;
    for (int i = 0; i < ch.count; i++) {
        if (ch.message[i].kind == INTENT_THROW_TARGET) saw_throw = 1;
        if (ch.message[i].kind == INTENT_MOVE_TARGET) saw_move = 1;
    }
    ASSERT_EQ(0, saw_throw, "carrier inside THROW_TO_BASE_DISTANCE must not emit a refused INTENT_THROW_TARGET");
    ASSERT_EQ(1, saw_move, "carrier inside THROW_TO_BASE_DISTANCE still declares MOVE_TARGET to back out");

    // Control: once the carrier steps outside THROW_TO_BASE_DISTANCE, the throw IS emitted.
    m->players[carrier].body.location = (Vector3D){fp->first_base.x + 2.5f, BALL_HEIGHT_WITH_PLAYER, fp->first_base.z};
    m->ball.location = m->players[carrier].body.location;
    ch = (IntentChannel){0};
    update_catching_ai(m, r, fp, &ai, &ch);

    saw_throw = 0;
    for (int i = 0; i < ch.count; i++) {
        if (ch.message[i].kind == INTENT_THROW_TARGET && ch.message[i].as.throw_target.base == BASE_FIRST) {
            saw_throw = 1;
        }
    }
    ASSERT_EQ(1, saw_throw, "control: carrier outside THROW_TO_BASE_DISTANCE emits INTENT_THROW_TARGET to 1st");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
