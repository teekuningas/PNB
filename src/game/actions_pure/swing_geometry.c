#include "swing_geometry.h"

float swing_vertical_angle(float vertical, float ball_vy)
{
    return SWING_ELEVATION_GAIN * ball_vy * (SWING_VERTICAL_FOCAL - vertical);
}

float swing_marker_top(float power)
{
    if (power < 0.0f) power = 0.0f;
    if (power > 1.0f) power = 1.0f;
    return SWING_VERTICAL_FOCAL + power * (1.0f - SWING_VERTICAL_FOCAL);
}

int swing_vertical_sweep_frames(int frames_to_contact)
{
    int length = frames_to_contact - SWING_LEAD_FRAMES;
    if (length > SWING_VERTICAL_SWEEP_FRAMES) length = SWING_VERTICAL_SWEEP_FRAMES;
    if (length < 1) length = 1;
    return length;
}
