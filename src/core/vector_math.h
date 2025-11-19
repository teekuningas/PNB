#ifndef VECTOR_MATH_H
#define VECTOR_MATH_H

#include "globals.h"

// Pure vector math utility functions
// These have no side effects and don't depend on game state

// Distance/length checking functions
int vec3_is_small_enough_sphere(Vector3D *vector, float limit);
int vec3_is_small_enough_circle_xz_v(Vector3D *vector, float limit);
int vec3_is_small_enough_circle_xz(float dx, float dz, float limit);

// Vector setting functions
void vec3_set_xyz(Vector3D *vector, float x, float y, float z);
void vec3_set_from_vector(Vector3D *dest, Vector3D *src);
void vec3_set_xz(Vector3D *vector, float x, float z);

// Vector addition functions
void vec3_add_xz(Vector3D *vector, float x, float z);
void vec3_add_vector(Vector3D *dest, Vector3D *src);

#endif /* VECTOR_MATH_H */
