#ifndef LOADOBJ_H
#define LOADOBJ_H

#define OBJ_NO_ERROR              0 /**< No error occurred. The data pointers have been placed in the input
	* structure. */
#define OBJ_FILE_OPEN_FAILED      1 /**< The function failed to open the file for one reason or another.
	* Handle this by telling the player to check their storage media. */
#define OBJ_FILE_READ_FAILED      2 /**< An error occurred while trying to read the file before the data
	* could be completely read. As for OBJ_FILE_OPEN_FAILED, the player should be told to check their media. */
#define OBJ_NAME_NOT_FOUND        3 /**< The object name given in \a objectname was not found in the
	* given \a filename. */
#define OBJ_FACE_TYPE_UNSUPPORTED 4 /**< The face type is unsupported. Currently only three-point
	* (triangle) and four-point (quad) primitives are supported. */
#define OBJ_TOO_MANY_ATTRIBUTES   5 /**< The object file being read has too many vertex positions, normal
	* positions or something else. The maximum is 65,535. */
#define OBJ_TOO_FEW_VERTICES      6 /**< The object file being read has too few vertices defined. */
#define OBJ_TOO_MANY_FACES        7 /**< The object file being read has too many faces. The maximum number
	* of faces for triangles is 21,845; the maximum for quads is 16,383. */
#define OBJ_TOO_FEW_FACES         8 /**< The object file being read has too few faces. This usually occurs
	* if there's no faces specified in the file. */
#define OBJ_ALLOCATE_FAILED       9/**< The function failed to allocate memory for all lists. This usually
	* happens because either the system is out of memory (most likely) or there are too many allocations. */

typedef struct _MeshObject
{
	unsigned int uFaceCount; /**< The number of faces this object has. */
	int *uFaceList; /**< Pointer to face array. Format should be:
							* \a position \a normal \a color \a texcoord
							* Each should be the array subscript to access each element. You need three of these for each three-sided face, etc. */
	unsigned int uPositionCount;
	unsigned int uNormalCount;
	unsigned int uColorCount;
	unsigned int uTexCoordCount;

	float *fPositionIndex; /**< Pointer to the position data array. */
	float *fNormalIndex; /**< Pointer to the normal data array. */
	float *fColorIndex; /**< Pointer to the color data array. */
	float *fTexCoordIndex; /**< Pointer to the texture coordinate data array. */

} MeshObject;

int LoadObj (const char* filename, const char *objectname, MeshObject *meshObj ) ;

#endif /* LOADOBJ_H */
