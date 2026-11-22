#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "ai/ai_controller.h"
#include "ai/ai_memory.h"
#include "intent_channel.h"
#include "ingest.h"
#include "actualize.h"
#include "actions/throwing_system.h"
#include "ball_catch.h"
#include "player_view.h"
#include "simulate.h"
#include <stddef.h>
#include "intents.h"
#include "world.h"
#include <math.h>

/**
 * CONTRACT: the throw — two declared values, a target and a power, actualized by the engine-owned windup
 * clock (CatchingAction.timer). Both producers declare `power` as a trusted VALUE; the ENGINE owns the
 * release instant (release when the clock reaches windup(power)), never a client "fire now" edge, never
 * derived from the clock:
 *   - both in one tick (AI): the engine sizes the windup to the power and auto-releases at its end. Higher
 *     declared power → faster launch.
 *   - target, then power (human): the target starts the windup (so the gather runs while the human picks
 *     power on the meter); the power hands the engine the value and it releases once the windup for that
 *     power completes — immediately if it already elapsed, else it waits.
 * We construct a ball-holder away from a base and drive the REAL gate and actualizer through the catching
 * channel (no AI, no meter). Calling those two stages directly (not the full frame) keeps the thrown ball
 * un-fielded so the launch velocity can be read at the release frame.
 */

static void declare_target(ScenarioContext* ctx, BaseID base)
{
    intent_push(
        &ctx->state->channels.catching, (IntentMessage){.kind = INTENT_THROW_TARGET, .as.throw_target = {.base = base}}
    );
}

static void declare_power(ScenarioContext* ctx, float power)
{
    intent_push(
        &ctx->state->channels.catching, (IntentMessage){.kind = INTENT_THROW_POWER, .as.throw_power = {.power = power}}
    );
}

// A controlled fielder holds the ball out in the field, far enough from any base to throw.
static void setup_fielder_with_ball(ScenarioContext* ctx)
{
    MatchSession* m = ctx->state->match;
    int fielderIdx = 15;
    m->ball.holder = fielderIdx;
    m->catching.controlled_fielder = fielderIdx;
    m->players[fielderIdx].body.location.x = 5.0f;
    m->players[fielderIdx].body.location.z = 5.0f;
}

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

static float horiz_speed(const MatchSession* m)
{
    float vx = m->ball.velocity.x, vz = m->ball.velocity.z;
    return sqrtf(vx * vx + vz * vz);
}

// Run INGEST + ACTUALIZE until the ball leaves the hand; return the launch horizontal speed (or -1). Also
// reports the number of frames ticked via *frames.
static float run_until_release(ScenarioContext* ctx, int budget, int* frames)
{
    MatchSession* m = ctx->state->match;
    int i;
    for (i = 0; i < budget; i++) {
        tick(ctx);
        if (m->ball.holder == -1) {
            if (frames) *frames = i + 1;
            return horiz_speed(m);
        }
    }
    if (frames) *frames = i;
    return -1.0f;
}

// 0. The windup sizing scales with declared power: throw_windup_total_frames maps [THROW_POWER_MIN,1] →
//    [MIN,MAX] frames, monotonically. This is the ONLY throw↔clock relationship that survives — the
//    human's power is a value declared from the client charge widget, never read back off the clock,
//    so the old power<->windup round-trip (and throw_power_from_windup) is gone.
int test_throw_windup_frames_scale_with_power(void)
{
    // Endpoints exact by construction.
    ASSERT_EQ(THROW_WINDUP_MIN_FRAMES, throw_windup_total_frames(THROW_POWER_MIN), "floor power → MIN windup");
    ASSERT_EQ(THROW_WINDUP_MAX_FRAMES, throw_windup_total_frames(1.0f), "max power → MAX windup");

    // Out-of-band power clamps into the band.
    ASSERT_EQ(THROW_WINDUP_MIN_FRAMES, throw_windup_total_frames(0.0f), "power below floor clamps to MIN windup");
    ASSERT_EQ(THROW_WINDUP_MAX_FRAMES, throw_windup_total_frames(2.0f), "power above 1 clamps to MAX windup");

    // Non-decreasing and bounded across the band.
    int prev = -1;
    for (float p = 0.0f; p <= 1.0f; p += 0.05f) {
        int f = throw_windup_total_frames(p);
        ASSERT(f >= prev, "windup length is non-decreasing in declared power");
        ASSERT(f >= THROW_WINDUP_MIN_FRAMES && f <= THROW_WINDUP_MAX_FRAMES, "windup length stays within [MIN,MAX]");
        prev = f;
    }
    return TEST_PASSED;
}

// 1. A throw declared whole begins the engine windup, releases at throw_windup_total_frames(power), and a
//    higher declared power launches the ball faster (power drives the outcome).
int test_throw_declared_whole_releases_sized_to_power(void)
{
    // --- low power: also assert the windup timing + resolution bookkeeping ---
    ScenarioContext* ctx = create_scenario();
    setup_fielder_with_ball(ctx);
    MatchSession* m = ctx->state->match;

    const float lowPower = 0.3f;
    declare_target(ctx, BASE_FIRST);
    declare_power(ctx, lowPower);

    // First frame begins the windup (engine mutex), without releasing.
    tick(ctx);
    ASSERT_EQ(
        CATCHING_ACTION_THROWING, (int)m->catching_action.kind, "declaring a throw must begin the engine-owned windup"
    );
    ASSERT_EQ(15, m->ball.holder, "ball is still in hand during the windup");

    int windup = throw_windup_total_frames(lowPower);
    int frames = 0;
    float lowSpeed = run_until_release(ctx, windup + 6, &frames);

    ASSERT_EQ(-1, m->ball.holder, "the ball must leave the hand at the windup end");
    ASSERT(lowSpeed > 0.0f, "a released throw has a horizontal launch velocity");
    // frames counts from the SECOND frame onward (the first began the windup); release near the windup end.
    ASSERT(frames + 1 >= windup - 1, "release must wait for the engine windup, not fire instantly");
    ASSERT_EQ(CATCHING_ACTION_NONE, (int)m->catching_action.kind, "the catching action clears at release");
    cleanup_scenario(ctx);

    // --- high power: same geometry → the only difference is the declared power ---
    ScenarioContext* ctx2 = create_scenario();
    setup_fielder_with_ball(ctx2);
    declare_target(ctx2, BASE_FIRST);
    declare_power(ctx2, 1.0f);
    float highSpeed = run_until_release(ctx2, throw_windup_total_frames(1.0f) + 6, NULL);
    ASSERT(highSpeed > 0.0f, "the high-power throw must release");

    ASSERT(highSpeed > lowSpeed, "a higher DECLARED power must launch the ball faster (power drives the outcome)");
    cleanup_scenario(ctx2);
    return TEST_PASSED;
}

// 2. The human's two values a moment apart: a target winds up but never releases while the power is owed;
//    the power hands the engine the value, and the ENGINE owns the release instant — it fires when its
//    clock reaches windup(power), NOT on the frame the power arrived. Proven two ways: (a) a long gather
//    then a low power releases immediately (the windup already elapsed); (b) a short gather then a high
//    power makes the engine WAIT out the windup (no client "fire now" edge). Velocity tracks the DECLARED
//    power either way — the value is trusted, not derived from the clock.
int test_throw_target_then_power_engine_times_release(void)
{
    // --- a long gather, then LOW power → releases immediately, velocity sized to the low power ---
    ScenarioContext* ctx = create_scenario();
    setup_fielder_with_ball(ctx);
    MatchSession* m = ctx->state->match;

    declare_target(ctx, BASE_FIRST);

    for (int i = 0; i < THROW_WINDUP_MAX_FRAMES + 20; i++) {
        tick(ctx);
    }
    ASSERT_EQ(15, m->ball.holder, "a throw never releases while its power is owed");
    ASSERT_EQ(CATCHING_ACTION_THROWING, (int)m->catching_action.kind, "still winding up while pending");
    ASSERT_EQ(
        THROW_WINDUP_MAX_FRAMES, m->catching_action.timer,
        "the windup clock caps at the full-power windup (holding past full does not overspill)"
    );

    // Declare a LOW power. The clock (capped at MAX) is already well past windup(0.3), so the engine
    // releases this very tick — the "power arrived late" case.
    declare_power(ctx, 0.3f);
    tick(ctx);
    ASSERT_EQ(-1, m->ball.holder, "a power declared after a full windup releases immediately");
    float longInitLowPowerSpeed = horiz_speed(m);
    ASSERT_EQ(CATCHING_ACTION_NONE, (int)m->catching_action.kind, "the throw is done at release");
    cleanup_scenario(ctx);

    // --- a short gather, then HIGH power → the engine WAITS out the windup (it owns the release instant) ---
    ScenarioContext* ctx2 = create_scenario();
    setup_fielder_with_ball(ctx2);
    MatchSession* m2 = ctx2->state->match;
    declare_target(ctx2, BASE_FIRST);
    tick(ctx2); // begin windup
    tick(ctx2); // barely wound (clock ~1, well short of windup(1.0) = MAX)

    declare_power(ctx2, 1.0f);
    tick(ctx2); // the power does NOT fire this frame — the windup is not done
    ASSERT_EQ(15, m2->ball.holder, "the engine owns the release: a powered throw waits out its windup");
    ASSERT_EQ(CATCHING_ACTION_THROWING, (int)m2->catching_action.kind, "still winding after the power");

    int budget = THROW_WINDUP_MAX_FRAMES + 10, frames = 0;
    float shortInitHighPowerSpeed = run_until_release(ctx2, budget, &frames);
    ASSERT(shortInitHighPowerSpeed > 0.0f, "the high-power throw eventually releases (once the windup completes)");
    ASSERT(
        frames >= THROW_WINDUP_MAX_FRAMES - THROW_WINDUP_MIN_FRAMES,
        "release waited for the full-power windup, not the frame the power arrived"
    );

    // The DECLARED power decides the launch speed (trusted value), independent of how long the gather ran.
    ASSERT(
        shortInitHighPowerSpeed > longInitLowPowerSpeed,
        "launch velocity tracks the DECLARED power value, not the hold/clock (the engine↔client contract)"
    );
    cleanup_scenario(ctx2);
    return TEST_PASSED;
}

static int check_throw_recovery(int animation_profile)
{
    ScenarioContext* ctx = create_scenario();
    setup_fielder_with_ball(ctx);
    MatchSession* m = ctx->state->match;
    PlayerInfo* thrower = &m->players[15];
    PlayerMotion* activity = &thrower->motion;
    PlayerBody* body = &thrower->body;
    FieldingPlayerState* fielding = &thrower->fielding;
    declare_target(ctx, BASE_FIRST);
    declare_power(ctx, 0.3f);
    ASSERT(run_until_release(ctx, 100, NULL) > 0.0f, "a real declared throw must release");
    PlayerView view;
    player_view_init(&view, m);
    PlayerAnimation* animation = &view.players[15];
    Vector3D release_position = body->location;
    body->home_location = (Vector3D){25.0f, 0.0f, 25.0f};
    fielding->near_home = 0;
    m->ball.location = (Vector3D){0.0f, 100.0f, 0.0f};
    m->ball.velocity = (Vector3D){0};
    m->ball.moving = 0;
    m->chase_replan = CHASE_REPLAN_NONE;

    if (animation_profile == 1) {
        animation->model = PLAYER_ANIM_STAND_NO_BALL;
    } else if (animation_profile == 2) {
        animation->frame_count = 1;
        animation->ticks_per_frame = 1;
    } else if (animation_profile == 3) {
        animation->frame_count = 1000;
        animation->ticks_per_frame = 17;
    } else if (animation_profile == 4) {
        m->ball.location = body->location;
        m->ball.flight_has_hit_ground = 1;
        check_if_ball_can_be_caught(m, ctx->state->field_positions, &ctx->state->play_sound_effect);
        ASSERT_EQ(15, m->ball.holder, "the recovering thrower can catch a ball after it has bounced");
        TickFacts facts = {0};
        facts.enforced_players[15].activity = m->players[15].events;
        facts.enforced_players[15].holds_ball = true;
        player_view_after_tick(&view, &facts);
        ASSERT_EQ(PLAYER_ANIM_STAND_WITH_BALL, animation->model, "the real catch replaces the release pose");
    }

    for (int tick_index = 0; tick_index < 42; tick_index++) {
        if (animation_profile == 2) {
            for (int draw = 0; draw < 5; draw++)
                player_view_after_tick(&view, &(TickFacts){0});
        }
        ASSERT(fielding->throw_recovery_ticks != 0, "presentation cannot finish physical recovery early");
        m->catching.controlled_fielder = 15;
        intent_push(
            &ctx->state->channels.catching,
            (IntentMessage){.kind = INTENT_MOVE_TARGET, .as.move_target = {.point = {25.0f, 0.0f, 25.0f}}}
        );
        tick(ctx);
        ASSERT_EQ(0, activity->moving, "ACTUALIZE cannot move the recovering thrower, even on the last tick");
        // Automatic home-return must also stay blocked through this tick's body step.
        m->catching.controlled_fielder = -1;
        simulate(m, ctx->state->field_positions, &ctx->state->rules->referee, &ctx->state->play_sound_effect);
        ASSERT_EQ(tick_index < 41, fielding->throw_recovery_ticks != 0, "recovery lasts exactly 42 SIMULATE steps");
        ASSERT_FLOAT_EQ(release_position.x, body->location.x, 1e-6f, "recovery stays active through body movement");
        ASSERT_FLOAT_EQ(release_position.z, body->location.z, 1e-6f, "the last recovery tick cannot move the body");
    }

    m->catching.controlled_fielder = 15;
    intent_push(
        &ctx->state->channels.catching,
        (IntentMessage){.kind = INTENT_MOVE_TARGET, .as.move_target = {.point = {25.0f, 0.0f, 25.0f}}}
    );
    tick(ctx);
    ASSERT_EQ(1, activity->moving, "movement resumes in the ACTUALIZE after recovery expires");
    cleanup_scenario(ctx);
    return TEST_PASSED;
}

int test_throw_recovery_preserves_the_movement_boundary(void)
{
    return check_throw_recovery(0);
}

int test_throw_recovery_is_independent_of_animation(void)
{
    for (int profile = 1; profile <= 4; profile++) {
        if (check_throw_recovery(profile) != TEST_PASSED) return TEST_FAILED;
    }
    return TEST_PASSED;
}

// An AI baseman who fielded the ball away from his own post must throw to his idle cover standing
// on that base instead of carrying the ball back himself.
int test_ai_baseman_throws_to_idle_cover_on_own_base(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    const FieldPositions* field = ctx->state->field_positions;

    // A batter is running from home to 1st, so the catching AI targets BASE_FIRST.
    setup_batter_at_home(ctx, 0);
    initialize_referee_from_physical_state(ctx);
    snapshot_pitch_start_state(ctx);
    m->at_bat.may_leave_home = 1;
    trigger_player_run_to_next_base(ctx, 0, BASE_HOME);

    // 1st baseman holds the ball 12m off 1st base; his cover is idle on 1st base.
    int baseman = m->fielding_posts.baseman[BASE_FIRST];
    int cover = m->fielding_posts.cover[BASE_FIRST];
    ASSERT(baseman != -1 && cover != -1 && baseman != cover, "setup: 1st base has distinct baseman and cover");

    m->players[baseman].body.location = (Vector3D){field->first_base.x + 12.0f, 0.0f, field->first_base.z + 4.0f};
    m->players[baseman].body.previous_location = m->players[baseman].body.location;
    m->players[baseman].fielding.near_home = 0;

    m->players[cover].body.location = field->first_base;
    m->players[cover].body.previous_location = field->first_base;
    m->players[cover].fielding.replacement = REPLACEMENT_ACTIVE;
    m->players[cover].fielding.replacement_base = BASE_FIRST;
    m->players[cover].motion.moving = 0;

    m->ball.holder = baseman;
    m->ball.location = m->players[baseman].body.location;
    m->ball.location.y = BALL_HEIGHT_WITH_PLAYER;
    m->catching.controlled_fielder = baseman;

    AIControllerState ai;
    init_ai_controller(&ai, 0xC07E12u);
    ai_update(m, ctx->state->rules, field, &ai, &ctx->state->channels);

    int sawTargetFirst = 0;
    int sawPower = 0;
    for (int i = 0; i < ctx->state->channels.catching.count; i++) {
        IntentMessage msg = ctx->state->channels.catching.message[i];
        if (msg.kind == INTENT_THROW_TARGET && msg.as.throw_target.base == BASE_FIRST) sawTargetFirst = 1;
        if (msg.kind == INTENT_THROW_POWER) sawPower = 1;
    }
    ASSERT_EQ(1, sawTargetFirst, "off-post baseman must declare INTENT_THROW_TARGET to his own base");
    ASSERT_EQ(1, sawPower, "off-post baseman must declare INTENT_THROW_POWER to his waiting cover");

    // Ingest, actualize, and simulate until the throw releases and the cover catches it on 1st.
    tick(ctx);
    ASSERT_EQ(CATCHING_ACTION_THROWING, (int)m->catching_action.kind, "throw windup begins");

    int caughtByCover = 0;
    for (int f = 0; f < 250; f++) {
        simulate_frames(ctx, 1);
        if (m->ball.holder == cover) {
            caughtByCover = 1;
            break;
        }
    }
    ASSERT_EQ(1, caughtByCover, "thrown ball must be caught by the waiting cover on 1st base");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
