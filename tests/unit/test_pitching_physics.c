#include "test_helpers.h"
#include "pitching_physics.h"
#include <stdio.h>
#include <math.h>

int test_pitch_velocity_from_aim(void)
{

    // direction 0 -> straight up over the plate (dx = 0); power sets the toss height (dy); dz always 0.
    Vector3D v = pitch_velocity_from_aim(0.0f, 0.0f);
    ASSERT_FLOAT_EQ(0.0f, v.x, 0.001f, "dir 0 -> dx 0 (on the plate)");
    ASSERT_FLOAT_EQ(PITCH_BASE_SPEED, v.y, 0.001f, "power 0 -> dy = base speed");
    ASSERT_FLOAT_EQ(0.0f, v.z, 0.001f, "dz is always 0");

    // full power + full positive direction
    v = pitch_velocity_from_aim(1.0f, 1.0f);
    ASSERT_FLOAT_EQ(PITCH_ANGLE_CONSTANT, v.x, 0.001f, "dir 1 -> dx = angle constant");
    ASSERT_FLOAT_EQ(PITCH_BASE_SPEED + PITCH_POWER_CONSTANT, v.y, 0.001f, "power 1 -> dy = base + power");

    // negative direction mirrors
    v = pitch_velocity_from_aim(0.5f, -1.0f);
    ASSERT_FLOAT_EQ(-PITCH_ANGLE_CONSTANT, v.x, 0.001f, "dir -1 -> dx = -angle constant");

    // out-of-range inputs clamp to [0,1] / [-1,1]
    v = pitch_velocity_from_aim(2.0f, 5.0f);
    ASSERT_FLOAT_EQ(PITCH_BASE_SPEED + PITCH_POWER_CONSTANT, v.y, 0.001f, "power clamps to 1");
    ASSERT_FLOAT_EQ(PITCH_ANGLE_CONSTANT, v.x, 0.001f, "direction clamps to 1");

    return TEST_PASSED;
}
