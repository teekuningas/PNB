#include "test_helpers.h"
#include "foundation.h"
#include "field.h"
#include "pitching_physics.h"
#include "actions_pure/batting_physics.h"
#include "actions_pure/swing_geometry.h"
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

// §23 substrate floor: even the minimum-power pitch (power = 0.0) must rise at least 1 m above the
// pitcher's head (≈ 3.304 world units), making sub-legal matala pitches physically unrepresentable,
// and must stay airborne long enough for the 60-tick swing preparation animation to complete.
int test_pitch_minimum_height_meets_rule_23_floor(void)
{
    Vector3D v = pitch_velocity_from_aim(0.0f, 0.0f);
    float y = BALL_HEIGHT_WITH_PLAYER;
    float vy = v.y;
    float peak_y = y;

    // Exact discrete integration matching ball_flight.c (apply velocity, then gravity).
    for (int f = 0; f < 300; f++) {
        y += vy;
        if (y > peak_y) peak_y = y;
        vy -= GRAVITY;
        if (y <= BALL_SIZE / 2.0f) break;
    }

    // Player mesh head sits at 1.45 + 2.7984 * 0.24 = 2.1216; height = 2.1276 (= 1.80 m -> 1.1820 u/m);
    // head + 1 m = 3.3036 units.
    ASSERT(peak_y >= 3.304f, "minimum-power pitch must reach at least 1 m above the pitcher's head (§23)");

    int contact_frames = calculate_pitch_frame_time(v.y, GRAVITY, 0.0f, SWING_CONTACT_TWEAK_FRAMES);
    ASSERT(contact_frames > 60, "minimum-power pitch must stay airborne longer than SWING_PREPARATION_TICKS (60)");

    return TEST_PASSED;
}
