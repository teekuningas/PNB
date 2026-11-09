#include "scenario_builder.h"
#include <math.h>
#include "field.h"
#include "world.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "simulate.h"

/**
 * CONTRACT: SIMULATE asks the catch against the fielders where the PREVIOUS tick left them.
 *
 * The catch is detected between the ball's step and the fielders' step, not after the motion — which is
 * why detection is not a stage of its own. Moving it after the bodies' step would move every catch by a
 * frame. These two hold the order from both sides, with real walking speed across the real reach:
 *
 *   - a fielder just inside reach, walking out of it this tick, still catches this tick;
 *   - a fielder just outside reach, walking into it this tick, does not — he catches on the next.
 *
 * Each drives SIMULATE alone, one tick at a time: no referee, no controller, nothing else that could
 * move a body or the ball between the two questions.
 */

#define REACH_EDGE 2.6f // the catching reach (ball_catch.c's CATCH_DISTANCE)
#define FIRST_FIELDER (PLAYERS_IN_TEAM + JOKER_COUNT)
#define LAST_FIELDER (PLAYERS_IN_TEAM * 2 + JOKER_COUNT)

// A fielder no SIMULATE behaviour will re-target: not a baseman, not a cover, not under control.
static int free_fielder(const MatchSession* m, int not_this)
{
    for (int i = FIRST_FIELDER; i < LAST_FIELDER; i++) {
        int posted = (i == m->catchingState.controlled_fielder) || i == not_this;
        for (int b = 0; b < BASE_COUNT; b++) {
            if (m->fielding_posts.baseman[b] == i || m->fielding_posts.cover[b] == i) posted = 1;
        }
        if (!posted) return i;
    }
    return -1;
}

// A still ball hanging at catching height in open field: no flight step will move it, so the only thing
// that changes between the catch question and the body step is where the fielder is.
static Vector3D hang_ball_in_open_field(MatchSession* m)
{
    Vector3D at = {.x = 0.0f, .y = 0.0f, .z = -60.0f};
    m->ballInfo.location = at;
    m->ballInfo.lastLocation = at;
    m->ballInfo.velocity = (Vector3D){0.0f, 0.0f, 0.0f};
    m->ballInfo.moving = 0;
    m->ballInfo.onGround = 0;
    m->ballInfo.currentFlightHasHitGround = 0;
    m->ballInfo.holder = -1;
    m->ballInfo.released_by = -1;
    m->ballInfo.pitch_in_flight = 0;
    m->chase_replan = CHASE_REPLAN_NONE;
    return at;
}

// Put a fielder `dz` from the ball along z, walking at WALK_SPEED in direction `sign`, toward a target far
// enough ahead that he cannot arrive this tick.
static void walk_fielder(MatchSession* m, int i, Vector3D ball, float dz, float sign)
{
    PlayerInfo* p = &m->playerInfo[i];
    PlayerBody* body = &p->body;
    PlayerMotion* motion = &p->motion;
    body->location = (Vector3D){ball.x, 0.0f, ball.z + dz};
    body->previous_location = body->location;
    body->velocity = (Vector3D){0.0f, 0.0f, sign * WALK_SPEED};
    body->target_location = (Vector3D){ball.x, 0.0f, ball.z + dz + sign * 10.0f};
    motion->moving = 1;
    motion->following_target = 1;
    motion->running = 0;
    p->fielding.busy_catching = 0;
    p->fielding.throw_recovery_ticks = 0;
}

static float xz_distance(const MatchSession* m, int i, Vector3D ball)
{
    float dx = m->playerInfo[i].body.location.x - ball.x, dz = m->playerInfo[i].body.location.z - ball.z;
    return sqrtf(dx * dx + dz * dz);
}

// Nobody but the fielders under test may be anywhere near the ball, or the test proves nothing.
static int nobody_else_near(const MatchSession* m, Vector3D ball, int a, int b)
{
    for (int i = FIRST_FIELDER; i < LAST_FIELDER; i++) {
        if (i != a && i != b && xz_distance(m, i, ball) < 3 * REACH_EDGE) return 0;
    }
    return 1;
}

static void simulate_one_tick(ScenarioContext* ctx)
{
    simulate(ctx->state->match, ctx->state->fieldPositions, &ctx->state->rules->referee, &ctx->state->playSoundEffect);
}

int test_a_fielder_walking_out_of_reach_still_catches_this_tick(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    initialize_referee_from_physical_state(ctx);
    Vector3D ball = hang_ball_in_open_field(m);
    int f = free_fielder(m, -1);
    ASSERT_TRUE(f != -1, "the fixture has a fielder no behaviour re-targets");
    walk_fielder(m, f, ball, REACH_EDGE - 0.05f, +1.0f); // 2.55 out, walking away
    ASSERT_TRUE(nobody_else_near(m, ball, f, -1), "precondition: nobody else near the ball");

    simulate_one_tick(ctx);

    ASSERT_EQ(f, m->ballInfo.holder, "he was in reach where the previous tick left him, so he caught it");
    ASSERT_TRUE(
        fabsf(xz_distance(m, f, ball) - (REACH_EDGE - 0.05f)) < 1e-4f,
        "and the catch stopped him there, before his step could take him out of reach"
    );
    ASSERT_EQ(1, m->gameEvents.catchMade, "and the catch is this frame's event");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

int test_a_fielder_walking_into_reach_catches_on_the_next_tick(void)
{
    ScenarioContext* ctx = create_scenario();
    MatchSession* m = ctx->state->match;
    initialize_referee_from_physical_state(ctx);
    Vector3D ball = hang_ball_in_open_field(m);
    int g = free_fielder(m, -1);
    ASSERT_TRUE(g != -1, "the fixture has a fielder no behaviour re-targets");
    walk_fielder(m, g, ball, REACH_EDGE + 0.05f, -1.0f); // 2.65 out, walking in
    ASSERT_TRUE(nobody_else_near(m, ball, g, -1), "precondition: nobody else near the ball");

    simulate_one_tick(ctx);

    ASSERT_TRUE(xz_distance(m, g, ball) < REACH_EDGE, "precondition held: he is in reach after his step");
    ASSERT_EQ(-1, m->ballInfo.holder, "but the catch was asked before the step, where he was out of reach");
    ASSERT_EQ(0, m->gameEvents.catchMade, "so no catch this frame");

    m->gameEvents.catchMade = 0; // the frame's end, as CLEAR would leave it
    simulate_one_tick(ctx);

    ASSERT_EQ(g, m->ballInfo.holder, "the next tick asks where this one left him: he catches it");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
