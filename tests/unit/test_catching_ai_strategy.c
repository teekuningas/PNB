#include "test_helpers.h"
#include "field.h"
#include "rules_state.h"
#include "catching_ai_strategy.h"
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
    // The holder (0) is not the baseman (1), and the baseman is near home.
    int result = should_ai_throw(0, 1, 1, 99, 0, 0, 0, 0);
    ASSERT_EQ(1, result, "Should throw if catcher is near home and doesn't have ball");

    // catcherNearHome = 0
    result = should_ai_throw(0, 1, 0, 99, 0, 0, 0, 0);
    ASSERT_EQ(0, result, "Should not throw if catcher not near home");

    // the holder is the baseman
    result = should_ai_throw(1, 1, 1, 99, 0, 0, 0, 0);
    ASSERT_EQ(0, result, "Should not throw if catcher has ball");

    return TEST_PASSED;
}

int test_should_ai_throw_replacer(void)
{
    // The holder (0) throws toward base 2, whose baseman (1) is away; the cover is player 1 too.
    int result = should_ai_throw(0, 1, 0, 1, 1, 2, 0, 2);
    ASSERT_EQ(1, result, "Should throw to replacer in position");

    result = should_ai_throw(0, 1, 0, 1, 1, 1, 0, 2);
    ASSERT_EQ(0, result, "Should not throw if replacer on wrong base");

    result = should_ai_throw(0, 1, 0, 1, 1, 2, 1, 2);
    ASSERT_EQ(0, result, "Should not throw if replacer is moving");

    return TEST_PASSED;
}

int test_should_ai_throw_baseman_to_own_cover(void)
{
    // Baseman (13) fielded the ball away from 1st base (catcherNearHome=0); cover (16) is idle on 1st.
    ASSERT_EQ(1, should_ai_throw(13, 13, 0, 16, 1, 1, 0, 1), "Off-post baseman should throw to his idle cover");

    // Baseman already on his own post must not throw to a cover.
    ASSERT_EQ(0, should_ai_throw(13, 13, 1, 16, 1, 1, 0, 1), "On-post baseman should not throw to cover");

    // Cover still moving to the base: wait instead of throwing.
    ASSERT_EQ(0, should_ai_throw(13, 13, 0, 16, 1, 1, 1, 1), "Off-post baseman should not throw while cover is moving");

    // Cover on a different base or inactive: do not throw.
    ASSERT_EQ(0, should_ai_throw(13, 13, 0, 16, 1, 0, 0, 1), "Cover on another base is not a receiver");
    ASSERT_EQ(0, should_ai_throw(13, 13, 0, 16, 0, 1, 0, 1), "Inactive cover is not a receiver");

    // Cover holding the ball never throws to himself.
    ASSERT_EQ(0, should_ai_throw(16, 13, 0, 16, 1, 1, 0, 1), "Cover holding the ball should not throw to himself");

    // Missing cover slot (-1) cannot receive a throw.
    ASSERT_EQ(0, should_ai_throw(13, 13, 0, -1, 1, 1, 0, 1), "Missing cover slot cannot receive a throw");

    return TEST_PASSED;
}

int test_should_ai_drop_ball_scenario(void)
{
    RefereeState ref = {0};
    ref.wounding_evaluation_active = 1;

    // Scenario: Runners on 2nd and 3rd, catcher has ball at home base.
    // wounding_evaluation_active=1, r3=3, r3On=1, r2=2, r2On=1, home==hasBall
    int result = should_ai_drop_ball(&ref, BASE_THIRD, 1, BASE_SECOND, 1, 1, 1);
    ASSERT_EQ(1, result, "Should drop ball in ajolähtö tactical drop scenario");

    // Not wounding evaluation
    ref.wounding_evaluation_active = 0;
    result = should_ai_drop_ball(&ref, BASE_THIRD, 1, BASE_SECOND, 1, 1, 1);
    ASSERT_EQ(0, result, "Should not drop if not a fly ball (wounding evaluation)");

    // Catcher doesn't have the ball
    ref.wounding_evaluation_active = 1;
    result = should_ai_drop_ball(&ref, BASE_THIRD, 1, BASE_SECOND, 1, 1, 0);
    ASSERT_EQ(0, result, "Should not drop if catcher doesn't have the ball at home");

    return TEST_PASSED;
}

int test_determine_throw_target_base_advancing(void)
{
    // Two advancing runners (1st->2nd and 2nd->3rd): throw to 3rd (lead runner's destination).
    CatchingRunnerInfo runners[2] = {
        {.is_on_base = 0, .taking_free_walk = 0, .base = BASE_FIRST, .leading = 0, .going_forward = 1},
        {.is_on_base = 0, .taking_free_walk = 0, .base = BASE_SECOND, .leading = 0, .going_forward = 1},
    };
    ASSERT_EQ(
        BASE_THIRD, determine_throw_target_base(runners, 2, 123, 0), "advancing lead runner from 2nd targets 3rd"
    );

    // Scoring runner heading home from 3rd outranks a runner heading to 3rd — even when pitch rights
    // still need a field-base visit.
    CatchingRunnerInfo scoring[2] = {
        {.is_on_base = 0, .taking_free_walk = 0, .base = BASE_SECOND, .leading = 0, .going_forward = 1},
        {.is_on_base = 0, .taking_free_walk = 0, .base = BASE_THIRD, .leading = 0, .going_forward = 1},
    };
    ASSERT_EQ(
        BASE_HOME, determine_throw_target_base(scoring, 2, 123, 1),
        "runner heading home from 3rd outranks runner heading to 3rd and pitch-rights toss"
    );

    // No active off-base runners -> return ball to home when pitch rights are granted, or toss to
    // 1st when §24 still needs a field-base visit.
    CatchingRunnerInfo settled[1] = {
        {.is_on_base = 1, .taking_free_walk = 0, .base = BASE_SECOND, .leading = 0, .going_forward = 0},
    };
    ASSERT_EQ(BASE_HOME, determine_throw_target_base(settled, 1, 123, 0), "no threatened runner returns home");
    ASSERT_EQ(
        BASE_FIRST, determine_throw_target_base(settled, 1, 123, 1), "idle field with §24 NEED_FIELD_BASE tosses to 1st"
    );

    return TEST_PASSED;
}

int test_determine_throw_target_base_retreating_and_pickoff(void)
{
    // Retreating runner (e.g. turning back to 1st on väärä/koppi): throw to 1st, NOT ahead to 2nd.
    CatchingRunnerInfo retreating[1] = {
        {.is_on_base = 0, .taking_free_walk = 0, .base = BASE_FIRST, .leading = 0, .going_forward = 0},
    };
    ASSERT_EQ(
        BASE_FIRST, determine_throw_target_base(retreating, 1, 123, 0),
        "retreating runner targets the base they are returning to"
    );

    // Leading runner on 2nd: pickoff only on randomValue == 0, else return home.
    CatchingRunnerInfo leading[1] = {
        {.is_on_base = 0, .taking_free_walk = 0, .base = BASE_SECOND, .leading = 1, .going_forward = 0},
    };
    ASSERT_EQ(
        BASE_SECOND, determine_throw_target_base(leading, 1, 0, 0), "pickoff roll targets the leading runner's base"
    );
    ASSERT_EQ(BASE_HOME, determine_throw_target_base(leading, 1, 1, 0), "non-pickoff roll returns ball home");

    // Free-walk runner is never targeted.
    CatchingRunnerInfo freeWalk[1] = {
        {.is_on_base = 0, .taking_free_walk = 1, .base = BASE_SECOND, .leading = 0, .going_forward = 1},
    };
    ASSERT_EQ(BASE_HOME, determine_throw_target_base(freeWalk, 1, 0, 0), "free-walk runner is ignored");
    ASSERT_EQ(
        BASE_FIRST, determine_throw_target_base(freeWalk, 1, 0, 1),
        "free-walk runner still lets idle defense toss to 1st for §24 pitch rights"
    );

    return TEST_PASSED;
}