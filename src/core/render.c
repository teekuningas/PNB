#include <time.h>

#include "globals.h"
#include "render.h"

#include "stb_image.h"

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
	int width;
	int height;
	int nrChannels;
	stbi_set_flip_vertically_on_load(1);
	unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
	if (data == NULL) {
		printf("Couldn't load texture: %s\n", name);
		return -1;
	}
	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);
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

void begin_3d_render(const RenderState* rs)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, PERSPECTIVE_ASPECT_RATIO, 0.1f, 250.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
}

void begin_2d_render(const RenderState* rs)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, rs->window_width, rs->window_height, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
}
