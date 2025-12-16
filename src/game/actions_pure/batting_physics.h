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
 * Calculates the vertical angle of the batted ball.
 *
 * @param power_count Selected power value from the meter
 * @param angle_count Selected angle value from the meter
 * @param ball_vy Vertical velocity of the incoming ball
 * @param swing_max The maximum value of the swing meter (BAT_SWING_MAX)
 * @param load_max The maximum value of the load meter (BAT_LOAD_MAX)
 * @return The vertical angle in radians (or internal angle units)
 */
float calculate_batting_vertical_angle(int power_count, int angle_count, float ball_vy, int swing_max, int load_max);

/**
 * Calculates the visual meter value [0.0, 1.0] for power selection.
 *
 * @param counter Current meter counter value
 * @param max Maximum meter counter value (meterCounterMax)
 * @return Normalized meter value
 */
float calculate_power_meter_value(int counter, int max);

/**
 * Calculates the visual meter value [0.0, 1.0] for angle selection.
 * The meter behaves differently here: it moves down from the power selection point.
 *
 * @param counter Current meter counter value
 * @param max Maximum meter counter value
 * @param power_count The power value that was selected
 * @param swing_max The maximum value of the swing meter
 * @param load_max The maximum value of the load meter
 * @return Normalized meter value
 */
float calculate_angle_meter_value(int counter, int max, int power_count, int swing_max, int load_max);

/**
 * Calculates the 3D velocity vector of the batted ball.
 *
 * @param vertical_angle Calculated vertical angle
 * @param horizontal_angle Calculated horizontal angle (-batterAngle * 2)
 * @param power Selected power value
 * @param power_factor Player's power attribute
 * @param ball_offset_x The horizontal offset of the ball when hit (affects direction)
 * @return Velocity vector
 */
Vector3D calculate_batted_ball_velocity(float vertical_angle, float horizontal_angle, float power, int power_factor, float ball_offset_x);

#endif /* BATTING_PHYSICS_H */
