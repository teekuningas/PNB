#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

#include "loadobj_gl.h"

static int seekIndex = 0;
static int lastN = 0;

static __inline int SeekToObject ( char *objfile, const char *objectname );
static __inline int ReadLine ( char* objfile, char *line );

int LoadObj (const char* filename, const char *objectname, MeshObjectGL *meshObj )
{
	char line[255]  = { 0 };   // Buffer to hold each line.
	char param[2]   = { 0,0 }; // Returned characters from a line
	int  retval     = 0;       // Return value from ReadLine.
	int bNoNormals = 0;   // TRUE if this object has no normals.

	unsigned int uFaceCount     = 0;
	unsigned int uPositionCount = 0;
	unsigned int uNormalCount   = 0;
	unsigned int uTexCoordCount = 0;

	int *uFaceList      = NULL; // Various lists.
	float *fPositionIndex = NULL;
	float *fNormalIndex   = NULL;
	float *fColorIndex    = NULL;
	float *fTexCoordIndex = NULL;

	unsigned int vcnt  = 0; // Vertex count
	unsigned int vncnt = 0; // Vertex normal count
	unsigned int vtcnt = 0; // Tex coord count
	unsigned int fcnt  = 0; // Face count

	char* objfile;
	int size = 0;
	FILE *f = fopen(filename, "rb");
	if (f == NULL)
	{
		objfile = NULL;
		printf("opening model file failed; terminating\n");
		return -1; // -1 means file opening fail
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	objfile = (char *)malloc(size+1);
	if (size != fread(objfile, sizeof(char), size, f))
	{
		free(objfile);
		printf("opening model file failed; terminating\n");
		return -1;
	}
	fclose(f);
	seekIndex = 0;
	lastN = 0;

	if ( objfile == NULL ) // Exit if we can't open it.
		return OBJ_FILE_OPEN_FAILED;

	if ( objectname != NULL )
	{
		// First we need to find the object group.

		retval = SeekToObject ( objfile, objectname );
		if ( retval == 1 )
			return OBJ_NAME_NOT_FOUND;
	}

	// Start counting the values.
	while (1) {
		retval = ReadLine ( objfile, line );

		// Break on ending.
		if ( retval == 1 )
			break;

		// Otherwise read in the things.
		sscanf ( line, "%c%c", &param[0], &param[1] );
		switch ( param[0] ) {
			case 'v': // One of three other types.
				switch ( param[1] ) {
					case ' ': // Regular vertex.
						uPositionCount++;
						break;
					case 'n': // Vertex normal.
						uNormalCount++;
						break;
					case 't': // Texture coord.
						uTexCoordCount++;
						break;
				}
				break;
			case 'f':
				uFaceCount++;
				break;
		}


		if ( objectname != NULL ) {
			if ( param[0] == 'o' )
				// Done processing this object.
				break;
		}
	}

	// Do some basic checks.
	if ( uFaceCount == 0 )
		return OBJ_TOO_FEW_FACES;
	else if ( uPositionCount == 0 )
		return OBJ_TOO_FEW_VERTICES;

	// Now it's time to allocate the arrays.
	uFaceList = (int *)malloc ( ( sizeof(int) * 3 * 4 * uFaceCount ));

	fPositionIndex = (float *)malloc ( ( sizeof(float) * 3 ) * uPositionCount );

	if ( uNormalCount == 0 ) {
		// Objects without normals get default normal values.
		bNoNormals = 1;
		fNormalIndex = (float *)malloc ( ( sizeof(float) * 3 ) );
		fNormalIndex[0] = 0.f;
		fNormalIndex[1] = 0.f;
		fNormalIndex[2] = 1.f;
	} else
		fNormalIndex = (float *)malloc ( ( sizeof(float) * 3 ) * uNormalCount );

	fColorIndex    = (float *)malloc ( ( sizeof(float) * 3 ) );
	fColorIndex[0] = 1.0f;
	fColorIndex[1] = 1.0f;
	fColorIndex[2] = 1.0f;

	fTexCoordIndex = (float *)malloc ( ( sizeof(float) * 2 ) * uTexCoordCount );

	if ( uFaceList     == NULL ||
		fPositionIndex == NULL ||
		fNormalIndex   == NULL ||
		fTexCoordIndex == NULL ) {
		free ( uFaceList );
		free ( fPositionIndex );
		free ( fNormalIndex );
		free ( fTexCoordIndex );
		return OBJ_ALLOCATE_FAILED;
	}

	// Rewind and find that object block again.
	seekIndex = 0;
	lastN = 0;
	if ( objectname != NULL ) {
		retval = SeekToObject ( objfile, objectname );
		if ( retval == 1 )
			return OBJ_NAME_NOT_FOUND;
	}
	// Populate the arrays.

	while (1) {

		retval = ReadLine ( objfile, line );

		// Break on ending.
		if ( retval == 1 )
			break; // TODO: make sure loaded count matches earlier count.

		// Read in the line.
		sscanf ( line, "%c%c", &param[0], &param[1] );

		if ( objectname != NULL ) {
		if ( param[0] == 'o' )
			break; // Break out if it's a new object.
		}

		if( param[0] == 's')
		{
			continue;
		}

		switch ( param[0] ) {
			case 'v': // One of three other types.
				switch ( param[1] ) {
					case ' ': // Regular vertex.
						sscanf ( line, "v %f %f %f",
						&fPositionIndex[vcnt+0], &fPositionIndex[vcnt+1], &fPositionIndex[vcnt+2] );
						vcnt += 3;
						break;
					case 'n': // Vertex normal.
						sscanf ( line, "vn %f %f %f",
						&fNormalIndex[vncnt+0], &fNormalIndex[vncnt+1], &fNormalIndex[vncnt+2] );
						vncnt += 3;
						break;
					case 't': // Texture coord.
						sscanf ( line, "vt %f %f",
						&fTexCoordIndex[vtcnt+0], &fTexCoordIndex[vtcnt+1] );
						vtcnt += 2;
						break;
				}
				break;
			case 'f':
				if ( bNoNormals ) {
					sscanf ( line, "f %d//%d %d//%d %d//%d",
					&uFaceList[fcnt+0], &uFaceList[fcnt+3],
					&uFaceList[fcnt+4], &uFaceList[fcnt+7],
					&uFaceList[fcnt+8], &uFaceList[fcnt+11] );

					uFaceList[fcnt+1]  = 0; // Default normal and colors.
					uFaceList[fcnt+2]  = 0;
					uFaceList[fcnt+5]  = 0;
					uFaceList[fcnt+6]  = 0;
					uFaceList[fcnt+9]  = 0;
					uFaceList[fcnt+10] = 0;
				} else {
					sscanf ( line, "f %d/%d/%d %d/%d/%d %d/%d/%d",
					&uFaceList[fcnt+0], &uFaceList[fcnt+3], &uFaceList[fcnt+1],
					&uFaceList[fcnt+4], &uFaceList[fcnt+7], &uFaceList[fcnt+5],
					&uFaceList[fcnt+8], &uFaceList[fcnt+11], &uFaceList[fcnt+9] );

					uFaceList[fcnt+2]  = 0; // Default colors.
					uFaceList[fcnt+6]  = 0;
					uFaceList[fcnt+10] = 0;

					uFaceList[fcnt+1]--; // Decrement normals.
					uFaceList[fcnt+5]--;
					uFaceList[fcnt+9]--;
				}

				uFaceList[fcnt+0]--; // Decrement these.
				uFaceList[fcnt+3]--;
				uFaceList[fcnt+4]--;
				uFaceList[fcnt+7]--;
				uFaceList[fcnt+8]--;
				uFaceList[fcnt+11]--;

				fcnt += 12;
				break;
		}

		// Done.
	}

	// All went well, so load the things into the mesh object!
	meshObj->uFaceCount     = uFaceCount;
	meshObj->uFaceList      = uFaceList;
	meshObj->fPositionIndex = fPositionIndex;
	meshObj->fNormalIndex   = fNormalIndex;
	meshObj->fColorIndex    = fColorIndex;
	meshObj->fTexCoordIndex = fTexCoordIndex;
	meshObj->uTexCoordCount = uTexCoordCount;
	meshObj->uPositionCount = uPositionCount;
	if(bNoNormals)
	{
		meshObj->uNormalCount = 1;
	}
	else
	{
		meshObj->uNormalCount = uNormalCount;
	}
	meshObj->uColorCount = 1;

	free(objfile);

	return OBJ_NO_ERROR;
}

__inline int ReadLine ( char* objfile, char *line )
{
	while(1)
	{
		if(objfile[seekIndex] == '\n')
		{
			strncpy (line, &(objfile[lastN]), seekIndex - lastN);
			line[seekIndex - lastN] = '\0';
			lastN = seekIndex + 1;
			seekIndex++;
			return 0;
		}
		else if(objfile[seekIndex] == '\0')
		{
			 return 1;
		}

		seekIndex++;
	}

	return 0;
}

__inline int SeekToObject (char *objfile, const char *objectname )
{
	int retval = 0;
	char line[255] = { 0 };
	char name[255] = { 0 }; // Buffer to hold the object name (for comparison)
	char param     = 0;

	while (1)
	{
		retval = ReadLine ( objfile, line );

		if ( retval != 0 )
			return retval;
		// Skip this line if it isn't a name.
		sscanf ( line, "%c", &param );
		if ( param != 'o' )
			continue;

		// If this is an "o" line, read the name.
		sscanf ( line, "o %s", name );
		if ( strcmp ( objectname, name ) != 0 )
			// Wrong name; continue to the next line.
			continue;

		break;
	}

	return 0;
}
