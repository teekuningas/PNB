#include "globals.h"

#include "render.h"
#include <time.h>

/*
	And here we have our wii or gc specific rendering code that is utilized by a ton of .c:s, 
	like ball.c, player.c, main_menu.c etc.
	Drawing is done in immediate mode but using indexing.
*/

#if defined(__wii__)
static __inline void DrawVertexV ( u16 p, u16 n, u16 c, u16 t ) {

	GX_Position1x16(p);
	GX_Normal1x16(n);
	GX_Color1x16(c);
	GX_TexCoord1x16(t);

	return;
}

void drawMesh(MeshObject* mesh)
{
	GX_SetArray(GX_VA_POS,  mesh->fPositionIndex, ( sizeof(f32) * 3 ) );
	GX_SetArray(GX_VA_NRM,  mesh->fNormalIndex,   ( sizeof(f32) * 3 ) );
	GX_SetArray(GX_VA_CLR0, mesh->uColorIndex,    ( sizeof(u8)  * 4 ) );
	GX_SetArray(GX_VA_TEX0, mesh->fTexCoordIndex, ( sizeof(f32) * 2 ) );
	
	GX_InvVtxCache();
	GX_Begin( GX_TRIANGLES, GX_VTXFMT0, (mesh->uFaceCount*3) );
		int i;
		for (i = 0; i < (mesh->uFaceCount*12); i += 4) {
			DrawVertexV(mesh->uFaceList[i+0],
						mesh->uFaceList[i+1],
						mesh->uFaceList[i+2],
						mesh->uFaceList[i+3]);
		}	
	GX_End();	
}

u32 prepareMesh(MeshObject* mesh, void** displayList)
{
	u32 blockSize;
	DCFlushRange(mesh->fPositionIndex, ( sizeof(f32) * 3 ) * mesh->uPositionCount);
	DCFlushRange(mesh->fNormalIndex, (sizeof(f32) * 3 ) * mesh->uNormalCount);
	DCFlushRange(mesh->uColorIndex, (sizeof(u8) * 4 ) * mesh->uColorCount);
	DCFlushRange(mesh->fTexCoordIndex, (sizeof(f32) * 2 ) * mesh->uTexCoordCount);	
	
	/*
		Just random approximations, not particularly accurate.
	*/
	if(mesh->uFaceCount < 100)
	{
		blockSize = 6400;
	}
	else
	{
		blockSize = 64000;
	}
	*displayList = memalign(32, blockSize);
	memset(*displayList,0,blockSize);
	DCInvalidateRange(*displayList,blockSize);
	GX_BeginDispList(*displayList,blockSize);
	drawMesh(mesh);
	u32 size = GX_EndDispList();
	return size;
}



void cleanMesh(MeshObject* mesh)
{
	if(mesh != NULL)
	{
		free(mesh->uFaceList);
		free(mesh->fPositionIndex);
		free(mesh->fNormalIndex);
		free(mesh->uColorIndex);
		free(mesh->fTexCoordIndex);
		free(mesh);
	}
		
}

int tryPreparingMeshGX(const char* path, const char* name, MeshObject* mesh, u32* listSize, void* displayList)
{
	
	int result;
	result = LoadObj(path, name, mesh);
	if(result != 0)
	{
		printf("\nError with LoadObj. Error code: %d\n", result);
		return -1;
	}	
	
	*listSize = prepareMesh(mesh, displayList);	
	
	return 0;
}


#else
void drawMesh(MeshObject* mesh)
{
	unsigned int i;
	
	glBegin(GL_TRIANGLES);	
	for (i = 0; i < (mesh->uFaceCount*12); i += 4)
	{
		glTexCoord2fv(&(mesh->fTexCoordIndex)[2*(mesh->uFaceList)[i+3]]);
		glNormal3fv(&(mesh->fNormalIndex)[3*(mesh->uFaceList)[i+1]]);
		glVertex3fv(&(mesh->fPositionIndex)[3*(mesh->uFaceList)[i]]);
	}	
	glEnd();
	
}

void cleanMesh(MeshObject* mesh)
{
	if(mesh != NULL)
	{
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
	if(result != 1)
	{
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
	if(result != 0)
	{
		printf("\nError with LoadObj. Error code: %d\n", result);
		return -1;
	}

	prepareMesh(mesh, displayList);
	return 0;
}
#endif
