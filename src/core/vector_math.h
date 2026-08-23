#ifndef VECTOR_MATH_H
#define VECTOR_MATH_H

#include "globals.h"

// Pure vector math utility functions
// These have no side effects and don't depend on game state

// Distance/length functions
float vec3_distance_xz(const Vector3D* a, const Vector3D* b);
int vec3_is_small_enough_sphere(const Vector3D* vector, float limit);
int vec3_is_small_enough_circle_xz_v(const Vector3D* vector, float limit);
int vec3_is_small_enough_circle_xz(float dx, float dz, float limit);

// Vector setting functions
void vec3_set_xyz(Vector3D* vector, float x, float y, float z);
void vec3_set_from_vector(Vector3D* dest, const Vector3D* src);
void vec3_set_xz(Vector3D* vector, float x, float z);

// Vector addition functions
void vec3_add_xz(Vector3D* vector, float x, float z);

#endif /* VECTOR_MATH_H */
