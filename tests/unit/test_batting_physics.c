#include "test_helpers.h"
#include "batting_physics.h"
#include <stdio.h>
#include <math.h>

// Helper for float comparison
static int float_eq(float a, float b, float epsilon)
{
	return fabs(a - b) < epsilon;
}

int test_pitch_frame_time()
{
	printf("Running test: %s\n", __func__);

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

int test_meter_values()
{
	printf("Running test: %s\n", __func__);

	// Power meter
	// Counter 0 -> 0.0
	ASSERT_TRUE(float_eq(0.0f, calculate_power_meter_value(0, 100), 0.001f), "Power meter 0");
	// Counter 100 -> 1.0
	ASSERT_TRUE(float_eq(1.0f, calculate_power_meter_value(100, 100), 0.001f), "Power meter max");
	// Counter 50 -> 0.5
	ASSERT_TRUE(float_eq(0.5f, calculate_power_meter_value(50, 100), 0.001f), "Power meter mid");

	// Angle meter
	// Angle meter moves down from power point

	// Counter = 0 (start of angle selection) => should start at power level (0.8)
	// upperLimit = (80 + 0)/100 = 0.8
	// Result = 0.8 - (0/100)*(...) = 0.8
	ASSERT_TRUE(float_eq(0.8f, calculate_angle_meter_value(0, 100, 80, 100, 100), 0.001f), "Angle meter at start (0)");

	// Counter = 100 (max) => should be at 0.0
	ASSERT_TRUE(float_eq(0.0f, calculate_angle_meter_value(100, 100, 80, 100, 100), 0.001f), "Angle meter at max (100)");

	// Counter = 50 (half) => should be halfway between 0.8 and 0.0 => 0.4
	ASSERT_TRUE(float_eq(0.4f, calculate_angle_meter_value(50, 100, 80, 100, 100), 0.001f), "Angle meter at half");

	return TEST_PASSED;
}

int test_batting_vertical_angle()
{
	printf("Running test: %s\n", __func__);

	int power_count = 50;
	int angle_count = 50;
	float ball_vy = -1.0f; // Ball falling down
	int swing_max = 100;
	int load_max = 100;

	// If power and angle counts are equal, it's a "level" swing relative to the power
	// The exact formula is implementation defined but we can check bounds or specific behavior
	float angle = calculate_batting_vertical_angle(power_count, angle_count, ball_vy, swing_max, load_max);

	// Just ensure it returns a valid number (not NaN)
	// The value can be large (e.g. 175.0) as it is scaled later
	ASSERT_TRUE(!isnan(angle), "Angle should not be NaN");
	ASSERT_TRUE(angle > -1000.0f &&angle < 1000.0f, "Angle should be within sanity bounds");

	return TEST_PASSED;
}

int test_batted_ball_velocity()
{
	printf("Running test: %s\n", __func__);

	float v_angle = 0.785f; // Used to calculate alpha
	float h_angle = 0.0f;   // Straight forward
	float power = 1.0f;     // Max power
	int power_factor = 10;  // Stats
	float offset_x = 0.0f;

	Vector3D vel = calculate_batted_ball_velocity(v_angle, h_angle, power, power_factor, offset_x);

	ASSERT_TRUE(vel.y > 0, "Ball should go up");
	// Forward in this engine is -Z (based on calculations: dz = -cos(alpha)*cos(theta))
	ASSERT_TRUE(vel.z < 0, "Ball should go forward (negative Z)");
	ASSERT_TRUE(float_eq(0.0f, vel.x, 0.001f), "Ball should go straight (x=0)");

	return TEST_PASSED;
}
