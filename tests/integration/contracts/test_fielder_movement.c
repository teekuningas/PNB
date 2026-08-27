#include "scenario_builder.h"
#include "test_helpers.h"
#include "all_contracts.h"
#include "execute_actions.h"
#include "globals.h"
#include <math.h>
#include <string.h>

/**
 * CONTRACT: the controlled fielder's movement is a DESTINATION, and the engine owns the walk.
 *
 * A producer declares INTENT_MOVE_TARGET{point} — a complete current value, held until replaced —
 * and the engine walks the controlled fielder there at RUN_SPEED. The producer never sends a
 * direction, an edge, or a "stop"; "stop" is just the point the fielder is standing on.
 *
 * The load-bearing property is idempotence, and it is the reason the shape was chosen: a message
 * that means the same thing however often it arrives is what makes "declare only when it changes" a
 * lossless compression of a per-tick value stream, and what makes rollback — which repeats the last
 * input it has when the next one is late — correct by construction. It is also not free: a spike
 * whose mover was NOT idempotent oscillated (arrive, stop, be re-sent to the point it stood on,
 * start) and drove the home-run contest into an illegal state.
 *
 * The tests below assert that in three strengths: one frame (the velocity is right), one arrival
 * (it stops and stays stopped), and one whole walk re-declared on every single frame of it (the
 * World comes out byte-identical to the walk that was declared once).
 */

#define FIELDER_INDEX 15

// A world containing exactly one thing worth watching: a controlled fielder standing in the field
// with nobody holding the ball, no pitch, no throw.
static ScenarioContext* fielder_scenario(void)
{
    ScenarioContext* ctx = create_scenario();
    move_pitcher_away(ctx);

    MatchSession* m = ctx->state->match;
    m->pII.controlIndex = FIELDER_INDEX;
    m->pII.hasBallIndex = -1;
    m->playerInfo[FIELDER_INDEX].tPI.location.x = 0.0f;
    m->playerInfo[FIELDER_INDEX].tPI.location.z = 5.0f;

    initialize_referee_from_physical_state(ctx);
    return ctx;
}

static void declare_move_target(ScenarioContext* ctx, float x, float z)
{
    IntentMessage msg = {.kind = INTENT_MOVE_TARGET};
    msg.as.move_target.point.x = x;
    msg.as.move_target.point.z = z;
    intent_push(&ctx->state->channels.catching, msg);
}

// One frame of the ENGINE only — no producers, no physics — so the velocity can be read at the
// instant ingestion and actualization have run and nothing has integrated it yet.
static void ingest_and_actualize(ScenarioContext* ctx)
{
    execute_actions(
        ctx->state->match, ctx->state->rules, ctx->state->fieldPositions, &ctx->state->channels,
        &ctx->state->playSoundEffect
    );
}

static float speed_xz(const MatchSession* m, int index)
{
    float vx = m->playerInfo[index].tPI.velocity.x;
    float vz = m->playerInfo[index].tPI.velocity.z;
    return sqrtf(vx * vx + vz * vz);
}

// The World, captured whole. MatchSession + GameRulesState is the whole of it by definition (the
// tick equation), and capturing it inside ONE scenario keeps the embedded name pointers identical,
// so a byte comparison means what it says.
typedef struct {
    MatchSession match;
    GameRulesState rules;
} WorldCapture;

static void capture_world(const ScenarioContext* ctx, WorldCapture* w)
{
    memcpy(&w->match, ctx->state->match, sizeof(MatchSession));
    memcpy(&w->rules, ctx->state->rules, sizeof(GameRulesState));
}

static void restore_world(ScenarioContext* ctx, const WorldCapture* w)
{
    memcpy(ctx->state->match, &w->match, sizeof(MatchSession));
    memcpy(ctx->state->rules, &w->rules, sizeof(GameRulesState));
}

static int worlds_identical(const WorldCapture* a, const WorldCapture* b)
{
    return memcmp(&a->match, &b->match, sizeof(MatchSession)) == 0 &&
           memcmp(&a->rules, &b->rules, sizeof(GameRulesState)) == 0;
}

// 1. One message, one frame: the fielder sets off toward the point, at RUN_SPEED and not the
//    auto-fielders' WALK_SPEED. The speed assertion is the one that would have caught the obvious
//    wrong implementation — reusing move_to_target, which walks at half the pace and never hands a
//    carried ball its velocity.
int test_move_target_sets_velocity_toward_the_point(void)
{
    ScenarioContext* ctx = fielder_scenario();
    MatchSession* m = ctx->state->match;

    declare_move_target(ctx, 0.0f, 25.0f); // straight ahead in +z
    ingest_and_actualize(ctx);

    ASSERT_EQ(1, m->playerInfo[FIELDER_INDEX].cPI.moving, "a declared destination starts the fielder moving");
    ASSERT_FLOAT_EQ(RUN_SPEED, speed_xz(m, FIELDER_INDEX), 1e-5f, "the controlled fielder runs, it does not walk");
    ASSERT(m->playerInfo[FIELDER_INDEX].tPI.velocity.z > 0.0f, "and it is heading toward the point, not away");
    ASSERT_FLOAT_EQ(0.0f, m->playerInfo[FIELDER_INDEX].tPI.velocity.x, 1e-5f, "no sideways drift on a straight line");
    ASSERT_EQ(1, m->ballInfo.needsMoveUpdate, "a change of the carrier's velocity has to be offered to a carried ball");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 2. The destination the fielder is already standing on is a no-op — it does not start a walk, and
//    re-sending it does not start one either. This is the oscillation the spike measured.
int test_move_target_at_the_fielder_is_a_no_op(void)
{
    ScenarioContext* ctx = fielder_scenario();
    MatchSession* m = ctx->state->match;
    Vector3D where = m->playerInfo[FIELDER_INDEX].tPI.location;

    for (int i = 0; i < 10; i++) {
        declare_move_target(ctx, where.x, where.z);
        ingest_and_actualize(ctx);
        ASSERT_EQ(0, m->playerInfo[FIELDER_INDEX].cPI.moving, "a fielder sent where it stands does not set off");
        ASSERT_FLOAT_EQ(0.0f, speed_xz(m, FIELDER_INDEX), 1e-6f, "and has no velocity to integrate");
    }

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 3. The walk ends by itself: the fielder arrives, stops, and stays stopped with the destination
//    still standing. Nothing has to send a "stop".
int test_move_target_stops_on_arrival(void)
{
    ScenarioContext* ctx = fielder_scenario();
    MatchSession* m = ctx->state->match;

    declare_move_target(ctx, 0.0f, 10.0f); // 5 units away: ~42 frames at RUN_SPEED
    simulate_frames(ctx, 200);

    ASSERT_EQ(0, m->playerInfo[FIELDER_INDEX].cPI.moving, "the fielder stops when it gets there");
    float dz = m->playerInfo[FIELDER_INDEX].tPI.location.z - 10.0f;
    float dx = m->playerInfo[FIELDER_INDEX].tPI.location.x;
    ASSERT(sqrtf(dx * dx + dz * dz) <= TARGET_ACHIEVED_THRESHOLD, "and it stops AT the point it was sent to");
    ASSERT_EQ(
        1, m->catchingState.controlledMoveTargetActive,
        "the destination is held, not consumed — arriving is not the same as being cancelled"
    );

    // And now the hazard itself: a producer that keeps restating the destination it already
    // reached. This is what the non-idempotent spike did on every tick — arrive, stop, be sent to
    // the point it was standing on, start — and that oscillation alone was enough to push the
    // home-run contest out of a legal state. The fielder must not twitch, and must not drift.
    Vector3D settled = m->playerInfo[FIELDER_INDEX].tPI.location;
    for (int i = 0; i < 200; i++) {
        declare_move_target(ctx, 0.0f, 10.0f);
        simulate_frames(ctx, 1);
        ASSERT_EQ(0, m->playerInfo[FIELDER_INDEX].cPI.moving, "restating a reached destination never restarts a walk");
    }
    float driftX = m->playerInfo[FIELDER_INDEX].tPI.location.x - settled.x;
    float driftZ = m->playerInfo[FIELDER_INDEX].tPI.location.z - settled.z;
    ASSERT_FLOAT_EQ(0.0f, sqrtf(driftX * driftX + driftZ * driftZ), 1e-6f, "and the fielder does not creep");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 4. REPEAT SAFETY, the whole claim in one assertion: a walk declared ONCE and the same walk
//    re-declared on every single frame of it produce a byte-identical World.
//
//    This is the mechanism that holds the message law ("never anything that means something
//    different when applied twice") rather than vigilance holding it. A rollback re-applies the last
//    input it has for every frame it re-simulates; if that were not a no-op, every rolled-back frame
//    would be a divergence. It is written over the whole World, so it will keep catching the
//    violation when some future intent introduces one.
int test_restating_the_move_target_changes_nothing(void)
{
    ScenarioContext* ctx = fielder_scenario();

    WorldCapture start, once, restated;
    capture_world(ctx, &start);

    // Declared once, then left alone for the whole walk.
    declare_move_target(ctx, 6.0f, 20.0f);
    simulate_frames(ctx, 120);
    capture_world(ctx, &once);

    // The same walk, restated every frame of it.
    restore_world(ctx, &start);
    for (int i = 0; i < 120; i++) {
        declare_move_target(ctx, 6.0f, 20.0f);
        simulate_frames(ctx, 1);
    }
    capture_world(ctx, &restated);

    ASSERT(worlds_identical(&once, &restated), "restating a destination on every frame must change nothing at all");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 5. And the same law within a single frame: two identical messages in one tick are one message.
//    Same-kind duplicates are last-write-wins by construction (globals.h), which for a value that
//    equals itself means the second one is free.
int test_duplicate_messages_in_one_frame_equal_one(void)
{
    ScenarioContext* ctx = fielder_scenario();

    WorldCapture start, single, doubled;
    capture_world(ctx, &start);

    declare_move_target(ctx, -8.0f, 14.0f);
    simulate_frames(ctx, 30);
    capture_world(ctx, &single);

    restore_world(ctx, &start);
    declare_move_target(ctx, -8.0f, 14.0f);
    declare_move_target(ctx, -8.0f, 14.0f);
    declare_move_target(ctx, -8.0f, 14.0f);
    simulate_frames(ctx, 30);
    capture_world(ctx, &doubled);

    ASSERT(worlds_identical(&single, &doubled), "three copies of one message in one frame are one message");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}

// 6. With nobody controlled there is nobody to send anywhere, and the gate says so rather than
//    storing a destination against a fielder that does not exist.
int test_move_target_refused_without_a_controlled_fielder(void)
{
    ScenarioContext* ctx = fielder_scenario();
    MatchSession* m = ctx->state->match;
    m->pII.controlIndex = -1;

    declare_move_target(ctx, 0.0f, 25.0f);
    ingest_and_actualize(ctx);

    ASSERT_EQ(
        0, m->catchingState.controlledMoveTargetActive, "a refused message leaves no destination behind in the engine"
    );
    ASSERT_EQ(0, ctx->state->channels.catching.count, "and the gate still drained it — a refusal is not a leak");

    cleanup_scenario(ctx);
    return TEST_PASSED;
}
