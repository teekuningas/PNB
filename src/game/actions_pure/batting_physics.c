#include "batting_physics.h"
#include <math.h>

int calculate_pitch_frame_time(float velocity_y, float gravity, float start_height, int tweak_frames)
{
	// solve 0 = s + vt + (1/2)at^2 => 0.5*a*t^2 + v*t + s = 0
	// a is -gravity
	float v = velocity_y;
	float a = -gravity;
	float s = start_height;

	if (fabs(a) < 0.000001f) return 0; // Avoid division by zero

	// t = (-v - sqrt(v*v - 4*(0.5*a)*s)) / (2 * 0.5 * a)
	// t = (-v - sqrt(v*v - 2*a*s)) / a
	// We choose the negative root because a is negative, making the denominator negative.
	// (-v - sqrt(...)) is likely negative (since sqrt(...) > -v if s=0, and definitely if s>0 and v>0).
	// So negative/negative = positive time.
	
	// Original code: (int)((-v - sqrt(v*v - 2*a*s))/a) + PITCH_FRAME_TIME_TWEAK;
	float discriminant = v*v - 2*a*s;
	if (discriminant < 0) return 0; // Should not happen for normal parabolic motion starting above ground

	float t = (-v - sqrtf(discriminant)) / a;
	return (int)t + tweak_frames;
}

float calculate_batting_vertical_angle(int power_count, int angle_count, float ball_vy, int swing_max, int load_max)
{
	// Original logic:
	// scaleNumber = (float)(selectedBattingPowerCount + (BAT_SWING_MAX - BAT_LOAD_MAX));
	// zeroNumber = BAT_SWING_MAX*(1.0f*selectedBattingPowerCount / scaleNumber);
	// verticalAngle = 7 * stateInfo->localGameInfo->ballInfo.velocity.y * (selectedBattingAngleCount - zeroNumber) * (scaleNumber / BAT_SWING_MAX);
	
	float scaleNumber = (float)(power_count + (swing_max - load_max));
	if (fabs(scaleNumber) < 0.00001f) return 0.0f;

	float zeroNumber = swing_max * (1.0f * power_count / scaleNumber);
	
	// 7 is a constant factor mentioned in original comments
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
	// upperLimit = (float)(selectedBattingPowerCount + (BAT_SWING_MAX - BAT_LOAD_MAX)) / BAT_SWING_MAX;
	// lowerLimit = 0.0f;
	// stateInfo->localGameInfo->pRAI.swingMeterValue = upperLimit - (1.0f*meterCounter / meterCounterMax) * (upperLimit - lowerLimit);
	
	if (swing_max == 0 || max == 0) return 0.0f;

	float upperLimit = (float)(power_count + (swing_max - load_max)) / swing_max;
	float lowerLimit = 0.0f;
	
	return upperLimit - (1.0f * counter / max) * (upperLimit - lowerLimit);
}

Vector3D calculate_batted_ball_velocity(float vertical_angle, float horizontal_angle, float power, int power_factor, float ball_offset_x)
{
	Vector3D velocity = {0.0f, 0.0f, 0.0f};
	
	// magnitude = (0.0125f + powerFactor*0.0015f)*power;
	float magnitude = (0.0125f + power_factor * 0.0015f) * power;
	
	// alfa = (verticalAngle * 2 + 5) * 0.05f;
	float alfa = (vertical_angle * 2.0f + 5.0f) * 0.05f;
	
	// theta = horizontalAngle + 0.05f * stateInfo->localGameInfo->ballInfo.location.x;
	float theta = horizontal_angle + 0.05f * ball_offset_x;
	
	// dy = (float)(sin(alfa) * cos(theta));
	// dz = - (float)(cos(alfa) * cos(theta));
	// dx = (float)sin(theta);
	
	float dy = sinf(alfa) * cosf(theta);
	float dz = -cosf(alfa) * cosf(theta);
	float dx = sinf(theta);
	
	velocity.x = magnitude * dx;
	velocity.y = magnitude * dy;
	velocity.z = magnitude * dz;
	
	return velocity;
}
