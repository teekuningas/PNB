#include "scenario_builder.h"
#include "rules_state.h"
#include "world.h"
#include "test_helpers.h"
#include "all_contracts.h"

/*
 * Home-run contest (*kotiutuskilpailu*) foul × pair-end contracts.
 *
 * Precedence in referee Phase B is strictly:
 *   END_INNING > NEXT_PAIR (on 3rd-strike foul) > FOUL (on 1st/2nd-strike foul).
 * `foul_state` and `next_pair_state` are never simultaneously active, a 1st/2nd-strike
 * foul never strips safety or ends the pair when the ball returns home during the foul
 * grace period, and once `next_pair_state` is active no post-play run may be awarded.
 */

static ScenarioContext* contest_pair_context(int pair_index, int pair_count, int strikes)
{
    ScenarioContext* ctx = create_scenario();
    ctx->state->rules->scoreboard.period = 4;
    ctx->state->rules->scoreboard.pair_count = pair_count;
    ctx->state->rules->homerun_contest.pair_index = pair_index;
    ctx->state->rules->half_inning.strikes = strikes;

    setup_batter_at_home(ctx, 1);
    place_runner_at_base(ctx, 0, BASE_THIRD, 0.0f);

    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);
    return ctx;
}

int test_foul_blocks_safety_removal_and_contest_pair_end(void)
{
    ScenarioContext* ctx = contest_pair_context(0, 5, 0);

    // 1st-strike foul already detected; batter stepped off home while the ball is returned home.
    ctx->state->rules->referee.foul_state = FOUL_STATE_DETECTED;
    ctx->state->rules->referee.foul_timer = 50;
    ctx->state->match->players[1].batting.presence.state = PLAYER_STATE_RUNNING;
    give_ball_to_pitcher(ctx);

    simulate_frames(ctx, 1);

    ASSERT_EQ(
        BASE_HOME, (int)ctx->state->rules->referee.batting_players[1].current_safety_base,
        "ball returned home during foul grace period must not strip batter's home safety"
    );
    ASSERT_EQ(
        HR_PAIR_STATE_NONE, (int)ctx->state->rules->referee.next_pair_state,
        "1st-strike foul must not end the contest pair"
    );
    ASSERT_EQ(0, ctx->state->rules->homerun_contest.pair_index, "pair_index must not advance on 1st-strike foul");
    ASSERT_EQ(FOUL_STATE_DETECTED, (int)ctx->state->rules->referee.foul_state, "foul state must stay active");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

int test_contest_third_strike_foul_subsumes_foul_into_pair_end(void)
{
    ScenarioContext* ctx = contest_pair_context(0, 5, 2);

    // Ball bounces out of bounds on the 3rd strike.
    ctx->state->match->events.ball_hit_by_bat = 1;
    ctx->state->match->events.ball_hit_ground = 1;
    ctx->state->rules->between_pitch.has_ball_hit_ground = 0;
    ctx->state->rules->between_pitch.bat_outcome = BAT_OUTCOME_HIT;
    ctx->state->match->ball.location.x = 200.0f;

    simulate_frames(ctx, 1);

    ASSERT_EQ(
        PLAYER_STATUS_OUT, (int)ctx->state->rules->referee.batting_players[1].status,
        "3rd-strike foul burns the batter immediately"
    );
    ASSERT_EQ(
        FOUL_STATE_NONE, (int)ctx->state->rules->referee.foul_state,
        "3rd-strike foul in contest is subsumed by pair-end (never enters foul reset)"
    );
    ASSERT_EQ(
        HR_PAIR_STATE_DETECTED, (int)ctx->state->rules->referee.next_pair_state,
        "3rd-strike foul enters HR_PAIR_STATE_DETECTED directly"
    );
    ASSERT_EQ(1, ctx->state->rules->homerun_contest.pair_index, "pair_index advances once on 3rd-strike foul");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

int test_contest_last_pair_third_strike_foul_upgrades_to_inning_end(void)
{
    ScenarioContext* ctx = contest_pair_context(4, 5, 2);

    // Final pair hits a 3rd-strike foul: must upgrade all the way to END_INNING_STATE_DETECTED.
    ctx->state->match->events.ball_hit_by_bat = 1;
    ctx->state->match->events.ball_hit_ground = 1;
    ctx->state->rules->between_pitch.has_ball_hit_ground = 0;
    ctx->state->rules->between_pitch.bat_outcome = BAT_OUTCOME_HIT;
    ctx->state->match->ball.location.x = 200.0f;

    simulate_frames(ctx, 1);

    ASSERT_EQ(FOUL_STATE_NONE, (int)ctx->state->rules->referee.foul_state, "foul state subsumed on last pair");
    ASSERT_EQ(HR_PAIR_STATE_NONE, (int)ctx->state->rules->referee.next_pair_state, "pair state subsumed on last pair");
    ASSERT_EQ(
        END_INNING_STATE_DETECTED, (int)ctx->state->rules->referee.end_of_inning_state,
        "last-pair 3rd-strike foul jumps directly to END_INNING_STATE_DETECTED"
    );
    ASSERT_EQ(5, ctx->state->rules->homerun_contest.pair_index, "pair_index reaches pair_count");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

int test_contest_pair_end_blocks_post_play_runs(void)
{
    ScenarioContext* ctx = contest_pair_context(0, 5, 2);

    // Pair has already ended (e.g. after a 3rd-strike foul subsumed into HR_PAIR_STATE_DETECTED).
    ctx->state->rules->referee.next_pair_state = HR_PAIR_STATE_DETECTED;
    ctx->state->rules->referee.next_pair_timer = 10;
    ctx->state->rules->between_pitch.has_ball_hit_ground = 1;

    // Third-base runner crosses home plate during the pair-end pause.
    ctx->state->match->players[0].batting.presence.base_id = BASE_HOME_SCORED;
    ctx->state->match->players[0].batting.presence.state = PLAYER_STATE_ON_BASE;
    ctx->state->match->events.player_arrived_at_base = 1;

    int runsBefore = ctx->state->rules->scoreboard.teams[0].runs + ctx->state->rules->scoreboard.teams[1].runs;
    simulate_frames(ctx, 1);
    int runsAfter = ctx->state->rules->scoreboard.teams[0].runs + ctx->state->rules->scoreboard.teams[1].runs;

    ASSERT_EQ(runsBefore, runsAfter, "no run may be awarded while HR_PAIR_STATE_DETECTED is active");
    ASSERT_EQ(0, ctx->state->rules->referee.batting_players[0].has_scored, "runner must not be marked scored");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
