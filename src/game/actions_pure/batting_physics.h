#ifndef BATTING_PHYSICS_H
#define BATTING_PHYSICS_H

#include "vector_math.h"

/**
 * Calculates the time (in frames) for a pitch to reach the target height (usually 0).
 * Solves 0 = s + vt + 0.5*a*t^2 for t.
 *
 * @param velocity_y Initial vertical velocity of the ball
 * @param gravity Gravity constant (positive value, effectively acts downwards)
 * @param start_height Initial height of the ball relative to target (s)
 * @param tweak_frames Additional frames to add for gameplay tweaking
 * @return Number of frames until impact/target height
 */
int calculate_pitch_frame_time(float velocity_y, float gravity, float start_height, int tweak_frames);

/**
 * Calculates the 3D velocity vector of the batted ball.
 *
 * @param vertical_angle Calculated vertical angle
 * @param horizontal_angle Calculated horizontal angle (-batter_angle * 2)
 * @param power Selected power value
 * @param power_factor Player's power attribute
 * @param ball_offset_x The horizontal offset of the ball when hit (affects direction)
 * @return Velocity vector
 */
Vector3D calculate_batted_ball_velocity(
    float vertical_angle, float horizontal_angle, float power, int power_factor, float ball_offset_x
);

#endif /* BATTING_PHYSICS_H */
