#ifndef RENDER_H
#define RENDER_H

#include "globals.h"
#include "loadobj.h"

// A struct to hold global rendering state, like window dimensions.
typedef struct {
	int window_width;
	int window_height;
} RenderState;

void cleanMesh(MeshObject* mesh);
void prepareMesh(MeshObject* mesh, GLuint* displayList);
int tryPreparingMeshGL(char* filename, char* objectname, MeshObject* mesh, GLuint* displayList);
int tryLoadingTextureGL(GLuint* texture, const char* filename, const char* name);

// Sets up the 3D perspective projection.
void begin_3d_render(const RenderState* rs);

// Sets up the 2D orthographic projection.
void begin_2d_render(const RenderState* rs);

#endif /* RENDER_H */
