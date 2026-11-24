#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "game_frame.h"
#include "home_run_view.h"
#include "intent_channel.h"
#include "base_control.h"

static ScenarioContext* camera_scenario(void)
{
    ScenarioContext* ctx = create_scenario();
    ctx->state->rules->scoreboard.teams[0].control = CONTROL_PLAYER_1;
    ctx->state->rules->scoreboard.teams[1].control = CONTROL_PLAYER_2;
    give_ball_to_pitcher(ctx);
    initialize_referee_from_physical_state(ctx);
    return ctx;
}

int test_home_run_view_preserves_phase_and_hold_boundaries(void)
{
    HomeRunView view = {.hold_ticks = -1};
    // Tick 1: a homeward run starts on pitch release; arms pending without immediately lifting.
    TickFacts facts = {
        .pitch_released = 1,
        .homeward_run_started = 1,
        .ball_home = 0,
        .third_base_control = THIRD_CONTROLLER_OFF_BASE,
        .enforcement_completed = 1
    };
    home_run_view_after_tick(&view, &facts);
    ASSERT_EQ(0, view.active, "departure tick arms pending before lifting the camera");
    ASSERT_EQ(1, view.pending, "homeward run start sets pending");

    // Tick 2: runner has resolved off third and ball is home -> confirms active and starts the hold.
    facts = (TickFacts){.ball_home = 1, .third_base_control = THIRD_NO_CONTROLLER, .enforcement_completed = 1};
    home_run_view_after_tick(&view, &facts);
    ASSERT_EQ(1, view.active, "runner remaining off third confirms elevated framing");
    ASSERT_EQ(0, view.pending, "pending clears on confirmation");
    ASSERT_EQ(1, view.hold_ticks, "the settled-home tick also advances game flow");

    facts = (TickFacts){.third_base_control = THIRD_NO_CONTROLLER, .enforcement_completed = 1};
    for (int tick = 0; tick < 49; tick++)
        home_run_view_after_tick(&view, &facts);
    ASSERT_EQ(1, view.active, "the view still holds on tick 50");
    ASSERT_EQ(50, view.hold_ticks, "the hold boundary is strict");

    facts.homeward_run_started_in_enforcement = 1;
    home_run_view_after_tick(&view, &facts);
    ASSERT_EQ(0, view.active, "previous hold expired on tick 51");
    ASSERT_EQ(1, view.pending, "a late enforced run arms a new view after the previous hold expires");
    ASSERT_EQ(-1, view.hold_ticks, "a late run starts a new view, not an expired hold");

    facts = (TickFacts){.third_base_control = THIRD_CONTROLLER_OFF_BASE, .enforcement_completed = 1};
    home_run_view_after_tick(&view, &facts);
    ASSERT_EQ(1, view.active, "late enforced run confirms on the next tick");

    facts = (TickFacts){.pitch_released = 1, .enforcement_completed = 1};
    home_run_view_after_tick(&view, &facts);
    ASSERT_EQ(0, view.active, "pitch release ends the homeward framing");

    view = (HomeRunView){.active = 1, .hold_ticks = 17};
    facts = (TickFacts){.pitch_released = 1, .third_base_control = THIRD_NO_CONTROLLER, .enforcement_completed = 1};
    home_run_view_after_tick(&view, &facts);
    ASSERT_EQ(18, view.hold_ticks, "release does not silently reset the existing hold clock");

    facts.homeward_run_started = 1;
    facts.homeward_run_started_in_enforcement = 1;
    facts.world_reset = 1;
    home_run_view_after_tick(&view, &facts);
    ASSERT_EQ(0, view.active, "a world reset supersedes earlier route starts");
    ASSERT_EQ(0, view.pending, "a world reset clears pending starts");
    ASSERT_EQ(-1, view.hold_ticks, "a world reset cancels the hold");
    return TEST_PASSED;
}

int test_home_run_view_ignores_aborted_break_and_drops_on_retreat(void)
{
    HomeRunView view = {.hold_ticks = -1};

    // Case 1: runner breaks on pitch release, then snaps back to third one tick later (väärä recall).
    TickFacts release = {
        .pitch_released = 1,
        .homeward_run_started = 1,
        .ball_home = 0,
        .third_base_control = THIRD_CONTROLLER_OFF_BASE,
        .enforcement_completed = 1
    };
    home_run_view_after_tick(&view, &release);
    ASSERT_EQ(0, view.active, "aborted break must not lift on departure frame");
    ASSERT_EQ(1, view.pending, "departure frame only arms pending");

    TickFacts snapped_back = {
        .ball_home = 0, .third_base_control = THIRD_CONTROLLER_ON_BASE, .enforcement_completed = 1
    };
    home_run_view_after_tick(&view, &snapped_back);
    ASSERT_EQ(0, view.active, "1-tick recall back onto third cancels pending without lifting camera");
    ASSERT_EQ(0, view.pending, "pending cleared after snap-back");
    ASSERT_EQ(-1, view.hold_ticks, "no hold started for aborted break");

    // Case 2: runner takes a real lead (stays off third), then retreats safely back onto third.
    home_run_view_after_tick(&view, &release);
    TickFacts running = {.ball_home = 0, .third_base_control = THIRD_CONTROLLER_OFF_BASE, .enforcement_completed = 1};
    home_run_view_after_tick(&view, &running);
    ASSERT_EQ(1, view.active, "sustained departure lifts the camera");

    home_run_view_after_tick(&view, &snapped_back);
    ASSERT_EQ(0, view.active, "safe return onto third drops elevated framing immediately");
    ASSERT_EQ(-1, view.hold_ticks, "safe return onto third does not start a 50-tick hold");
    return TEST_PASSED;
}

int test_homeward_run_facts_survive_clear_and_legal_enforcement(void)
{
    ScenarioContext* ctx = camera_scenario();
    place_runner_at_base(ctx, 1, BASE_THIRD, 0.0f);
    initialize_referee_from_physical_state(ctx);
    // The pending legal outcome is enforced later in this tick, after the physical observation.
    ctx->state->rules->referee.batting_players[1].status = PLAYER_STATUS_OUT;
    intent_push(
        &ctx->state->channels.batting,
        (IntentMessage){.kind = INTENT_BASE_RUN, .as.base_run = {.base = BASE_THIRD, .command = RUN_COMMIT}}
    );
    ASSERT_EQ(1, update_game_frame(ctx->state, &ctx->menu), "the full pipeline must run");
    const TickFacts* facts = &ctx->state->tick_facts;
    ASSERT_EQ(1, facts->homeward_run_started, "the real route start survives the tick");
    ASSERT_EQ(1, facts->ball_home, "SIMULATE sees the ball held at home");
    ASSERT_EQ(
        THIRD_CONTROLLER_OFF_BASE, facts->third_base_control, "SIMULATE still sees the third-base controller off base"
    );
    ASSERT_EQ(0, ctx->state->match->events.homeward_runs_started, "CLEAR drains the physical event");
    ASSERT_EQ(
        -1, get_base_controller(ctx->state->match, &ctx->state->rules->referee, BASE_THIRD),
        "the final snapshot has already lost the third-base controller"
    );
    HomeRunView view = {.hold_ticks = -1};
    home_run_view_after_tick(&view, facts);
    ASSERT_EQ(1, view.pending, "the fleeting route start still reaches its client");
    ASSERT_EQ(-1, view.hold_ticks, "post-enforcement emptiness must not start the hold one tick early");

    // On the next tick, the runner is resolved off third (THIRD_NO_CONTROLLER), confirming the view.
    ASSERT_EQ(1, update_game_frame(ctx->state, &ctx->menu), "second tick must run");
    home_run_view_after_tick(&view, &ctx->state->tick_facts);
    ASSERT_EQ(1, view.active, "resolved runner confirms elevated framing on the next tick");
    ASSERT_EQ(1, view.hold_ticks, "hold begins once runner is resolved and ball is home");
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

int test_camera_facts_distinguish_resets_from_flow_short_circuits(void)
{
    for (int recipe = 0; recipe < 4; recipe++) {
        ScenarioContext* ctx = camera_scenario();
        GameRulesState* rules = ctx->state->rules;
        if (recipe == 0) {
            rules->referee.foul_state = FOUL_STATE_RESETTING;
        } else if (recipe == 1) {
            rules->referee.end_of_inning_state = END_INNING_STATE_RESETTING;
        } else if (recipe == 2) {
            rules->scoreboard.period = 4;
            rules->scoreboard.pair_count = 5;
            rules->homerun_contest.pair_index = 1;
            rules->referee.next_pair_state = HR_PAIR_STATE_RESETTING;
        } else {
            rules->referee.end_of_inning_state = END_INNING_STATE_RESETTING;
            rules->referee.period_transition = PERIOD_TRANSITION_INTER_PERIOD;
        }
        ASSERT_EQ(1, update_game_frame(ctx->state, &ctx->menu), "the reset/transition tick must run");
        const TickFacts* facts = &ctx->state->tick_facts;
        ASSERT_EQ(recipe < 3, facts->world_reset, "only an actual physical reset reports a reset");
        ASSERT_EQ(0, facts->enforcement_completed, "a reset or pair transition skips normal enforcement");
        HomeRunView view = {.active = 1, .hold_ticks = 20};
        home_run_view_after_tick(&view, facts);
        ASSERT_EQ(recipe == 3, view.active, "the period transition pauses the cue; reset recipes clear it");
        ASSERT_EQ(recipe == 3 ? 20 : -1, view.hold_ticks, "skipped game flow cannot advance the hold");
        cleanup_scenario(ctx);
    }
    return TEST_PASSED;
}
