#ifndef RENDER_H
#define RENDER_H

#if defined(__wii__)
#include "loadobj.h"
void cleanMesh(MeshObject* mesh);
u32 prepareMesh(MeshObject* mesh, void** displayList);
int tryPreparingMeshGX(const char* path, const char* name, MeshObject* mesh, u32* listSize, void* displayList);
#else
#include "loadobj.h"
void cleanMesh(MeshObject* mesh);
void prepareMesh(MeshObject* mesh, GLuint* displayList);
int tryPreparingMeshGL(char* filename, char* objectname, MeshObject* mesh, GLuint* displayList);
int tryLoadingTextureGL(GLuint* texture, const char* filename, const char* name);
#endif

#endif /* RENDER_H */
