#include "batting_physics.h"
#include <math.h>

int calculate_pitch_frame_time(float velocity_y, float gravity, float start_height, int tweak_frames)
{
	float v = velocity_y;
	float a = -gravity;
	float s = start_height;

	if (fabs(a) < 0.000001f) return 0; // Avoid division by zero

	// Solve 0 = s + vt + 0.5*a*t^2 for t
	float discriminant = v*v - 2*a*s;
	if (discriminant < 0) return 0; // Should not happen for normal parabolic motion starting above ground

	float t = (-v - sqrtf(discriminant)) / a;
	return (int)t + tweak_frames;
}

float calculate_batting_vertical_angle(int power_count, int angle_count, float ball_vy, int swing_max, int load_max)
{
	float scaleNumber = (float)(power_count + (swing_max - load_max));
	if (fabs(scaleNumber) < 0.00001f) return 0.0f;

	float zeroNumber = swing_max * (1.0f * power_count / scaleNumber);

	float verticalAngle = 7.0f * ball_vy * (angle_count - zeroNumber) * (scaleNumber / swing_max);

	return verticalAngle;
}

float calculate_power_meter_value(int counter, int max)
{
	if (max == 0) return 0.0f;
	return 1.0f * counter / max;
}

float calculate_angle_meter_value(int counter, int max, int power_count, int swing_max, int load_max)
{
	if (swing_max == 0 || max == 0) return 0.0f;

	float upperLimit = (float)(power_count + (swing_max - load_max)) / swing_max;
	float lowerLimit = 0.0f;

	return upperLimit - (1.0f * counter / max) * (upperLimit - lowerLimit);
}

Vector3D calculate_batted_ball_velocity(float vertical_angle, float horizontal_angle, float power, int power_factor, float ball_offset_x)
{
	Vector3D velocity = {0.0f, 0.0f, 0.0f};

	float magnitude = (0.0125f + power_factor * 0.0015f) * power;

	float alfa = (vertical_angle * 2.0f + 5.0f) * 0.05f;

	float theta = horizontal_angle + 0.05f * ball_offset_x;

	float dy = sinf(alfa) * cosf(theta);
	float dz = -cosf(alfa) * cosf(theta);
	float dx = sinf(theta);

	velocity.x = magnitude * dx;
	velocity.y = magnitude * dy;
	velocity.z = magnitude * dz;

	return velocity;
}
