#include "swing_geometry.h"

float swing_vertical_angle(float vertical, float ball_vy)
{
    return SWING_ELEVATION_GAIN * ball_vy * (SWING_VERTICAL_FOCAL - vertical);
}

int swing_vertical_sweep_frames(int frames_to_contact)
{
    int length = frames_to_contact - SWING_LEAD_FRAMES;
    if (length > SWING_VERTICAL_SWEEP_FRAMES) length = SWING_VERTICAL_SWEEP_FRAMES;
    if (length < 1) length = 1;
    return length;
}
