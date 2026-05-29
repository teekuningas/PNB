#ifndef BALL_PHYSICS_H
#define BALL_PHYSICS_H

#include "globals.h"

void physics_apply_gravity(Vector3D* velocity, float dt);
void physics_apply_velocity(Vector3D* position, const Vector3D* velocity);

#endif // BALL_PHYSICS_H