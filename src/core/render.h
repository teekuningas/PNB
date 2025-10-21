#ifndef RENDER_H
#define RENDER_H

#include "loadobj.h"

void cleanMesh(MeshObject* mesh);
void prepareMesh(MeshObject* mesh, GLuint* displayList);
int tryPreparingMeshGL(char* filename, char* objectname, MeshObject* mesh, GLuint* displayList);
int tryLoadingTextureGL(GLuint* texture, const char* filename, const char* name);

#endif /* RENDER_H */
