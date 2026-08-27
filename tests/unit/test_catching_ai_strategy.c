#include "test_helpers.h"
#include "catching_ai_strategy.h"
#include "base_logic.h"
#include <math.h>

static Vector3D at(float x, float z)
{
    Vector3D v = {0};
    v.x = x;
    v.z = z;
    return v;
}

static float dist_xz(Vector3D a, Vector3D b)
{
    float dx = a.x - b.x, dz = a.z - b.z;
    return sqrtf(dx * dx + dz * dz);
}

// The chase: far from the predicted meeting point, go to it; near it, "go" where you already are,
// which is this controller's way of saying stop without there being a stop message to send.
int test_chase_point_outside_the_dead_zone_is_the_prediction(void)
{
    Vector3D fielder = at(0.0f, 0.0f);
    Vector3D predicted = at(10.0f, 4.0f);

    Vector3D target = chase_point(&fielder, &predicted);

    ASSERT_FLOAT_EQ(predicted.x, target.x, 1e-6f, "a distant prediction is the destination, unmodified");
    ASSERT_FLOAT_EQ(predicted.z, target.z, 1e-6f, "a distant prediction is the destination, unmodified");
    return TEST_PASSED;
}

int test_chase_point_inside_the_dead_zone_is_a_stop(void)
{
    Vector3D fielder = at(3.0f, -2.0f);
    Vector3D predicted = at(3.0f + AI_MOVE_DEAD_ZONE * 0.5f, -2.0f);

    Vector3D target = chase_point(&fielder, &predicted);

    ASSERT_FLOAT_EQ(fielder.x, target.x, 1e-6f, "close enough means stop where you stand");
    ASSERT_FLOAT_EQ(fielder.z, target.z, 1e-6f, "close enough means stop where you stand");
    return TEST_PASSED;
}

// The carry. The stand-off is two-sided on purpose: walking IN when the carrier is far, and back
// OUT when it picked the ball up on top of the base. Inside THROW_TO_BASE_DISTANCE the engine
// refuses the throw and there is no hand-over, so a carrier that stays put holds the ball forever.
int test_carry_to_throw_point_walks_in_from_far_away(void)
{
    Vector3D base = at(0.0f, 0.0f);
    Vector3D carrier = at(30.0f, 0.0f);

    Vector3D target = carry_to_throw_point(&carrier, &base);

    ASSERT_FLOAT_EQ(AI_THROW_STANDOFF, dist_xz(target, base), 1e-4f, "it stops exactly at the stand-off");
    ASSERT(target.x > 0.0f, "and on the carrier's side of the base, not through it");
    return TEST_PASSED;
}

int test_carry_to_throw_point_backs_out_when_too_close(void)
{
    Vector3D base = at(0.0f, 0.0f);
    Vector3D carrier = at(0.4f, 0.0f); // inside the refusal radius: no throw, no hand-over

    Vector3D target = carry_to_throw_point(&carrier, &base);

    ASSERT_FLOAT_EQ(AI_THROW_STANDOFF, dist_xz(target, base), 1e-4f, "it steps back out to a throwable range");
    ASSERT(dist_xz(target, base) > THROW_TO_BASE_DISTANCE, "which is the whole point: the throw must be possible");
    ASSERT(target.x > 0.0f, "and it backs away along the line it came in on");
    return TEST_PASSED;
}

int test_carry_to_throw_point_standing_on_the_base_still_yields_a_throwable_spot(void)
{
    Vector3D base = at(-12.0f, 7.0f);
    Vector3D carrier = base; // no direction to step back along

    Vector3D target = carry_to_throw_point(&carrier, &base);

    ASSERT_FLOAT_EQ(AI_THROW_STANDOFF, dist_xz(target, base), 1e-4f, "a fixed fallback direction, but a real one");
    return TEST_PASSED;
}

// The cadence. Saying nothing is the normal case: the engine already holds a value that close, and
// re-sending it would change nothing.
int test_move_declaration_speaks_first_time_and_on_a_real_drift(void)
{
    Vector3D want = at(5.0f, 5.0f);
    Vector3D last = at(5.0f, 5.0f);

    MoveDeclaration first = decide_move_declaration(&want, 0, &last, 0);
    ASSERT_EQ(1, first.declare, "nothing said yet, so say it");

    MoveDeclaration unchanged = decide_move_declaration(&want, 1, &last, 0);
    ASSERT_EQ(0, unchanged.declare, "the engine already holds this exact value");

    Vector3D nudged = at(5.0f + AI_MOVE_DECLARE_THRESHOLD * 0.5f, 5.0f);
    MoveDeclaration small = decide_move_declaration(&nudged, 1, &last, 0);
    ASSERT_EQ(0, small.declare, "a drift under the threshold is not worth a message");

    Vector3D moved = at(5.0f + AI_MOVE_DECLARE_THRESHOLD * 2.0f, 5.0f);
    MoveDeclaration big = decide_move_declaration(&moved, 1, &last, 0);
    ASSERT_EQ(1, big.declare, "a real drift is");
    ASSERT_FLOAT_EQ(moved.x, big.point.x, 1e-6f, "and what it says is the destination it was given");
    return TEST_PASSED;
}

// The blind heartbeat: it observes nothing, so it cannot become a divergence source, and it is what
// gets the fielder moving again after a reset emptied the engine's destination.
int test_move_declaration_heartbeat_restates_an_unchanged_destination(void)
{
    Vector3D want = at(-3.0f, 8.0f);
    Vector3D last = at(-3.0f, 8.0f);

    MoveDeclaration quiet = decide_move_declaration(&want, 1, &last, AI_MOVE_HEARTBEAT_FRAMES - 1);
    ASSERT_EQ(0, quiet.declare, "not due yet");

    MoveDeclaration due = decide_move_declaration(&want, 1, &last, AI_MOVE_HEARTBEAT_FRAMES);
    ASSERT_EQ(1, due.declare, "due: restate it, having looked at nothing to decide so");
    return TEST_PASSED;
}

int test_should_ai_throw_normal(void)
{
    PlayerIndexInfo pii = {0};
    pii.hasBallIndex = 0;
    pii.catcherOnBaseIndex[0] = 1;
    // hasBallIndex != catcherIndex, catcherNearHome = 1
    int result = should_ai_throw(&pii, 1, 99, 0, 0, 0, 0);
    ASSERT_EQ(1, result, "Should throw if catcher is near home and doesn't have ball");

    // catcherNearHome = 0
    result = should_ai_throw(&pii, 0, 99, 0, 0, 0, 0);
    ASSERT_EQ(0, result, "Should not throw if catcher not near home");

    // hasBallIndex == catcherIndex
    pii.hasBallIndex = 1;
    result = should_ai_throw(&pii, 1, 99, 0, 0, 0, 0);
    ASSERT_EQ(0, result, "Should not throw if catcher has ball");

    return TEST_PASSED;
}

int test_should_ai_throw_replacer(void)
{
    PlayerIndexInfo pii = {0};
    pii.hasBallIndex = 0;
    pii.catcherOnBaseIndex[2] = 1;

    int result = should_ai_throw(&pii, 0, 1, 1, 2, 0, 2);
    ASSERT_EQ(1, result, "Should throw to replacer in position");

    result = should_ai_throw(&pii, 0, 1, 1, 1, 0, 2);
    ASSERT_EQ(0, result, "Should not throw if replacer on wrong base");

    result = should_ai_throw(&pii, 0, 1, 1, 2, 1, 2);
    ASSERT_EQ(0, result, "Should not throw if replacer is moving");

    return TEST_PASSED;
}

int test_should_ai_drop_ball_scenario(void)
{
    RefereeState ref = {0};
    BetweenPitchState bps;
    bps.catchHasBeenMade = 1;
    ref.woundingEvaluationActive = 1;

    // Scenario: Runners on 2nd and 3rd, catcher has ball at home base.
    // woundingEvaluationActive=1, r3=3, r3On=1, r2=2, r2On=1, home==hasBall
    int result = should_ai_drop_ball(&ref, &bps, BASE_THIRD, 1, BASE_SECOND, 1, 1, 1);
    ASSERT_EQ(1, result, "Should drop ball in ajolähtö tactical drop scenario");

    // Not wounding evaluation
    ref.woundingEvaluationActive = 0;
    result = should_ai_drop_ball(&ref, &bps, BASE_THIRD, 1, BASE_SECOND, 1, 1, 1);
    ASSERT_EQ(0, result, "Should not drop if not a fly ball (wounding evaluation)");

    // Catcher doesn't have the ball
    ref.woundingEvaluationActive = 1;
    result = should_ai_drop_ball(&ref, &bps, BASE_THIRD, 1, BASE_SECOND, 1, 1, 0);
    ASSERT_EQ(0, result, "Should not drop if catcher doesn't have the ball at home");

    return TEST_PASSED;
}

int test_determine_lead_base_simple(void)
{
    CatchingRunnerInfo runners[2];
    // Runner 1: Base 1, Not leading
    runners[0].base = BASE_FIRST;
    runners[0].isOnBase = 0;
    runners[0].takingFreeWalk = 0;
    runners[0].leading = 0;

    // Runner 2: Base 2, Not leading
    runners[1].base = BASE_SECOND;
    runners[1].isOnBase = 0;
    runners[1].takingFreeWalk = 0;
    runners[1].leading = 0;

    BaseID result = determine_lead_base(runners, 2, 123);
    ASSERT_EQ(BASE_SECOND, result, "Lead base should be BASE_SECOND");

    return TEST_PASSED;
}

int test_determine_lead_base_random(void)
{
    CatchingRunnerInfo runners[1];
    // Runner: Base 2, Leading
    runners[0].base = BASE_SECOND;
    runners[0].isOnBase = 0;
    runners[0].takingFreeWalk = 0;
    runners[0].leading = 1;

    // Random = 0 -> base - 1 (BASE_FIRST)
    BaseID result = determine_lead_base(runners, 1, 0);
    ASSERT_EQ(BASE_FIRST, result, "Should be base - 1 when random is 0");

    // Random != 0 -> base (actually it stays leadBase, which init to BASE_NONE)
    result = determine_lead_base(runners, 1, 1);
    ASSERT_EQ(BASE_NONE, result, "Should not update leadBase if leading and random != 0");

    return TEST_PASSED;
}