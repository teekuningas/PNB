#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "globals.h"

// Pure geometry utility functions
// These functions perform calculations based on Vector3D, without modifying global state.

/**
 * @brief Calculates the Euclidean distance between two 3D points.
 * @param a Pointer to the first Vector3D.
 * @param b Pointer to the second Vector3D.
 * @return The 3D distance between points a and b.
 */
float geometry_distance_3d(const Vector3D* a, const Vector3D* b);

/**
 * @brief Calculates the 2D distance between two points in the XZ plane.
 * @param a Pointer to the first Vector3D.
 * @param b Pointer to the second Vector3D.
 * @return The 2D distance (XZ plane) between points a and b.
 */
float geometry_distance_2d_xz(const Vector3D* a, const Vector3D* b);

/**
 * @brief Checks if a position is within a specified spherical radius from a center point.
 * @param pos Pointer to the position Vector3D.
 * @param center Pointer to the center Vector3D.
 * @param radius The radius to check against.
 * @return 1 if 'pos' is within 'radius' of 'center', 0 otherwise.
 */
int geometry_is_within_radius(const Vector3D* pos, const Vector3D* center, float radius);

/**
 * @brief Checks if a position is within a specified circular radius (XZ plane) from a center point.
 * @param pos Pointer to the position Vector3D.
 * @param center Pointer to the center Vector3D.
 * @param radius The radius to check against.
 * @return 1 if 'pos' is within 'radius' of 'center' (in XZ plane), 0 otherwise.
 */
int geometry_is_within_circle_xz_radius(const Vector3D* pos, const Vector3D* center, float radius);

/**
 * @brief Calculates the angle (in radians) between two vectors (from-to direction).
 * @param from Pointer to the origin Vector3D.
 * @param to Pointer to the target Vector3D.
 * @return The angle in radians.
 */
float geometry_angle_between(const Vector3D* from, const Vector3D* to);

#endif // GEOMETRY_H