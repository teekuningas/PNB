#ifndef PITCHING_PHYSICS_H
#define PITCHING_PHYSICS_H

#define PITCH_BASE_SPEED 0.065f
#define PITCH_POWER_CONSTANT 0.12f
#define PITCH_ANGLE_CONSTANT 0.15f
#define PITCH_DOWN_MAX 9
#define PITCH_UP_MAX 13
#define VERTICAL_ANGLE_LIMIT 5

float calculate_pitch_power(int meter_counter, int meter_counter_max);
float calculate_pitch_angle(int meter_counter, int meter_counter_max);
float calculate_pitch_dx(float pitch_angle);
float calculate_pitch_dy(float pitch_power);
float calculate_meter_value(int pitch_phase, int meter_counter, int meter_counter_max);

#endif
