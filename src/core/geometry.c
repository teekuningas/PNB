#include "geometry.h"
#include <math.h>

float geometry_distance_3d(const Vector3D* a, const Vector3D* b)
{
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float dz = a->z - b->z;
    return (float)sqrt(dx*dx + dy*dy + dz*dz);
}

float geometry_distance_2d_xz(const Vector3D* a, const Vector3D* b)
{
    float dx = a->x - b->x;
    float dz = a->z - b->z;
    return (float)sqrt(dx*dx + dz*dz);
}

int geometry_is_within_radius(const Vector3D* pos, const Vector3D* center, float radius)
{
    return geometry_distance_3d(pos, center) < radius ? 1 : 0;
}

int geometry_is_within_circle_xz_radius(const Vector3D* pos, const Vector3D* center, float radius)
{
    return geometry_distance_2d_xz(pos, center) < radius ? 1 : 0;
}

float geometry_angle_between(const Vector3D* from, const Vector3D* to)
{
    // This is a 2D (XZ plane) angle calculation for now, as is common in many game contexts.
    // If a 3D angle is needed, the dot product formula would be different.
    float dx = to->x - from->x;
    float dz = to->z - from->z;
    return (float)atan2(dx, dz); // atan2(y, x) -> atan2(dx, dz) for angle relative to Z-axis
}
