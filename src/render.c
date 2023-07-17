#include "globals.h"

#include "render.h"
#include <time.h>

void drawMesh(MeshObject* mesh)
{
	unsigned int i;

	glBegin(GL_TRIANGLES);
	for (i = 0; i < (mesh->uFaceCount*12); i += 4) {
		glTexCoord2fv(&(mesh->fTexCoordIndex)[2*(mesh->uFaceList)[i+3]]);
		glNormal3fv(&(mesh->fNormalIndex)[3*(mesh->uFaceList)[i+1]]);
		glVertex3fv(&(mesh->fPositionIndex)[3*(mesh->uFaceList)[i]]);
	}
	glEnd();

}

void cleanMesh(MeshObject* mesh)
{
	if(mesh != NULL) {
		free(mesh->uFaceList);
		free(mesh->fPositionIndex);
		free(mesh->fNormalIndex);
		free(mesh->fColorIndex);
		free(mesh->fTexCoordIndex);
		free(mesh);
	}

}

void prepareMesh(MeshObject* mesh, GLuint* displayList)
{
	*displayList=glGenLists(1);
	glNewList(*displayList,GL_COMPILE);
	drawMesh(mesh);
	glEndList();
}

int tryLoadingTextureGL(GLuint* texture, const char* filename, const char* name)
{
	int result;
	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);
	result = glfwLoadTexture2D(filename, GLFW_BUILD_MIPMAPS_BIT);
	if(result != 1) {
		printf("\n Couldn't load %s texture.", name);
		return -1;
	}
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glBindTexture(GL_TEXTURE_2D, 0);
	return 0;
}

int tryPreparingMeshGL(char* filename, char* objectname, MeshObject* mesh, GLuint* displayList)
{
	int result;
	// load from obj file to mesh-struct
	result = LoadObj (filename, objectname, mesh) ;
	if(result != 0) {
		printf("\nError with LoadObj. Error code: %d\n", result);
		return -1;
	}

	prepareMesh(mesh, displayList);
	return 0;
}
