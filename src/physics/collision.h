#ifndef COLLISION_H
#define COLLISION_H

#include "../core/vector_math.h" // For Vector3D

int physics_check_ground_collision(float y_pos, float ground_level);
int physics_resolve_field_boundaries(Vector3D* position, Vector3D* velocity, float field_front, float field_back, float field_left, float field_right, float damping_factor);

#endif // COLLISION_H
