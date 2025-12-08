#include "ball_physics.h"
#include "globals.h"

void physics_apply_gravity(Vector3D* velocity, float dt) {
    if (velocity) {
        velocity->y -= GRAVITY * dt;
    }
}

void physics_apply_velocity(Vector3D* position, const Vector3D* velocity) {
    if (position && velocity) {
        position->x += velocity->x;
        position->y += velocity->y;
        position->z += velocity->z;
    }
}