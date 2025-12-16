#include "pitching_physics.h"

float calculate_pitch_power(int meter_counter, int meter_counter_max) {
    if (meter_counter_max == 0) return 0.0f;
    return 1.0f * meter_counter / meter_counter_max;
}

float calculate_pitch_angle(int meter_counter, int meter_counter_max) {
    if (meter_counter_max == 0) return 0.0f;
    return 1.0f * meter_counter / meter_counter_max - 1.0f * PITCH_DOWN_MAX / PITCH_UP_MAX;
}

float calculate_pitch_dx(float pitch_angle) {
    return pitch_angle * PITCH_ANGLE_CONSTANT;
}

float calculate_pitch_dy(float pitch_power) {
    return PITCH_BASE_SPEED + pitch_power * PITCH_POWER_CONSTANT;
}

float calculate_meter_value(int pitch_phase, int meter_counter, int meter_counter_max) {
    if (meter_counter_max == 0) return 0.0f;
    
    if (pitch_phase == 2) {
        return 1.0f * meter_counter / meter_counter_max;
    } else if (pitch_phase == 4) {
        return 1.0f - 1.0f * meter_counter / meter_counter_max;
    }
    return 0.0f;
}
