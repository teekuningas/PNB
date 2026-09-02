#include "pitching_physics.h"

Vector3D pitch_velocity_from_aim(float power, float direction)
{
    Vector3D v;
    if (power < 0.0f) power = 0.0f;
    if (power > 1.0f) power = 1.0f;
    if (direction < -1.0f) direction = -1.0f;
    if (direction > 1.0f) direction = 1.0f;
    v.x = direction * PITCH_ANGLE_CONSTANT; // placement; 0 -> on the plate (oikea)
    v.y = PITCH_BASE_SPEED + power * PITCH_POWER_CONSTANT; // toss height
    v.z = 0.0f;
    return v;
}

int pitch_windup_total_frames(float power)
{
    if (power < 0.0f) power = 0.0f;
    if (power > 1.0f) power = 1.0f;
    return PITCH_WINDUP_DOWN_FRAMES + (int)(power * PITCH_WINDUP_HOLD_MAX) + PITCH_WINDUP_UP_FRAMES;
}
