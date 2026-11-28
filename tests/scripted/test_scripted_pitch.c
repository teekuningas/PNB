#include "test_helpers.h"
#include "actions/batting_system.h"
#include "scripted_harness.h"
#include <stdio.h>
#include "key_states.h"
#include "world.h"
#include "client_input.h"
#include "action_invocations.h" // catching_widget_view — what the pitcher actually sees on the bar
#include "rules_pure/player_utils.h"
#include <math.h>

/*
 * Scripted-human PITCH test — the human input→intent path for the pitch slice, driven
 * entirely through the real action_invocations via scripted KeyStates (no AI, no direct state pokes).
 *
 * It proves the two-ping-pong meter dance: press → start the power meter sweeping; click during the sweep
 * → declare power AND start the windup (power-as-start); click during the aim sweep → declare aim (and the
 * engine releases a real pitch). Timing the aim click on the early/low part of the sweep keeps the
 * direction near the plate (a strike). Dropping the aim click → the aim meter runs out and the engine
 * windup elapses → valesyöttö (a fake; the pitcher keeps the ball). The pitching team is the human here.
 */

#define HUMAN_PAD CONTROL_PLAYER_1 /* the catching (pitching) team is the human */

// Tick until the human controls the pitcher, holding the ball at home, with a ready batter and no
// catching action in progress — i.e. a windup may legally begin.
static int tick_until_pitch_ready(ScriptedGame* g, int budget)
{
    for (int i = 0; i < budget; i++) {
        scripted_tick(g);
        MatchSession* m = scripted_match(g);
        int p = m->fielding_posts.baseman[0];
        if (p != -1 && m->ball.holder == p && m->catching.controlled_fielder == p && batter_stands_ready(&m->at_bat) &&
            m->catching_action.kind == CATCHING_ACTION_NONE && !m->ball.pitch_in_flight) {
            return 1;
        }
    }
    return 0;
}

// Tick until the human's pitch sampling widget is actively sweeping (one ping-pong is running), so the
// next click lands a value rather than no-op'ing. Returns 0 if the meter never starts.
static int tick_until_widget_sweeping(ScriptedGame* g, int budget)
{
    ClientInputState* ci = scripted_state(g)->client_input;
    for (int i = 0; i < budget; i++) {
        if (ci->pitch_widget.meter.dir != 0) return 1;
        scripted_tick(g);
    }
    return ci->pitch_widget.meter.dir != 0;
}

int test_scripted_pitch_meter_waits_for_the_batter_to_arrive(void)
{
    ScriptedGame* g = scripted_create(0, 1, CONTROL_PLAYER_2, HUMAN_PAD, 0x9117C405u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(scripted_tick_until_batter_decision(g, 1200), "the batting side never received its selection offer");
    scripted_tap(g, CONTROL_PLAYER_2, KEY_2);

    MatchSession* m = scripted_match(g);
    const ClientInputState* ci = scripted_state(g)->client_input;
    int batter = get_active_batter_index(m);
    int pitcher = m->fielding_posts.baseman[0];
    ASSERT_NE(-1, batter, "the selected batter has taken the bat");
    ASSERT_NE(-1, pitcher, "the formation has a pitcher");
    ASSERT_EQ(pitcher, m->ball.holder, "the pitcher already holds the ball");
    ASSERT_EQ(pitcher, m->catching.controlled_fielder, "the pitching pad controls the pitcher");
    ASSERT_EQ(1, m->players[pitcher].fielding.near_home, "only batter readiness prevents pitching");
    ASSERT_EQ(1, m->players[batter].motion.moving, "the selected batter is still walking to the plate");
    ASSERT(!batter_stands_ready(&m->at_bat), "selection alone does not make the batter ready");

    for (int attempt = 0; attempt < 3; attempt++) {
        scripted_tap(g, HUMAN_PAD, KEY_2);
        ASSERT(!batter_stands_ready(&m->at_bat), "the early presses occur during the real approach");
        ASSERT_EQ(WIDGET_IDLE, ci->pitch_widget.meter.mode, "an early pitch press must not start a power sweep");
        ASSERT_FLOAT_EQ(0.0f, catching_widget_view(ci), 1e-6f, "the catching bar stays at rest");
        ASSERT_EQ(CATCHING_ACTION_NONE, m->catching_action.kind, "no pitch starts while the batter approaches");
    }

    scripted_hold(g, HUMAN_PAD, KEY_2);
    ASSERT(tick_until_pitch_ready(g, 1200), "the batter never physically arrived ready to face a pitch");
    ASSERT_EQ(WIDGET_IDLE, ci->pitch_widget.meter.mode, "holding an early press does not queue a sweep on arrival");
    scripted_release(g, HUMAN_PAD, KEY_2);
    ASSERT(scripted_tick(g), "the game must remain live after arrival");
    scripted_tap(g, HUMAN_PAD, KEY_2);
    ASSERT_EQ(WIDGET_PING_PONG, ci->pitch_widget.meter.mode, "a fresh press after arrival starts the power sweep");
    ASSERT(catching_widget_view(ci) > 0.0f, "the ready pitcher's meter really advances");
    scripted_tap(g, HUMAN_PAD, KEY_2);
    ASSERT_EQ(CATCHING_ACTION_PITCHING, m->catching_action.kind, "the next press still starts the real windup");
    ASSERT_EQ(WIDGET_DESCENT, ci->pitch_widget.meter.mode, "committing power still starts the aim descent");
    ASSERT(!scripted_failed(g), scripted_fail_reason(g));
    scripted_destroy(g);
    return TEST_PASSED;
}

// 1. Press → power click → aim click (timed at the sweet-spot) → a real strike-zone pitch is released.
int test_scripted_pitch_two_pingpongs_strike(void)
{
    ScriptedGame* g = scripted_create(0, 1, CONTROL_AI, HUMAN_PAD, 0x9117C401u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_pitch_ready(g, 8000), "never reached a human pitch-ready state");

    scripted_tap(g, HUMAN_PAD, KEY_2); // press: start the power meter sweeping (nothing declared yet)
    ASSERT(tick_until_widget_sweeping(g, 8), "power meter did not start sweeping after the start press");
    scripted_tap(
        g, HUMAN_PAD, KEY_2
    ); // click during the power sweep: lock power → POWER, windup begins, aim meter starts

    // The aim meter descends from the right; tick until the cursor falls near the strike sweet-spot (≈FOCAL
    // from the left), then click → a strike-zone aim.
    ClientInputState* ci = scripted_state(g)->client_input;
    int guard = 0;
    while (ci->pitch_widget.meter.dir != 0 &&
           ci->pitch_widget.meter.counter * 100 > ci->pitch_widget.meter.counter_max * 35 && guard++ < 400) {
        scripted_tick(g);
    }
    scripted_tap(g, HUMAN_PAD, KEY_2); // click during the aim descent: lock aim near the plate (strike)

    // Tick to the windup end → release; capture the launch the frame the ball goes airborne.
    MatchSession* m = scripted_match(g);
    float launch_vx = 999.0f;
    int released = 0;
    for (int i = 0; i < 200 && !released; i++) {
        scripted_tick(g);
        if (m->ball.pitch_in_flight) {
            launch_vx = m->ball.velocity.x;
            released = 1;
        }
    }

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT(released, "the power+aim dance must release a real pitch");
    // Sweet-spot-timed aim → a strike-zone pitch (direction near 0 → small horizontal velocity).
    ASSERT(fabs(launch_vx) < 0.1f, "sweet-spot-timed aim click must aim a strike-zone pitch (small vx)");
    return TEST_PASSED;
}

// 2. Press → power click → no aim click → valesyöttö: the pitcher keeps the ball, no pitch is released.
int test_scripted_pitch_dropped_aim_is_valesyotto(void)
{
    ScriptedGame* g = scripted_create(0, 1, CONTROL_AI, HUMAN_PAD, 0x9117C402u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_pitch_ready(g, 8000), "never reached a human pitch-ready state");

    MatchSession* m = scripted_match(g);
    int pitcher = m->fielding_posts.baseman[0];

    scripted_tap(g, HUMAN_PAD, KEY_2); // press: start the power meter sweeping
    ASSERT(tick_until_widget_sweeping(g, 8), "power meter did not start sweeping after the start press");
    scripted_tap(g, HUMAN_PAD, KEY_2); // click during the power sweep: lock power → POWER, windup begins
    // Deliberately never click the aim. Tick well past the (power-dependent) windup deadline.
    int wentAirborne = 0;
    for (int i = 0; i < 250; i++) {
        scripted_tick(g);
        if (m->ball.pitch_in_flight) wentAirborne = 1;
    }

    int keptBall = (m->ball.holder == pitcher);
    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT(!wentAirborne, "dropping the aim must NOT release a pitch");
    ASSERT(keptBall, "valesyöttö: the pitcher keeps the ball");
    return TEST_PASSED;
}

// 3. After a HUMAN pitch, the AI batter's hit must actually FLY — regression guard for the bug where the
// pitch widget stayed "active" past release (checkPitch goes quiet once the ball leaves the hand), so
// the shared meter kept showing it and never advanced the batting meter → the swing built ~0 power → the bat
// "hit" the ball with near-zero velocity and it floated at the plate. AI-vs-AI never hit this (no widget),
// so only a human-pitch → AI-bat path catches it.
int test_scripted_human_pitch_ai_hit_flies(void)
{
    ScriptedGame* g = scripted_create(0, 1, CONTROL_AI, HUMAN_PAD, 0x9117C403u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_pitch_ready(g, 8000), "never reached a human pitch-ready state");
    ClientInputState* ci = scripted_state(g)->client_input;
    MatchSession* m = scripted_match(g);

    // Pitch a medium-power, centred strike (the band the AI hits well): start the power meter, click near
    // mid-sweep for ~0.5 power, then click the aim at the sweet-spot.
    scripted_tap(g, HUMAN_PAD, KEY_2);
    ASSERT(tick_until_widget_sweeping(g, 8), "power meter did not start sweeping");
    int guard = 0;
    while (ci->pitch_widget.meter.dir == 1 &&
           ci->pitch_widget.meter.counter * 100 < ci->pitch_widget.meter.counter_max * 50 && guard++ < 200) {
        scripted_tick(g);
    }
    scripted_tap(g, HUMAN_PAD, KEY_2); // lock ~mid power → windup begins
    guard = 0;
    while (ci->pitch_widget.meter.dir != 0 &&
           ci->pitch_widget.meter.counter * 100 > ci->pitch_widget.meter.counter_max * 35 && guard++ < 400) {
        scripted_tick(g);
    }
    scripted_tap(g, HUMAN_PAD, KEY_2); // lock aim near the plate (strike)

    // Wait for release, then watch the ball: a real hit sends it flying (large horizontal speed); the bug
    // leaves it floating (horizontal speed ≈ the pitch's ≈0). No one fields it (the catching team is the
    // idle human), so only the bat can give it horizontal speed.
    int airborne = 0;
    float max_horiz_speed = 0.0f;
    for (int i = 0; i < 400; i++) {
        scripted_tick(g);
        if (m->ball.pitch_in_flight) airborne = 1;
        if (airborne) {
            float vx = m->ball.velocity.x, vz = m->ball.velocity.z;
            float horiz = sqrtf(vx * vx + vz * vz);
            if (horiz > max_horiz_speed) max_horiz_speed = horiz;
        }
    }

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT(airborne, "the human pitch must release");
    // A floated hit has horizontal speed ≈ the pitch's (~0); a real batted ball is many times faster.
    ASSERT(max_horiz_speed > 0.15f, "the AI's hit off a human pitch must FLY, not float at the plate");
    return TEST_PASSED;
}

// 4. The catching side's bar, as the human pitcher sees it, across one whole gesture. It is answered by
// the input layer from the gesture itself (catching_widget_view) — there is no engine copy of it any
// more — so this holds what that copy used to promise, every frame: the bar rests at its left end until
// the human starts, the cursor IS the sweep's level while it rises, the aim cursor descends from the far
// end once power is locked, and the bar is back at rest once the engine has resolved the pitch.
static float widget_level(const InputWidget* w)
{
    return (float)w->counter / (float)w->counter_max;
}

int test_scripted_catching_bar_follows_the_pitch_gesture(void)
{
    ScriptedGame* g = scripted_create(0, 1, CONTROL_AI, HUMAN_PAD, 0x9117C404u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(tick_until_pitch_ready(g, 8000), "never reached a human pitch-ready state");
    const ClientInputState* ci = scripted_state(g)->client_input;

    ASSERT_FLOAT_EQ(0.0f, catching_widget_view(ci), 1e-6f, "before the gesture the bar rests at its left end");

    scripted_tap(g, HUMAN_PAD, KEY_2); // start the power sweep
    ASSERT(tick_until_widget_sweeping(g, 8), "power meter did not start sweeping after the start press");
    float previous = catching_widget_view(ci);
    int rose = 0;
    for (int i = 0; i < 6; i++) {
        scripted_tick(g);
        const float now = catching_widget_view(ci);
        ASSERT_FLOAT_EQ(widget_level(&ci->pitch_widget.meter), now, 1e-6f, "the bar shows the power sweep's own level");
        if (now > previous) rose = 1;
        previous = now;
    }
    ASSERT(rose, "the power cursor moves while it sweeps");

    scripted_tap(g, HUMAN_PAD, KEY_2); // lock power: the windup begins and the aim descent arms
    ASSERT_EQ(WIDGET_DESCENT, (int)ci->pitch_widget.meter.mode, "locking power arms the aim descent");
    previous = catching_widget_view(ci);
    int fell = 0;
    for (int i = 0; i < 6; i++) {
        scripted_tick(g);
        const float now = catching_widget_view(ci);
        ASSERT_FLOAT_EQ(widget_level(&ci->pitch_widget.meter), now, 1e-6f, "the bar shows the aim descent's own level");
        if (now < previous) fell = 1;
        previous = now;
    }
    ASSERT(fell, "the aim cursor descends");

    // Never click the aim: the descent runs out, the engine fakes the pitch and clears it, and the
    // gesture is over. The bar must come back to rest rather than hang on the last level it showed.
    int rested = 0;
    for (int i = 0; i < 300 && !rested; i++) {
        scripted_tick(g);
        rested = (ci->pitch_widget.meter.mode == WIDGET_IDLE && catching_widget_view(ci) == 0.0f);
    }

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);

    ASSERT(!failed, reason);
    ASSERT(rested, "once the pitch is resolved the bar is back at its left end");
    return TEST_PASSED;
}

// 5. §24 through the real human input path: after Batter 1 takes a free walk from home (1 wrong
//    pitch on an empty field -> §18(3)) and Batter 2 arrives at the plate, pressing KEY_2 must NOT
//    arm the pitch widget while pitch_rights == PITCH_RIGHTS_NEED_FIELD_BASE. Throwing Home -> 1st
//    -> Home with real keys restores PITCH_RIGHTS_GRANTED, after which KEY_2 arms and pitches.
int test_scripted_pitch_blocked_until_pitch_rights_reacquired(void)
{
    ScriptedGame* g = scripted_create(0, 1, CONTROL_PLAYER_2, HUMAN_PAD, 0x9117C406u);
    ASSERT_NOT_NULL(g, "scripted_create returned NULL");
    ASSERT(scripted_tick_until_batter_decision(g, 1200), "Batter 1 seat offer never opened");
    scripted_tap(g, CONTROL_PLAYER_2, KEY_2);
    ASSERT(tick_until_pitch_ready(g, 1200), "Batter 1 never became ready");

    MatchSession* m = scripted_match(g);
    const GameRulesState* r = scripted_state(g)->rules;
    const ClientInputState* ci = scripted_state(g)->client_input;
    const int pitcher = m->fielding_posts.baseman[0];
    const int first_baseman = m->fielding_posts.baseman[1];

    // Pitch an intentional wrong pitch (ball): click aim immediately at the top of the descent so
    // direction is far off the plate -> empty field -> 1-ball free-walk offer for Batter 1.
    scripted_tap(g, HUMAN_PAD, KEY_2);
    ASSERT(tick_until_widget_sweeping(g, 8), "power meter started");
    scripted_tap(g, HUMAN_PAD, KEY_2); // lock low power, start windup + aim descent
    scripted_tap(g, HUMAN_PAD, KEY_2); // immediate aim click at far right -> off-plate BALL

    int walk_offered = 0;
    for (int i = 0; i < 400; i++) {
        scripted_tick(g);
        if (m->offers.free_walk == 1) {
            walk_offered = 1;
            break;
        }
    }
    ASSERT_EQ(1, walk_offered, "off-plate pitch on empty field must offer a §26 free walk");

    // Batter 1 accepts the free walk (§18(3)).
    scripted_tap(g, CONTROL_PLAYER_2, KEY_2);

    // Wait for the pitcher to pick up the loose ball at home (walking left/right if needed, or
    // auto-chasing) and for Batter 2's seat offer to open.
    int ball_back_home = 0;
    for (int i = 0; i < 600; i++) {
        scripted_tick(g);
        if (m->ball.holder == -1 && m->catching.controlled_fielder != -1) {
            // Walk controlled fielder toward the ball if needed.
            float dx = m->ball.location.x - m->players[m->catching.controlled_fielder].body.location.x;
            float dz = m->ball.location.z - m->players[m->catching.controlled_fielder].body.location.z;
            if (dx > 0.5f)
                scripted_hold(g, HUMAN_PAD, KEY_RIGHT);
            else
                scripted_release(g, HUMAN_PAD, KEY_RIGHT);
            if (dx < -0.5f)
                scripted_hold(g, HUMAN_PAD, KEY_LEFT);
            else
                scripted_release(g, HUMAN_PAD, KEY_LEFT);
            if (dz > 0.5f)
                scripted_hold(g, HUMAN_PAD, KEY_DOWN);
            else
                scripted_release(g, HUMAN_PAD, KEY_DOWN);
            if (dz < -0.5f)
                scripted_hold(g, HUMAN_PAD, KEY_UP);
            else
                scripted_release(g, HUMAN_PAD, KEY_UP);
        } else {
            scripted_release(g, HUMAN_PAD, KEY_LEFT);
            scripted_release(g, HUMAN_PAD, KEY_RIGHT);
            scripted_release(g, HUMAN_PAD, KEY_UP);
            scripted_release(g, HUMAN_PAD, KEY_DOWN);
        }
        if (m->offers.batter_seat == 1) {
            scripted_tap(g, CONTROL_PLAYER_2, KEY_2); // seat Batter 2
        }
        if (m->ball.holder == pitcher && m->players[pitcher].fielding.near_home == 1 &&
            batter_stands_ready(&m->at_bat)) {
            ball_back_home = 1;
            break;
        }
        // If pitcher caught ball slightly off plate, walk back toward pitcher post.
        if (m->ball.holder == pitcher && m->players[pitcher].fielding.near_home == 0) {
            float dx = m->players[pitcher].body.home_location.x - m->players[pitcher].body.location.x;
            float dz = m->players[pitcher].body.home_location.z - m->players[pitcher].body.location.z;
            if (dx > 0.4f)
                scripted_hold(g, HUMAN_PAD, KEY_RIGHT);
            else
                scripted_release(g, HUMAN_PAD, KEY_RIGHT);
            if (dx < -0.4f)
                scripted_hold(g, HUMAN_PAD, KEY_LEFT);
            else
                scripted_release(g, HUMAN_PAD, KEY_LEFT);
            if (dz > 0.4f)
                scripted_hold(g, HUMAN_PAD, KEY_DOWN);
            else
                scripted_release(g, HUMAN_PAD, KEY_DOWN);
            if (dz < -0.4f)
                scripted_hold(g, HUMAN_PAD, KEY_UP);
            else
                scripted_release(g, HUMAN_PAD, KEY_UP);
        }
    }
    scripted_release(g, HUMAN_PAD, KEY_LEFT);
    scripted_release(g, HUMAN_PAD, KEY_RIGHT);
    scripted_release(g, HUMAN_PAD, KEY_UP);
    scripted_release(g, HUMAN_PAD, KEY_DOWN);
    scripted_tick(g);

    ASSERT_EQ(1, ball_back_home, "pitcher recovered ball at home with Batter 2 ready");
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_FIELD_BASE, (int)r->half_inning.pitch_rights,
        "after Batter 1 free walk + ball at home, §24 requires a field-base visit"
    );

    // Pressing KEY_2 MUST NOT arm the pitch widget while pitch_rights != GRANTED!
    scripted_tap(g, HUMAN_PAD, KEY_2);
    ASSERT_EQ(WIDGET_IDLE, (int)ci->pitch_widget.meter.mode, "§24 blocks pitch widget until Home -> Base -> Home");
    ASSERT_EQ(CATCHING_ACTION_NONE, (int)m->catching_action.kind, "no pitch windup starts without pitch rights");

    // Throw to 1st base (KEY_LEFT + KEY_2 hold, then release).
    scripted_hold(g, HUMAN_PAD, KEY_LEFT);
    scripted_hold(g, HUMAN_PAD, KEY_2);
    for (int i = 0; i < 20; i++)
        scripted_tick(g);
    scripted_release(g, HUMAN_PAD, KEY_2);
    scripted_release(g, HUMAN_PAD, KEY_LEFT);

    int caught_at_first = 0;
    for (int i = 0; i < 300; i++) {
        scripted_tick(g);
        if (m->ball.holder == first_baseman) {
            caught_at_first = 1;
            break;
        }
    }
    ASSERT_EQ(1, caught_at_first, "1B caught the pitch-rights throw from home");
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_RETURN_HOME, (int)r->half_inning.pitch_rights,
        "1st base catch advances pitch_rights -> NEED_RETURN_HOME"
    );

    // Throw back to home (KEY_DOWN + KEY_2 hold, then release).
    scripted_hold(g, HUMAN_PAD, KEY_DOWN);
    scripted_hold(g, HUMAN_PAD, KEY_2);
    for (int i = 0; i < 20; i++)
        scripted_tick(g);
    scripted_release(g, HUMAN_PAD, KEY_2);
    scripted_release(g, HUMAN_PAD, KEY_DOWN);

    int returned_home = 0;
    for (int i = 0; i < 300; i++) {
        scripted_tick(g);
        if (m->ball.holder == pitcher) {
            returned_home = 1;
            break;
        }
    }
    ASSERT_EQ(1, returned_home, "pitcher caught the return throw from 1st");
    ASSERT_EQ(PITCH_RIGHTS_GRANTED, (int)r->half_inning.pitch_rights, "return home grants §24 pitch rights");

    // Now pressing KEY_2 arms the pitch widget and starts a windup!
    scripted_tap(g, HUMAN_PAD, KEY_2);
    ASSERT_EQ(WIDGET_PING_PONG, (int)ci->pitch_widget.meter.mode, "KEY_2 arms pitch widget once pitch rights granted");
    scripted_tap(g, HUMAN_PAD, KEY_2);
    ASSERT_EQ(CATCHING_ACTION_PITCHING, (int)m->catching_action.kind, "second press starts real pitch windup");

    int failed = scripted_failed(g);
    char reason[SIM_FAIL_REASON_LEN];
    snprintf(reason, sizeof(reason), "%s", scripted_fail_reason(g));
    scripted_destroy(g);
    ASSERT(!failed, reason);
    return TEST_PASSED;
}
