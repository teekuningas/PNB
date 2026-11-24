#include "scenario_builder.h"
#include "rules_state.h"
#include "intents.h"
#include "intent_channel.h"
#include "world.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include "rules_pure/player_utils.h"

/*
 * Home-run contest (*kotiutuskilpailu*) foul handling scenarios:
 *   1. A 1st-strike foul whose ball returns home during the foul grace period must NOT strip
 *      safety or end the pair — it must restore the same pair with 1 strike.
 *   2. A 3rd-strike foul must burn the batter and transition directly through the pair-end
 *      machine without ever re-seating the burnt batter at the plate or awarding a run.
 */

static ScenarioContext* setup_contest_scenario(int initial_strikes)
{
    ScenarioContext* ctx = create_scenario();
    Scoreboard* sb = &ctx->state->rules->scoreboard;
    sb->period = 4;
    sb->pair_count = 5;
    ctx->state->rules->homerun_contest.pair_index = 0;
    ctx->state->rules->half_inning.strikes = initial_strikes;

    int battingTeam = get_batting_team_index(sb);
    for (int i = 0; i < MAX_HOMERUN_PAIRS; i++) {
        sb->teams[battingTeam].batter_runner_indices[0][i] = -1;
        sb->teams[battingTeam].batter_runner_indices[1][i] = -1;
    }
    // Pair 0: batter 1, runner 0. Pair 1: batter 3, runner 2.
    sb->teams[battingTeam].batter_runner_indices[0][0] = 1;
    sb->teams[battingTeam].batter_runner_indices[1][0] = 0;
    sb->teams[battingTeam].batter_runner_indices[0][1] = 3;
    sb->teams[battingTeam].batter_runner_indices[1][1] = 2;

    setup_batter_at_home(ctx, 1);
    place_runner_at_base(ctx, 0, BASE_THIRD, 0.0f);

    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);
    move_pitcher_away(ctx);
    return ctx;
}

int test_contest_first_strike_foul_resumes_same_pair(void)
{
    ScenarioContext* ctx = setup_contest_scenario(0);

    Vector3D home = ctx->state->field_positions->pitch_plate;
    Vector3D foulTarget = {200.0f, 0.0f, 200.0f};
    hit_fly_ball_to_location(ctx, home, foulTarget);
    ctx->state->match->events.ball_hit_by_bat = 1; // real swing contact counts strike 1

    // Both batter and runner leave their bases while the foul ball is airborne.
    trigger_player_run_to_next_base(ctx, 1, BASE_HOME);
    trigger_player_run_to_next_base(ctx, 0, BASE_THIRD);

    int sawFoul = 0;
    int returnedBallHomeDuringFoul = 0;
    int foulResetCompleted = 0;

    for (int frame = 0; frame < 600; frame++) {
        simulate_frames(ctx, 1);
        const RefereeState* ref = &ctx->state->rules->referee;

        ASSERT(
            !(ref->foul_state != FOUL_STATE_NONE && ref->next_pair_state != HR_PAIR_STATE_NONE),
            "foul_state and next_pair_state must never overlap"
        );
        ASSERT_EQ(HR_PAIR_STATE_NONE, (int)ref->next_pair_state, "1st-strike foul must never start pair-end");
        ASSERT_EQ(0, ctx->state->rules->homerun_contest.pair_index, "pair_index must stay on Pair 0");
        ASSERT_EQ(
            0, ctx->state->rules->scoreboard.teams[0].runs + ctx->state->rules->scoreboard.teams[1].runs,
            "1st-strike foul must not award any run"
        );

        if (ref->foul_state == FOUL_STATE_DETECTED) {
            sawFoul = 1;
            // Return the ball home midway through the foul grace period (the exact Bug A2 trigger).
            if (!returnedBallHomeDuringFoul && ref->foul_timer >= 20) {
                give_ball_to_pitcher(ctx);
                returnedBallHomeDuringFoul = 1;
            }
        } else if (sawFoul && ref->foul_state == FOUL_STATE_NONE) {
            foulResetCompleted = 1;
            break;
        }
    }

    ASSERT_EQ(1, sawFoul, "foul must be detected");
    ASSERT_EQ(1, returnedBallHomeDuringFoul, "ball was returned home during foul grace period");
    ASSERT_EQ(1, foulResetCompleted, "foul reset must complete");
    ASSERT_EQ(0, ctx->state->rules->homerun_contest.pair_index, "same pair resumes after 1st-strike foul");
    ASSERT_EQ(1, ctx->state->rules->half_inning.strikes, "strike count is 1 after 1st-strike foul");
    ASSERT_EQ(
        1, ctx->state->rules->half_inning.batter_may_leave_home,
        "§32 right persists across 1st-strike foul reset in contest"
    );
    ASSERT_EQ(
        PLAYER_STATE_AT_BAT, (int)ctx->state->match->players[1].batting.presence.state,
        "Pair 0 batter resumes at the plate"
    );
    ASSERT_EQ(
        BASE_THIRD, (int)ctx->state->match->players[0].batting.presence.base_id, "Pair 0 runner resumes on third base"
    );

    // Continuing contest batter may commit to leave home before the next pitch is thrown.
    intent_push(
        &ctx->state->channels.batting,
        (IntentMessage){.kind = INTENT_BASE_RUN, .as.base_run = {.base = BASE_HOME, .command = RUN_COMMIT}}
    );
    simulate_frames(ctx, 1);
    ASSERT_EQ(
        PLAYER_STATE_RUNNING, (int)ctx->state->match->players[1].batting.presence.state,
        "contest batter may leave home after 1st-strike foul reset before next pitch"
    );
    ASSERT_EQ(1, ctx->state->match->players[1].path.going_forward, "contest batter runs forward toward 1st");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

int test_contest_third_strike_foul_advances_pair_cleanly(void)
{
    ScenarioContext* ctx = setup_contest_scenario(2);

    Vector3D home = ctx->state->field_positions->pitch_plate;
    Vector3D foulTarget = {200.0f, 0.0f, 200.0f};
    hit_fly_ball_to_location(ctx, home, foulTarget);
    ctx->state->match->events.ball_hit_by_bat = 1; // real swing contact counts strike 1

    // Third-base runner sprints home while the 3rd-strike foul is in the air.
    trigger_player_run_to_next_base(ctx, 0, BASE_THIRD);

    int batterMarkedOut = 0;
    int sawPairEnd = 0;
    int pairResetCompleted = 0;

    for (int frame = 0; frame < 800; frame++) {
        simulate_frames(ctx, 1);
        const RefereeState* ref = &ctx->state->rules->referee;
        const PlayerInfo* batter0 = &ctx->state->match->players[1];

        ASSERT(
            !(ref->foul_state != FOUL_STATE_NONE && ref->next_pair_state != HR_PAIR_STATE_NONE),
            "foul_state and next_pair_state must never overlap"
        );
        ASSERT_EQ(FOUL_STATE_NONE, (int)ref->foul_state, "3rd-strike foul in contest must be subsumed immediately");
        ASSERT_EQ(
            0, ctx->state->rules->scoreboard.teams[0].runs + ctx->state->rules->scoreboard.teams[1].runs,
            "3rd-strike foul must never award a run"
        );

        if (ref->batting_players[1].status == PLAYER_STATUS_OUT) {
            batterMarkedOut = 1;
        }
        if (batterMarkedOut && !pairResetCompleted) {
            ASSERT(
                batter0->batting.presence.state != PLAYER_STATE_AT_BAT,
                "burnt batter must never be teleported back to AT_BAT before the pair resets"
            );
        }

        if (ref->next_pair_state == HR_PAIR_STATE_DETECTED) {
            sawPairEnd = 1;
        } else if (sawPairEnd && ref->next_pair_state == HR_PAIR_STATE_NONE) {
            pairResetCompleted = 1;
            break;
        }
    }

    ASSERT_EQ(1, batterMarkedOut, "3rd-strike foul must burn Pair 0 batter");
    ASSERT_EQ(1, sawPairEnd, "3rd-strike foul must enter pair-end state machine");
    ASSERT_EQ(1, pairResetCompleted, "pair reset must complete");
    ASSERT_EQ(1, ctx->state->rules->homerun_contest.pair_index, "pair_index must advance to Pair 1");
    ASSERT_EQ(0, ctx->state->rules->half_inning.strikes, "Pair 1 starts with 0 strikes");
    ASSERT_EQ(
        PLAYER_STATE_AT_BAT, (int)ctx->state->match->players[3].batting.presence.state,
        "Pair 1 batter is seated at the plate"
    );
    ASSERT_EQ(
        BASE_THIRD, (int)ctx->state->match->players[2].batting.presence.base_id, "Pair 1 runner is seated on third base"
    );

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
