#include "scenario_builder.h"
#include "rules_state.h"
#include "intents.h"
#include "world.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "intent_channel.h"
#include "ingest.h"
#include "actualize.h"
#include "referee.h"
#include "game_reset.h"
#include "actions_pure/pitching_physics.h"
#include <string.h>

/**
 * CONTRACT: a base-run command is for the runner holding that base at the top of the tick.
 *
 * The gate asks who holds the base and binds the command to that player; with nobody there it refuses,
 * and the actualizer is never asked to move nobody. The binding is the frame-top reading by
 * construction: holding a base is the referee's safety plus the player's base_id, and nothing between
 * the gate and the base running can change either for a player who already holds one.
 */

static void tick(ScenarioContext* ctx)
{
    {
        IngestedCommands ingested =
            ingest_intents(ctx->state->match, ctx->state->rules, ctx->state->field_positions, &ctx->state->channels);
        actualize(
            ctx->state->match, ctx->state->rules, ctx->state->field_positions, &ingested, &ctx->state->play_sound_effect
        );
    }
}

static void run_command(ScenarioContext* ctx, BaseID base, RunIntent command)
{
    intent_push(
        &ctx->state->channels.batting,
        (IntentMessage){.kind = INTENT_BASE_RUN, .as.base_run = {.base = base, .command = command}}
    );
}

// The batting side reduced to one settled runner, holding first.
static int one_runner_on_first(ScenarioContext* ctx)
{
    MatchSession* m = ctx->state->match;
    RefereeState* referee = &ctx->state->rules->referee;
    initialize_referee_from_physical_state(ctx);
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        m->players[i].batting.presence.state = PLAYER_STATE_IDLE;
        m->players[i].batting.presence.base_id = BASE_NONE;
        referee->batting_players[i].current_safety_base = BASE_NONE;
    }
    const int runner = 3;
    m->players[runner].batting.presence.base_id = BASE_FIRST;
    m->players[runner].batting.presence.state = PLAYER_STATE_ON_BASE;
    m->players[runner].motion.moving = 0;
    referee->batting_players[runner].current_safety_base = BASE_FIRST;
    referee->batting_players[runner].base_at_pitch_start = BASE_FIRST;
    referee->batting_players[runner].status = PLAYER_STATUS_ACTIVE;
    return runner;
}

int test_a_base_run_needs_a_runner(void)
{
    ScenarioContext* ctx = create_scenario();
    one_runner_on_first(ctx);
    MatchSession* m = ctx->state->match;

    MatchSession before;
    memcpy(&before, m, sizeof(MatchSession));
    run_command(ctx, BASE_SECOND, RUN_COMMIT); // nobody holds second
    run_command(ctx, BASE_THIRD, RUN_BACK); // ...or third
    tick(ctx);
    MatchSession refused;
    memcpy(&refused, m, sizeof(MatchSession));

    memcpy(m, &before, sizeof(MatchSession));
    tick(ctx);
    ASSERT(memcmp(&refused, m, sizeof(MatchSession)) == 0, "commands for empty bases change nothing at all");

    // Control: the runner who does hold a base is armed by the same command.
    memcpy(m, &before, sizeof(MatchSession));
    run_command(ctx, BASE_FIRST, RUN_FORWARD);
    tick(ctx);
    ASSERT_EQ(1, m->runner_commitments.armed[BASE_FIRST], "the runner on first is armed to go on the pitch");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// A command for a base that does not exist, or a command that is none of the three, is not refused — it is
// malformed: a broken producer, which fails the frame through the state validator rather than being told no.
int test_a_base_run_for_no_base_is_malformed(void)
{
    ScenarioContext* ctx = create_scenario();
    one_runner_on_first(ctx);
    run_command(ctx, (BaseID)BASE_COUNT, RUN_FORWARD);
    tick(ctx);
    ASSERT_EQ(1, ctx->state->channels.batting.malformed, "a base outside the field is flagged as malformed");
    cleanup_scenario(ctx);

    // And a command that is not one of the three: read as RUN_BACK it would quietly send a runner home.
    ctx = create_scenario();
    one_runner_on_first(ctx);
    run_command(ctx, BASE_FIRST, (RunIntent)(RUN_BACK + 1));
    tick(ctx);
    ASSERT_EQ(1, ctx->state->channels.batting.malformed, "a command outside RunIntent is flagged as malformed");
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// §32: before the turn's first pitch leaves the lukkari's hand, the batter may not commit to leave home
// (refused at the INGEST gate with RULE_BASE_RUN_BATTER_NEEDS_FIRST_PITCH), while arming (RUN_FORWARD)
// and disarming (RUN_BACK) stay allowed. Once batter_may_leave_home == 1, RUN_COMMIT succeeds.
int test_batter_cannot_commit_before_first_pitch_but_may_arm(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    GameRulesState* r = ctx->state->rules;
    setup_batter_at_home(ctx, 0);
    initialize_referee_from_physical_state(ctx);
    r->half_inning.batter_may_leave_home = 0;

    // 1. RUN_COMMIT before the first pitch is refused at the gate — World untouched.
    MatchSession before;
    memcpy(&before, m, sizeof(MatchSession));
    run_command(ctx, BASE_HOME, RUN_COMMIT);
    IngestedCommands ingested = ingest_intents(m, r, ctx->state->field_positions, &ctx->state->channels);
    ASSERT_EQ(RUN_NONE, (int)ingested.base_run[BASE_HOME], "gate refuses RUN_COMMIT from BASE_HOME before first pitch");
    actualize(m, r, ctx->state->field_positions, &ingested, &ctx->state->play_sound_effect);
    ASSERT(memcmp(&before, m, sizeof(MatchSession)) == 0, "refused home commit changes nothing in MatchSession");

    // 2. RUN_FORWARD while settled at bat still arms the batter without leaving home.
    run_command(ctx, BASE_HOME, RUN_FORWARD);
    tick(ctx);
    ASSERT_EQ(1, m->runner_commitments.armed[BASE_HOME], "RUN_FORWARD arms batter before first pitch");
    ASSERT_EQ(PLAYER_STATE_AT_BAT, (int)m->players[0].batting.presence.state, "arming keeps batter AT_BAT");
    ASSERT_EQ(0, m->players[0].path.going_forward, "arming never moves batter off home");

    // 3. RUN_BACK while settled disarms cleanly.
    run_command(ctx, BASE_HOME, RUN_BACK);
    tick(ctx);
    ASSERT_EQ(0, m->runner_commitments.armed[BASE_HOME], "RUN_BACK disarms batter before first pitch");

    // 4. Control: once §32 grants the right, RUN_COMMIT passes the gate and sends the batter toward 1st.
    r->half_inning.batter_may_leave_home = 1;
    run_command(ctx, BASE_HOME, RUN_COMMIT);
    ingested = ingest_intents(m, r, ctx->state->field_positions, &ctx->state->channels);
    ASSERT_EQ(RUN_COMMIT, (int)ingested.base_run[BASE_HOME], "gate admits RUN_COMMIT once batter_may_leave_home == 1");
    actualize(m, r, ctx->state->field_positions, &ingested, &ctx->state->play_sound_effect);
    ASSERT_EQ(PLAYER_STATE_RUNNING, (int)m->players[0].batting.presence.state, "batter leaves home in RUNNING");
    ASSERT_EQ(1, m->players[0].path.going_forward, "batter runs forward toward first");
    ASSERT_EQ(BATTING_CYCLE_CLOSED, (int)m->at_bat.cycle, "leaving home closes the batting cycle");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// Law 3 (frame-top fairness): on the exact tick a pitch is released in ACTUALIZE, batter_may_leave_home
// is still 0 at the top of the tick (INGEST), so a RUN_COMMIT on that frame is refused. On the next
// tick — once ADJUDICATE has promoted events.pitch_released into batter_may_leave_home == 1 — it passes.
int test_batter_commit_on_pitch_release_tick_is_refused_at_frame_top(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    GameRulesState* r = ctx->state->rules;
    setup_batter_at_home(ctx, 0);
    give_ball_to_pitcher(ctx);
    initialize_referee_from_physical_state(ctx);
    r->half_inning.batter_may_leave_home = 0;

    // Position the pitcher's windup one tick before release.
    const float power = 0.5f;
    m->catching_action = (CatchingAction){
        .kind = CATCHING_ACTION_PITCHING,
        .timer = pitch_windup_total_frames(power) - 1,
        .as.pitch = {.power = power, .aimed = 1, .direction = 0.0f},
    };

    // Frame T: RUN_COMMIT arrives on the exact release tick.
    run_command(ctx, BASE_HOME, RUN_COMMIT);
    IngestedCommands ingested = ingest_intents(m, r, ctx->state->field_positions, &ctx->state->channels);
    ASSERT_EQ(RUN_NONE, (int)ingested.base_run[BASE_HOME], "gate reads frame-top batter_may_leave_home == 0");
    actualize(m, r, ctx->state->field_positions, &ingested, &ctx->state->play_sound_effect);
    ASSERT_EQ(1, m->events.pitch_released, "actualize releases the pitch on frame T");
    ASSERT_EQ(PLAYER_STATE_AT_BAT, (int)m->players[0].batting.presence.state, "batter does not break on release frame");

    // ADJUDICATE promotes pitch_released into legal batter_may_leave_home = 1.
    update_referee(
        m, ctx->state->field_positions, &r->referee, &r->half_inning, &r->between_pitch, &r->scoreboard,
        &r->homerun_contest, &ctx->state->announcement
    );
    ASSERT_EQ(1, r->half_inning.batter_may_leave_home, "referee grants §32 right on pitch_released");

    // Frame T+1: RUN_COMMIT now passes and sends the batter toward 1st.
    run_command(ctx, BASE_HOME, RUN_COMMIT);
    tick(ctx);
    ASSERT_EQ(PLAYER_STATE_RUNNING, (int)m->players[0].batting.presence.state, "batter may commit on frame T+1");
    ASSERT_EQ(1, m->players[0].path.going_forward, "batter runs forward on frame T+1");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// Legal lifetime of HalfInningState.batter_may_leave_home:
//   - starts 0; set to 1 by referee on pitch_released
//   - preserved across 1st/2nd-strike foul resets (normal play and contest)
//   - cleared to 0 on 3rd-strike foul burn, on batter_entered, and on inning reset
int test_batter_leave_home_lifecycle_through_referee(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    GameRulesState* r = ctx->state->rules;
    setup_batter_at_home(ctx, 0);
    initialize_referee_from_physical_state(ctx);
    ASSERT_EQ(0, r->half_inning.batter_may_leave_home, "starts 0 before first pitch");

    // Pitch released → 1.
    m->events.pitch_released = 1;
    update_referee(
        m, ctx->state->field_positions, &r->referee, &r->half_inning, &r->between_pitch, &r->scoreboard,
        &r->homerun_contest, &ctx->state->announcement
    );
    m->events.pitch_released = 0;
    ASSERT_EQ(1, r->half_inning.batter_may_leave_home, "pitch_released grants batter_may_leave_home");

    // 1st-strike foul reset leaves batter_may_leave_home == 1 intact even after reset_for_foul_play wipes AtBat.
    r->referee.foul_state = FOUL_STATE_DETECTED;
    r->referee.foul_timer = 1000;
    update_referee(
        m, ctx->state->field_positions, &r->referee, &r->half_inning, &r->between_pitch, &r->scoreboard,
        &r->homerun_contest, &ctx->state->announcement
    );
    ASSERT_EQ(FOUL_STATE_RESETTING, (int)r->referee.foul_state, "foul transitions to RESETTING");
    reset_for_foul_play(m, ctx->state->field_positions, r);
    referee_finalize(m, &r->referee, &r->between_pitch);
    ASSERT_EQ(1, r->half_inning.batter_may_leave_home, "1st-strike foul reset preserves batter_may_leave_home");

    // New batter entering clears batter_may_leave_home to 0.
    m->events.batter_entered = 1;
    update_referee(
        m, ctx->state->field_positions, &r->referee, &r->half_inning, &r->between_pitch, &r->scoreboard,
        &r->homerun_contest, &ctx->state->announcement
    );
    m->events.batter_entered = 0;
    ASSERT_EQ(0, r->half_inning.batter_may_leave_home, "batter_entered clears batter_may_leave_home");

    // 3rd-strike foul burn clears batter_may_leave_home to 0 immediately on detection.
    r->half_inning.strikes = 2;
    m->events.pitch_released = 1;
    update_referee(
        m, ctx->state->field_positions, &r->referee, &r->half_inning, &r->between_pitch, &r->scoreboard,
        &r->homerun_contest, &ctx->state->announcement
    );
    m->events.pitch_released = 0;
    ASSERT_EQ(1, r->half_inning.batter_may_leave_home, "pitch_released grants right before 3rd-strike foul");

    r->between_pitch.bat_outcome = BAT_OUTCOME_HIT;
    r->between_pitch.catch_has_been_made = 0;
    r->between_pitch.has_ball_hit_ground = 0;
    m->ball.location = (Vector3D){200.0f, 0.0f, 200.0f};
    m->events.ball_hit_ground = 1;
    update_referee(
        m, ctx->state->field_positions, &r->referee, &r->half_inning, &r->between_pitch, &r->scoreboard,
        &r->homerun_contest, &ctx->state->announcement
    );
    m->events.ball_hit_ground = 0;
    ASSERT_EQ(0, r->half_inning.batter_may_leave_home, "3rd-strike foul burn clears batter_may_leave_home");

    // Inning reset clears batter_may_leave_home to 0.
    r->half_inning.batter_may_leave_home = 1;
    referee_reset_for_new_inning(&r->referee, &r->half_inning, &r->between_pitch);
    ASSERT_EQ(0, r->half_inning.batter_may_leave_home, "referee_reset_for_new_inning clears batter_may_leave_home");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
