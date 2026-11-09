#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "game_frame.h"
#include "player_view.h"
#include "player_motion.h"

int test_player_view_preserves_phase_and_restart_boundaries(void)
{
    PlayerView view = {0};
    view.players[1] = (PlayerAnimation){.model = PLAYER_ANIM_STAND_BARE, .ticks_per_frame = 1};
    TickFacts facts = {0};
    facts.simulated_players[1].activity =
        (PlayerActivityEvents){.latest = PLAYER_ACTIVITY_TARGET_REACHED, .started = PLAYER_ACTIVITY_WALKING_TO_TARGET};
    player_view_after_tick(&view, &facts);
    ASSERT_EQ(PLAYER_ANIM_STAND_BARE, view.players[1].model, "an arrival supersedes the earlier walk");
    ASSERT_EQ(16, view.players[1].frame_count, "arrival must not discard the earlier stride's setup");
    ASSERT_EQ(0, view.players[1].frame_tick, "an arrived body does not advance its former walking loop");

    facts = (TickFacts){0};
    facts.enforced_players[1].activity =
        (PlayerActivityEvents){.latest = PLAYER_ACTIVITY_RUNNING_ROUTE, .started = PLAYER_ACTIVITY_RUNNING_ROUTE};
    player_view_after_tick(&view, &facts);
    ASSERT_EQ(PLAYER_ANIM_RUN_BARE, view.players[1].model, "a late enforced route reaches the client");
    ASSERT_EQ(0, view.players[1].frame_tick, "enforcement begins after this tick's clip advancement");
    facts = (TickFacts){0};
    player_view_after_tick(&view, &facts);
    ASSERT_EQ(1, view.players[1].frame_tick, "the route first advances on the following tick");

    view.players[12] = (PlayerAnimation){
        .model = PLAYER_ANIM_THROW_RELEASE, .frame_tick = 17, .frame_count = 21, .ticks_per_frame = 2
    };
    facts.simulated_players[12].throw_recovering = true;
    player_view_after_tick(&view, &facts);
    ASSERT_EQ(18, view.players[12].frame_tick, "the release follows physical recovery");
    facts.simulated_players[12].activity.latest = PLAYER_ACTIVITY_BALL_CAUGHT;
    player_view_after_tick(&view, &facts);
    ASSERT_EQ(PLAYER_ANIM_STAND_WITH_BALL, view.players[12].model, "a recatch replaces the release");
    ASSERT_EQ(18, view.players[12].frame_tick, "a recatch does not invent a fresh clip clock");
    view.players[12] = (PlayerAnimation){
        .model = PLAYER_ANIM_THROW_RELEASE, .frame_tick = 40, .frame_count = 21, .ticks_per_frame = 2
    };
    facts.simulated_players[12].activity = (PlayerActivityEvents){0};
    player_view_after_tick(&view, &facts);
    ASSERT_EQ(41, view.players[12].frame_tick, "a recovering release can reach its final mesh");
    facts.simulated_players[12].throw_recovering = false;
    player_view_after_tick(&view, &facts);
    ASSERT_EQ(PLAYER_ANIM_STAND_NO_BALL, view.players[12].model, "physical expiry ends the follow-through");
    ASSERT_EQ(41, view.players[12].frame_tick, "expiry changes the pose without taking an extra clip step");
    return TEST_PASSED;
}

int test_player_activity_facts_survive_same_tick_arrival(void)
{
    ScenarioContext* ctx = create_scenario();
    ctx->state->rules->scoreboard.teams[0].control = CONTROL_PLAYER_1;
    ctx->state->rules->scoreboard.teams[1].control = CONTROL_PLAYER_2;
    MatchSession* match = ctx->state->match;
    PlayerView view;
    player_view_init(&view, match);
    move_to_target(match->playerInfo, 1, &match->playerInfo[1].tPI.location);
    ASSERT_EQ(1, update_game_frame(ctx->state, &ctx->menu), "the real pipeline runs");
    const TickFacts* facts = &ctx->state->tick_facts;
    ASSERT_EQ(PLAYER_ACTIVITY_WALKING_TO_TARGET, facts->simulated_players[1].activity.started, "the walk began");
    ASSERT_EQ(PLAYER_ACTIVITY_TARGET_REACHED, facts->simulated_players[1].activity.latest, "it also arrived");
    ASSERT_EQ(
        PLAYER_ACTIVITY_NONE, facts->enforced_players[1].activity.latest, "SIMULATE facts cannot leak into ENFORCE"
    );
    ASSERT_EQ(PLAYER_ACTIVITY_NONE, match->playerInfo[1].events.latest, "CLEAR leaves no activity for controllers");
    ASSERT_EQ(PLAYER_ACTIVITY_NONE, match->playerInfo[1].events.started, "CLEAR also drains the fresh-start fact");
    player_view_after_tick(&view, facts);
    ASSERT_EQ(PLAYER_ANIM_STAND_BARE, view.players[1].model, "the transient arrival selects the standing body");
    ASSERT_EQ(16, view.players[1].frame_count, "the transient fresh start still sets the original clip size");
    ASSERT_EQ(0, view.players[1].frame_tick, "the stopped body does not take a phantom stride");
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

int test_player_view_resets_before_batter_readiness(void)
{
    ScenarioContext* ctx = create_scenario();
    ctx->state->rules->scoreboard.teams[0].control = CONTROL_PLAYER_1;
    ctx->state->rules->scoreboard.teams[1].control = CONTROL_PLAYER_2;
    setup_batter_at_home(ctx, 0);
    give_ball_to_pitcher(ctx);
    initialize_referee_from_physical_state(ctx);
    PlayerView view;
    player_view_init(&view, ctx->state->match);
    ASSERT_EQ(1, update_game_frame(ctx->state, &ctx->menu), "initial setup settles before the reset fixture");
    player_view_after_tick(&view, &ctx->state->tick_facts);
    ctx->state->rules->referee.battingPlayers[0].baseAtPitchStart = BASE_HOME;
    ctx->state->rules->referee.foulState = FOUL_STATE_RESETTING;
    view.players[0] =
        (PlayerAnimation){.model = PLAYER_ANIM_BAT_SWING_1, .frame_tick = 37, .frame_count = 34, .ticks_per_frame = 3};
    ASSERT_EQ(1, update_game_frame(ctx->state, &ctx->menu), "the foul reset runs through the pipeline");
    const TickFacts* facts = &ctx->state->tick_facts;
    ASSERT(facts->world_reset, "the physical reset must really have happened");
    ASSERT_EQ(
        PLAYER_ACTIVITY_NONE, facts->simulated_players[0].activity.started,
        "no earlier fresh start may hide a missing enforcement reset"
    );
    ASSERT_EQ(PLAYER_ACTIVITY_RESET, facts->enforced_players[0].activity.started, "the reset restarts the lifetime");
    ASSERT_EQ(PLAYER_ACTIVITY_BATTER_READY, facts->enforced_players[0].activity.latest, "the same batter then resumes");
    player_view_after_tick(&view, facts);
    ASSERT_EQ(PLAYER_ANIM_BATTER_READY, view.players[0].model, "readiness must survive the reset");
    ASSERT_EQ(0, view.players[0].frame_tick, "no stale swing clock survives");
    ASSERT_EQ(0, view.players[0].frame_count, "the reset clears clip metadata");
    ASSERT_EQ(1, view.players[0].ticks_per_frame, "standing remains safe to draw immediately");
    cleanup_scenario(ctx);
    return TEST_PASSED;
}
