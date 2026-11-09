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
    TickFacts facts = {
        .pitch_released = 1,
        .homeward_run_started = 1,
        .ball_home = 1,
        .third_base_control = THIRD_CONTROLLER_ON_BASE,
        .enforcement_completed = 1
    };
    home_run_view_after_tick(&view, &facts);
    ASSERT_EQ(1, view.active, "a runner starting on pitch release reactivates the view");
    ASSERT_EQ(1, view.hold_ticks, "the settled-home tick also advances game flow");

    facts = (TickFacts){.enforcement_completed = 1};
    for (int tick = 0; tick < 49; tick++)
        home_run_view_after_tick(&view, &facts);
    ASSERT_EQ(1, view.active, "the view still holds on tick 50");
    ASSERT_EQ(50, view.hold_ticks, "the hold boundary is strict");

    facts.homeward_run_started_in_enforcement = 1;
    home_run_view_after_tick(&view, &facts);
    ASSERT_EQ(1, view.active, "a late enforced run starts after the previous hold expires");
    ASSERT_EQ(-1, view.hold_ticks, "a late run starts a new view, not an expired hold");

    facts = (TickFacts){.pitch_released = 1, .enforcement_completed = 1};
    home_run_view_after_tick(&view, &facts);
    ASSERT_EQ(0, view.active, "pitch release ends the homeward framing");

    view = (HomeRunView){.active = 1, .hold_ticks = 17};
    home_run_view_after_tick(&view, &facts);
    ASSERT_EQ(18, view.hold_ticks, "release does not silently reset the existing hold clock");

    facts.homeward_run_started = 1;
    facts.homeward_run_started_in_enforcement = 1;
    facts.world_reset = 1;
    home_run_view_after_tick(&view, &facts);
    ASSERT_EQ(0, view.active, "a world reset supersedes earlier route starts");
    ASSERT_EQ(-1, view.hold_ticks, "a world reset cancels the hold");
    return TEST_PASSED;
}

int test_homeward_run_facts_survive_clear_and_legal_enforcement(void)
{
    ScenarioContext* ctx = camera_scenario();
    place_runner_at_base(ctx, 1, BASE_THIRD, 0.0f);
    initialize_referee_from_physical_state(ctx);
    // The pending legal outcome is enforced later in this tick, after the physical observation.
    ctx->state->rules->referee.battingPlayers[1].status = PLAYER_STATUS_OUT;
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
    ASSERT_EQ(0, ctx->state->match->gameEvents.homeward_runs_started, "CLEAR drains the physical event");
    ASSERT_EQ(
        -1, get_base_controller(ctx->state->match, &ctx->state->rules->referee, BASE_THIRD),
        "the final snapshot has already lost the third-base controller"
    );
    HomeRunView view = {.hold_ticks = -1};
    home_run_view_after_tick(&view, facts);
    ASSERT_EQ(1, view.active, "the fleeting route start still reaches its client");
    ASSERT_EQ(-1, view.hold_ticks, "post-enforcement emptiness must not start the hold one tick early");
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

int test_camera_facts_distinguish_resets_from_flow_short_circuits(void)
{
    for (int recipe = 0; recipe < 4; recipe++) {
        ScenarioContext* ctx = camera_scenario();
        GameRulesState* rules = ctx->state->rules;
        if (recipe == 0) {
            rules->referee.foulState = FOUL_STATE_RESETTING;
        } else if (recipe == 1) {
            rules->referee.endOfInningState = END_INNING_STATE_RESETTING;
        } else if (recipe == 2) {
            rules->scoreboard.period = 4;
            rules->scoreboard.pairCount = 5;
            rules->homeRunContestState.runnerBatterPairCounter = 1;
            rules->referee.nextPairTransitionState = HR_PAIR_STATE_RESETTING;
        } else {
            rules->referee.endOfInningState = END_INNING_STATE_RESETTING;
            rules->referee.periodTransition = PERIOD_TRANSITION_INTER_PERIOD;
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
