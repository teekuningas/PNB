#include "collision.h"
#include "../include/globals.h" // For Vector3D (already included via collision.h -> vector_math.h, but good to be explicit for constants)

int physics_check_ground_collision(float y_pos, float ground_level) {
    return y_pos < ground_level;
}

void physics_resolve_field_boundaries(Vector3D* position, Vector3D* velocity, float field_front, float field_back, float field_left, float field_right, float damping_factor) {
    // Check field boundaries (Z-axis)
    if (position->z > field_front) {
        position->z = field_front;
        velocity->z *= -damping_factor;
    } else if (position->z < field_back) {
        position->z = field_back;
        velocity->z *= -damping_factor;
    }

    // Check field boundaries (X-axis)
    if (position->x > field_right) {
        position->x = field_right;
        velocity->x *= -damping_factor;
    } else if (position->x < field_left) {
        position->x = field_left;
        velocity->x *= -damping_factor;
    }
}