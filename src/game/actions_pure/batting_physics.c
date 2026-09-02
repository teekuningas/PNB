#include "batting_physics.h"
#include <math.h>

int calculate_pitch_frame_time(float velocity_y, float gravity, float start_height, int tweak_frames)
{
    float v = velocity_y;
    float a = -gravity;
    float s = start_height;

    if (fabs(a) < 0.000001f) return 0; // Avoid division by zero

    // Solve 0 = s + vt + 0.5*a*t^2 for t
    float discriminant = v * v - 2 * a * s;
    if (discriminant < 0) return 0; // Should not happen for normal parabolic motion starting above ground

    float t = (-v - sqrtf(discriminant)) / a;
    return (int)t + tweak_frames;
}

Vector3D calculate_batted_ball_velocity(
    float vertical_angle, float horizontal_angle, float power, int power_factor, float ball_offset_x
)
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
