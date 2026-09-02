// The two meter functions this file used to test are gone: the swing declares its power and its
// elevation as values, so there is no meter left to read a level off. What they were really pinning
// — that the elevation law behaves — is pinned harder in test_swing_geometry.c, which proves the law
// that replaced them reproduces them exactly over the whole reachable grid.
#include "test_helpers.h"
#include "batting_physics.h"
#include <stdio.h>
#include <math.h>

int test_pitch_frame_time(void)
{

    float vy = 0.0f; // Dropped from a height
    float gravity = 0.5f;
    float start_height = 100.0f;
    int tweak = 0;

    // 0 = 100 + 0*t - 0.5*0.5*t^2 => 0.25*t^2 = 100 => t^2 = 400 => t = 20
    int frames = calculate_pitch_frame_time(vy, gravity, start_height, tweak);
    ASSERT_EQ(20, frames, "Frames should be 20 for simple drop");

    // With tweak
    tweak = 5;
    frames = calculate_pitch_frame_time(vy, gravity, start_height, tweak);
    ASSERT_EQ(25, frames, "Frames should include tweak");

    return TEST_PASSED;
}

int test_batted_ball_velocity(void)
{

    float v_angle = 0.785f; // Used to calculate alpha
    float h_angle = 0.0f; // Straight forward
    float power = 1.0f; // Max power
    int power_factor = 10; // Stats
    float offset_x = 0.0f;

    Vector3D vel = calculate_batted_ball_velocity(v_angle, h_angle, power, power_factor, offset_x);

    ASSERT_TRUE(vel.y > 0, "Ball should go up");
    // Forward in this engine is -Z (based on calculations: dz = -cos(alpha)*cos(theta))
    ASSERT_TRUE(vel.z < 0, "Ball should go forward (negative Z)");
    ASSERT_FLOAT_EQ(0.0f, vel.x, 0.001f, "Ball should go straight (x=0)");

    return TEST_PASSED;
}
