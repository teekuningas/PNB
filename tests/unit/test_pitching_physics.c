#include "test_helpers.h"
#include "pitching_physics.h"
#include <stdio.h>
#include <math.h>

static int float_eq(float a, float b, float epsilon)
{
	return fabs(a - b) < epsilon;
}

int test_pitch_power_calc()
{
	printf("Running test: %s\n", __func__);
	ASSERT_TRUE(float_eq(0.0f, calculate_pitch_power(0, 100), 0.001f), "Power 0");
	ASSERT_TRUE(float_eq(0.5f, calculate_pitch_power(50, 100), 0.001f), "Power 0.5");
	ASSERT_TRUE(float_eq(1.0f, calculate_pitch_power(100, 100), 0.001f), "Power 1.0");
	return TEST_PASSED;
}

int test_pitch_angle_calc()
{
	printf("Running test: %s\n", __func__);
	// pitchAngle = meterCounter/max - 9/13
	// 9/13 ~= 0.6923

	// max = 1300 (for easy math)
	// counter = 900 -> 900/1300 = 9/13. Angle should be 0.
	ASSERT_TRUE(float_eq(0.0f, calculate_pitch_angle(900, 1300), 0.001f), "Angle 0");

	// counter = 1300 -> 1.0 - 9/13 = 4/13 ~= 0.3077
	ASSERT_TRUE(float_eq(4.0f/13.0f, calculate_pitch_angle(1300, 1300), 0.001f), "Angle max");

	return TEST_PASSED;
}

int test_pitch_velocity()
{
	printf("Running test: %s\n", __func__);

	// dx = angle * 0.15
	ASSERT_TRUE(float_eq(0.15f, calculate_pitch_dx(1.0f), 0.001f), "dx for angle 1");
	ASSERT_TRUE(float_eq(0.0f, calculate_pitch_dx(0.0f), 0.001f), "dx for angle 0");

	// dy = 0.065 + power * 0.12
	ASSERT_TRUE(float_eq(0.065f, calculate_pitch_dy(0.0f), 0.001f), "dy min");
	ASSERT_TRUE(float_eq(0.185f, calculate_pitch_dy(1.0f), 0.001f), "dy max");

	return TEST_PASSED;
}

int test_pitch_meter_ui()
{
	printf("Running test: %s\n", __func__);

	// Phase 2: Direct ratio
	ASSERT_TRUE(float_eq(0.2f, calculate_meter_value(2, 20, 100), 0.001f), "Meter phase 2");

	// Phase 4: Inverse ratio
	// 1.0 - 20/100 = 0.8
	ASSERT_TRUE(float_eq(0.8f, calculate_meter_value(4, 20, 100), 0.001f), "Meter phase 4");

	return TEST_PASSED;
}

