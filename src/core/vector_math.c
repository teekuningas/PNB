#include "vector_math.h"
#include <math.h>

int vec3_is_small_enough_sphere(Vector3D *vector, float limit)
{
	if(sqrt((vector->x)*(vector->x) + (vector->y)*(vector->y) + (vector->z)*(vector->z)) < limit) return 1;
	else return 0;
}

int vec3_is_small_enough_circle_xz_v(Vector3D *vector, float limit)
{
	if(sqrt((vector->x)*(vector->x) + (vector->z)*(vector->z)) < limit) return 1;
	else return 0;
}

int vec3_is_small_enough_circle_xz(float dx, float dz, float limit)
{
	if(sqrt(dx*dx + dz*dz) < limit) return 1;
	else return 0;
}

void vec3_set_xyz(Vector3D *vector, float x, float y, float z)
{
	vector->x = x;
	vector->y = y;
	vector->z = z;
}

void vec3_set_from_vector(Vector3D *dest, Vector3D *src)
{
	dest->x = src->x;
	dest->y = src->y;
	dest->z = src->z;
}

void vec3_set_xz(Vector3D *vector, float x, float z)
{
	vector->x = x;
	vector->z = z;
}

void vec3_add_xz(Vector3D *vector, float x, float z)
{
	vector->x += x;
	vector->z += z;
}

void vec3_add_vector(Vector3D *dest, Vector3D *src)
{
	dest->x += src->x;
	dest->y += src->y;
	dest->z += src->z;
}
