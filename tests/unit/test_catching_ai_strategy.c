#include "test_helpers.h"
#include "catching_ai_strategy.h"
#include "base_logic.h"
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

int test_calculate_movement_keys_up_left()
{
    // Angle > 5pi/8 and <= 7pi/8
    // 6pi/8 = 3pi/4 = 135 degrees (up-left)
    // dx = -1, dz = -1 (down-left? No wait)
    // atan2(-dz, dx) -> atan2(1, -1) -> 135 deg = 3pi/4

    // dx = -1, dz = -1
    // angle = atan2(1, -1) = 3pi/4 = 2.356
    // 5pi/8 = 1.96, 7pi/8 = 2.74
    // Should be UP + LEFT

    MovementKeys keys = calculate_movement_keys(-1.0f, -1.0f);
    ASSERT_EQ(1, keys.up, "Should be UP");
    ASSERT_EQ(1, keys.left, "Should be LEFT");
    ASSERT_EQ(0, keys.down, "Should not be DOWN");
    ASSERT_EQ(0, keys.right, "Should not be RIGHT");

    return TEST_PASSED;
}

int test_calculate_movement_keys_right()
{
    // Angle between -pi/8 and pi/8
    // dx = 1, dz = 0
    // angle = atan2(0, 1) = 0

    MovementKeys keys = calculate_movement_keys(1.0f, 0.0f);
    ASSERT_EQ(0, keys.up, "Should not be UP");
    ASSERT_EQ(0, keys.left, "Should not be LEFT");
    ASSERT_EQ(0, keys.down, "Should not be DOWN");
    ASSERT_EQ(1, keys.right, "Should be RIGHT");

    return TEST_PASSED;
}

int test_should_ai_throw_normal()
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

int test_should_ai_throw_replacer()
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

int test_should_ai_drop_ball_scenario()
{
    RefereeState ref = {0};
    BetweenPitchState bps;
    bps.catchHasBeenMade = 1;
    ref.woundingEvaluationActive = 1;
    // checkForRun removed - no longer used

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

int test_determine_lead_base_simple()
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

int test_determine_lead_base_random()
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