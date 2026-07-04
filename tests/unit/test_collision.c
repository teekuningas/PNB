#include "test_helpers.h"
#include "collision.h"
#include "vector_math.h"
#include <stdio.h>
#include <math.h>

int test_collision_resolve_boundaries_no_collision(void)
{

    Vector3D pos = {0.0f, 0.0f, 0.0f};
    Vector3D vel = {1.0f, 0.0f, 1.0f};

    // Define boundaries that contain the point
    float front = 10.0f;
    float back = -10.0f;
    float left = -10.0f;
    float right = 10.0f;
    float damping = 0.5f;

    int result = physics_resolve_field_boundaries(&pos, &vel, front, back, left, right, damping);

    ASSERT_EQ(0, result, "Should return 0 for no collision");
    ASSERT_FLOAT_EQ(0.0f, pos.x, 0.001f, "Position X unchanged");
    ASSERT_FLOAT_EQ(0.0f, pos.z, 0.001f, "Position Z unchanged");
    ASSERT_FLOAT_EQ(1.0f, vel.x, 0.001f, "Velocity X unchanged");
    ASSERT_FLOAT_EQ(1.0f, vel.z, 0.001f, "Velocity Z unchanged");

    return TEST_PASSED;
}

int test_collision_resolve_boundaries_z_front(void)
{

    Vector3D pos = {0.0f, 0.0f, 15.0f}; // Outside front boundary (10.0f)
    Vector3D vel = {1.0f, 0.0f, 5.0f}; // Moving towards boundary

    float front = 10.0f;
    float back = -10.0f;
    float left = -10.0f;
    float right = 10.0f;
    float damping = 0.5f;

    int result = physics_resolve_field_boundaries(&pos, &vel, front, back, left, right, damping);

    ASSERT_EQ(1, result, "Should return 1 for collision");
    ASSERT_FLOAT_EQ(10.0f, pos.z, 0.001f, "Position Z clamped to front boundary");
    ASSERT_FLOAT_EQ(-2.5f, vel.z, 0.001f, "Velocity Z reflected and damped (5.0 * -0.5 = -2.5)");

    return TEST_PASSED;
}

int test_collision_resolve_boundaries_z_back(void)
{

    Vector3D pos = {0.0f, 0.0f, -15.0f}; // Outside back boundary (-10.0f)
    Vector3D vel = {1.0f, 0.0f, -5.0f}; // Moving backwards

    float front = 10.0f;
    float back = -10.0f;
    float left = -10.0f;
    float right = 10.0f;
    float damping = 0.5f;

    int result = physics_resolve_field_boundaries(&pos, &vel, front, back, left, right, damping);

    ASSERT_EQ(1, result, "Should return 1 for collision");
    ASSERT_FLOAT_EQ(-10.0f, pos.z, 0.001f, "Position Z clamped to back boundary");
    ASSERT_FLOAT_EQ(2.5f, vel.z, 0.001f, "Velocity Z reflected and damped (-5.0 * -0.5 = 2.5)");

    return TEST_PASSED;
}

int test_collision_resolve_boundaries_x_right(void)
{

    Vector3D pos = {15.0f, 0.0f, 0.0f}; // Outside right boundary (10.0f)
    Vector3D vel = {5.0f, 0.0f, 1.0f};

    float front = 10.0f;
    float back = -10.0f;
    float left = -10.0f;
    float right = 10.0f;
    float damping = 0.5f;

    int result = physics_resolve_field_boundaries(&pos, &vel, front, back, left, right, damping);

    ASSERT_EQ(1, result, "Should return 1 for collision");
    ASSERT_FLOAT_EQ(10.0f, pos.x, 0.001f, "Position X clamped to right boundary");
    ASSERT_FLOAT_EQ(-2.5f, vel.x, 0.001f, "Velocity X reflected and damped");

    return TEST_PASSED;
}

int test_collision_resolve_boundaries_x_left(void)
{

    Vector3D pos = {-15.0f, 0.0f, 0.0f}; // Outside left boundary (-10.0f)
    Vector3D vel = {-5.0f, 0.0f, 1.0f};

    float front = 10.0f;
    float back = -10.0f;
    float left = -10.0f;
    float right = 10.0f;
    float damping = 0.5f;

    int result = physics_resolve_field_boundaries(&pos, &vel, front, back, left, right, damping);

    ASSERT_EQ(1, result, "Should return 1 for collision");
    ASSERT_FLOAT_EQ(-10.0f, pos.x, 0.001f, "Position X clamped to left boundary");
    ASSERT_FLOAT_EQ(2.5f, vel.x, 0.001f, "Velocity X reflected and damped");

    return TEST_PASSED;
}
