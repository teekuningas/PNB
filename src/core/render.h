#ifndef RENDER_H
#define RENDER_H

#include <GL/glew.h>
#include "loadobj.h"

// A struct to hold global rendering state, like window dimensions.
typedef struct {
    int window_width;
    int window_height;
} RenderState;

void clean_mesh(MeshObject* mesh);
void prepare_mesh(MeshObject* mesh, GLuint* displayList);
int try_preparing_mesh_gl(char* filename, char* objectname, MeshObject* mesh, GLuint* displayList);
int try_loading_texture_gl(GLuint* texture, const char* filename, const char* name);

// Sets up the 3D perspective projection.
void begin_3d_render(const RenderState* rs);

// Sets up the 2D orthographic projection.
void begin_2d_render(const RenderState* rs);

// Draws a textured 2D quad.
void draw_texture_2d(GLuint texture, float x, float y, float width, float height);

#endif /* RENDER_H */
