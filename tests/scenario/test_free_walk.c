#include "scenario_builder.h"
#include "rules_state.h"
#include "intents.h"
#include "world.h"
#include "session.h"
#include "test_helpers.h"
#include "all_scenarios.h"
#include <stdio.h>
#include "intent_channel.h"

/**
 * TEST: Free Walk Resolution
 *
 * Verifies that accepting a free walk:
 * 1. Triggers player movement (Action)
 * 2. Grants safety at the next base (Referee)
 * 3. Player physically runs and arrives at the next base
 */
int test_free_walk_resolution(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* game = ctx->state->match;
    GameRulesState* rules = ctx->state->rules;

    // 1. Setup: Runner at 1st base (physical state)
    int runnerIdx = 0;
    place_runner_at_base(ctx, runnerIdx, BASE_FIRST, 0.0f);

    // 2. Initialize referee from physical state
    initialize_referee_from_physical_state(ctx);

    printf(
        "[TEST] Initial state: Player %d at base %d, safety=%d\n", runnerIdx,
        game->players[runnerIdx].batting.presence.base_id, rules->referee.batting_players[runnerIdx].current_safety_base
    );

    // 3. Setup: Free Walk Offer (artificial - normally from game_consolidation.c)
    game->free_walk.runner = runnerIdx;
    game->free_walk.from = BASE_FIRST;
    game->offers.free_walk = 1;

    printf("[TEST] Offering Free Walk to Player %d from Base %d\n", runnerIdx, BASE_FIRST);

    // 3b. Wait a frame (simulate UI showing the offer before player responds)
    simulate_frames(ctx, 1);

    // 4. Action: Accept Free Walk — declared as a message on the batting team's channel, the same way
    // a human's key or the AI's decision reaches the engine.
    intent_push(
        &ctx->state->channels.batting, (IntentMessage){.kind = INTENT_TAKE_FREE_WALK, .as.free_walk = {.accept = 1}}
    );

    // 5. Process acceptance (1 frame to trigger event and start movement)
    simulate_frames(ctx, 1);

    printf(
        "[TEST] After acceptance: State=%d (Expected %d), SafetyBase=%d (Expected %d)\n",
        game->players[runnerIdx].batting.presence.state, PLAYER_STATE_ADVANCING_FREELY,
        rules->referee.batting_players[runnerIdx].current_safety_base, BASE_SECOND
    );

    // CHECK 1: Action system started movement
    ASSERT_EQ(
        PLAYER_STATE_ADVANCING_FREELY, game->players[runnerIdx].batting.presence.state,
        "Player state should be ADVANCING_FREELY"
    );

    // CHECK 2: Referee granted safety at NEXT base (2nd) immediately
    BaseID safetyBase = rules->referee.batting_players[runnerIdx].current_safety_base;
    ASSERT_EQ(BASE_SECOND, safetyBase, "Referee should grant safety at SECOND base immediately upon event");

    // CHECK 3: Base at pitch start updated (for future foul play resolution)
    BaseID startBase = rules->referee.batting_players[runnerIdx].base_at_pitch_start;
    ASSERT_EQ(BASE_SECOND, startBase, "BaseAtPitchStart should be updated to SECOND base");

    // 6. Wait for runner to physically arrive at second base
    printf("[TEST] Waiting for runner to arrive at 2nd base...\n");
    int arrivedAt2nd = 0;
    for (int frame = 0; frame < 500; frame++) {
        simulate_frames(ctx, 1);
        if (game->players[runnerIdx].batting.presence.base_id == BASE_SECOND) {
            arrivedAt2nd = 1;
            printf("[TEST] Runner arrived at 2nd base (frame %d)\n", frame);
            break;
        }
    }

    // CHECK 4: Runner physically arrived at destination
    ASSERT_EQ(1, arrivedAt2nd, "Runner should physically arrive at 2nd base");
    ASSERT_EQ(BASE_SECOND, game->players[runnerIdx].batting.presence.base_id, "Runner baseId should be SECOND");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

/**
 * §26 — a free walk is offered to a PLAYER, so when there is nobody to offer it to there is no offer.
 *
 * Found by the owner in play: get burnt while a walk is on the table and the prompt stays up, with
 * the game refusing to continue until it is answered. The mechanism is that consolidation clears
 * `free_walk.runner` on every wrong pitch so the walk is re-evaluated, and does NOT clear the offer
 * along with it — so if the re-evaluation then finds nobody (the batter burnt, no runner left), the
 * offer was raised again anyway. It named nobody, nothing could withdraw it because the withdrawal
 * asks about the player it names, and the lukkari may not pitch while an offer is open. Only a human
 * answering a question about no one got the game moving again.
 *
 * The fix has two halves — an offer is not RAISED when the walk belongs to nobody, and an offer
 * already open is CLOSED when the player it belongs to is taken out from under it — and the test has
 * to be able to tell them apart. Asserting the end state cannot: the cancellation alone makes the
 * final reading correct while the invalid state was still entered. So this asserts on every frame,
 * and each half of the fix has been ablated against it separately.
 */
int test_free_walk_is_not_offered_to_nobody(void)
{
    ScenarioContext* ctx = create_scenario();
    StateInfo* st = ctx->state;
    MatchSession* m = st->match;
    GameRulesState* r = st->rules;

    setup_batter_at_home(ctx, 0);
    initialize_referee_from_physical_state(ctx);
    simulate_frames(ctx, 2);

    // The whole inner side is off the field: the batter has been burnt and no runner is left.
    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        m->players[i].batting.presence.base_id = BASE_NONE;
        m->players[i].batting.presence.state = PLAYER_STATE_OUT;
        r->referee.batting_players[i].current_safety_base = BASE_NONE;
    }

    // A wrong pitch resolves, which is what runs the §26 free-walk calculation.
    r->half_inning.balls = 2;
    r->between_pitch.pitch_result = PITCH_RESULT_BALL;
    m->ball.pitch_in_flight = 1;
    m->free_walk.runner = -1;

    // Stepped one frame at a time, and asserting on EVERY frame rather than at the end. Checking
    // only the final state cannot tell the two halves of the fix apart: with the offer raised and
    // then withdrawn a frame later, an end-state assertion is green while the invalid state was
    // still reachable. Measured — that is exactly what this test did before it was written this way,
    // and ablating the raise half left it passing.
    int ever_offered = 0;
    for (int f = 0; f < 10; f++) {
        simulate_frames(ctx, 1);
        if (m->offers.free_walk == 1) ever_offered = 1;
    }

    int waiting = m->offers.free_walk;
    int index = m->free_walk.runner;
    cleanup_scenario(ctx);

    ASSERT_EQ(-1, index, "setup: this is the case where §26 has nobody to give the walk to");
    ASSERT_EQ(0, ever_offered, "a free walk that belongs to nobody must never be offered, not even for a frame");
    ASSERT_EQ(0, waiting, "and no offer may be left standing");
    return TEST_PASSED;
}

/**
 * §26 + §32 — a batter who legally breaks off BASE_HOME during a pitch is still eligible for the
 * empty-field 1-ball free walk when that pitch lands off the plate.
 *
 * Regression test for the legacy "Ghost Runner" bug in calculate_free_walk: once the batter starts
 * running toward 1st base, presence.state becomes PLAYER_STATE_RUNNING while presence.base_id stays
 * BASE_HOME, so get_active_batter_index returns -1. The old check rejected any BASE_HOME player not
 * matching get_active_batter_index, silently denying the batter's §26 walk. Also verifies that
 * enforce_legal_state runs before update_game_flow, raising the offer on the exact tick the BALL
 * resolves (zero 1-frame gap).
 */
int test_free_walk_offered_to_running_batter_from_home(void)
{
    ScenarioContext* ctx = create_scenario();
    StateInfo* st = ctx->state;
    MatchSession* m = st->match;
    GameRulesState* r = st->rules;

    const int batter = 0;
    setup_batter_at_home(ctx, batter);
    setup_pitcher_ready(ctx);
    initialize_referee_from_physical_state(ctx);
    simulate_frames(ctx, 2);

    // Release an off-plate (BALL) pitch: low legal power, wide direction.
    intent_push(&st->channels.catching, (IntentMessage){.kind = INTENT_PITCH_POWER, .as.pitch_power = {.power = 0.0f}});
    intent_push(&st->channels.catching, (IntentMessage){.kind = INTENT_PITCH_AIM, .as.pitch_aim = {.direction = 0.4f}});

    int released = 0;
    for (int f = 0; f < 120; f++) {
        simulate_frames(ctx, 1);
        if (m->ball.pitch_in_flight) {
            released = 1;
            break;
        }
    }
    ASSERT_EQ(1, released, "precondition: wrong pitch must release into flight");
    ASSERT_EQ(1, r->half_inning.batter_may_leave_home, "precondition: §32 grants home-leaving on pitch release");

    // Batter legally breaks off BASE_HOME while the wrong pitch is still in flight.
    intent_push(
        &st->channels.batting,
        (IntentMessage){.kind = INTENT_BASE_RUN, .as.base_run = {.base = BASE_HOME, .command = RUN_COMMIT}}
    );
    simulate_frames(ctx, 1);
    ASSERT_EQ(PLAYER_STATE_RUNNING, m->players[batter].batting.presence.state, "batter must be running off home");
    ASSERT_EQ(BASE_HOME, m->players[batter].batting.presence.base_id, "running batter still has base_id BASE_HOME");

    // Step until the pitch hits the ground and resolves as BALL. The offer must open on the EXACT
    // same tick pitch_in_flight clears — no 1-frame lag between pitch_in_flight=0 and offers.free_walk=1.
    int resolved = 0;
    for (int f = 0; f < 200; f++) {
        simulate_frames(ctx, 1);
        if (!m->ball.pitch_in_flight) {
            resolved = 1;
            break;
        }
    }
    ASSERT_EQ(1, resolved, "wrong pitch must land and resolve");
    ASSERT_EQ(PITCH_RESULT_BALL, r->between_pitch.pitch_result, "off-plate pitch must be called BALL");
    ASSERT_EQ(1, r->half_inning.balls, "balls count must increment to 1");
    ASSERT_EQ(
        1, m->offers.free_walk,
        "running batter alone on field must be offered §26 free walk on the exact tick the BALL resolves"
    );
    ASSERT_EQ(batter, m->free_walk.runner, "free walk must name the running batter");
    ASSERT_EQ(BASE_HOME, m->free_walk.from, "free walk source base must be BASE_HOME");

    // Accept the free walk mid-run and confirm safe arrival at 1st base.
    intent_push(&st->channels.batting, (IntentMessage){.kind = INTENT_TAKE_FREE_WALK, .as.free_walk = {.accept = 1}});
    simulate_frames(ctx, 1);
    ASSERT_EQ(0, m->offers.free_walk, "accepting closes the offer");
    ASSERT_EQ(
        PLAYER_STATE_ADVANCING_FREELY, m->players[batter].batting.presence.state,
        "running batter transitions to ADVANCING_FREELY"
    );
    ASSERT_EQ(BASE_FIRST, r->referee.batting_players[batter].current_safety_base, "safety granted at 1st base");

    int arrived = 0;
    for (int f = 0; f < 300; f++) {
        simulate_frames(ctx, 1);
        if (m->players[batter].batting.presence.base_id == BASE_FIRST) {
            arrived = 1;
            break;
        }
    }
    cleanup_scenario(ctx);

    ASSERT_EQ(1, arrived, "running batter accepting §26 free walk must arrive safely at 1st base");
    return TEST_PASSED;
}

/**
 * ENFORCE ordering + §26 candidate hygiene: because enforce_legal_state runs BEFORE update_game_flow,
 * a runner ruled OUT or SCORED on the exact tick a BALL resolves is already removed from play and
 * cannot steal the free-walk offer from the next live runner. Likewise, BASE_HOME_SCORED is never a
 * valid free-walk source base.
 */
int test_free_walk_skips_player_out_or_scored_on_ball_frame(void)
{
    ScenarioContext* ctx = create_scenario();
    StateInfo* st = ctx->state;
    MatchSession* m = st->match;
    GameRulesState* r = st->rules;

    const int runner_third = 0;
    const int runner_first = 1;
    place_runner_at_base(ctx, runner_third, BASE_THIRD, 0.0f);
    place_runner_at_base(ctx, runner_first, BASE_FIRST, 0.0f);
    initialize_referee_from_physical_state(ctx);

    // Referee has just ruled runner_third OUT on this tick while a 2nd BALL resolves.
    r->referee.batting_players[runner_third].status = PLAYER_STATUS_OUT;
    r->half_inning.balls = 2;
    r->between_pitch.pitch_result = PITCH_RESULT_BALL;
    m->ball.pitch_in_flight = 1;

    simulate_frames(ctx, 1);

    ASSERT_EQ(BASE_NONE, m->players[runner_third].batting.presence.base_id, "OUT runner must be removed first");
    ASSERT_EQ(1, m->offers.free_walk, "free walk must still be offered to the live runner on 1st");
    ASSERT_EQ(runner_first, m->free_walk.runner, "OUT lead runner must not steal the free walk");
    ASSERT_EQ(BASE_FIRST, m->free_walk.from, "free walk must be offered from 1st base");

    // Also verify BASE_HOME_SCORED (e.g., pending run) is never selected as a free-walk source.
    m->offers.free_walk = 0;
    m->free_walk = (FreeWalkEntitlement){.runner = -1, .from = BASE_NONE};
    const int pending_scorer = 2;
    m->players[pending_scorer].batting.presence.base_id = BASE_HOME_SCORED;
    m->players[pending_scorer].batting.presence.state = PLAYER_STATE_ON_BASE;
    r->referee.batting_players[pending_scorer].status = PLAYER_STATUS_ACTIVE;
    r->referee.batting_players[pending_scorer].current_safety_base = BASE_HOME_SCORED;
    r->referee.batting_players[pending_scorer].has_pending_run = 1;
    r->half_inning.balls = 2;
    r->between_pitch.pitch_result = PITCH_RESULT_BALL;
    m->ball.pitch_in_flight = 1;

    simulate_frames(ctx, 1);

    ASSERT_EQ(1, m->offers.free_walk, "live runner on 1st must still receive the offer");
    ASSERT_EQ(runner_first, m->free_walk.runner, "BASE_HOME_SCORED player must never be selected for a free walk");
    ASSERT_EQ(BASE_FIRST, m->free_walk.from, "free walk source must be BASE_FIRST, never BASE_HOME_SCORED");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
