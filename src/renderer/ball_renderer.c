#include "ball_renderer.h"
#include "../core/render.h" // For GL-related functions and MeshObject
#include <GL/glew.h>
#include <GL/gl.h>
#include <stdlib.h> // For malloc

#define BALL_SCALE BALL_SIZE
#define SHADOW_CONSTANT 0.2f

static GLuint ballTexture;

static MeshObject* ballMesh;
static GLuint ballDisplayList;

static MeshObject* shadowMesh;
static GLuint shadowDisplayList;

int initBallRenderer(void)
{
	if(tryLoadingTextureGL(&ballTexture, "data/textures/pallo.tga", "ball") != 0) return -1;
	ballMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/pallo.obj", "Icosphere", ballMesh, &ballDisplayList) != 0) return -1;
	shadowMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/shadow.obj", "Circle", shadowMesh, &shadowDisplayList) != 0) return -1;

	return 0;
}

// Function to draw the ball
void drawBallRenderer(const BallInfo* ballInfo, double alpha)
{
	if(ballInfo->visible == 1) {
		// we draw ball and its shadow. shadow's x offset is just proportional to ball's height.
		glBindTexture(GL_TEXTURE_2D, ballTexture);
		glPushMatrix();
		glTranslatef((float)(alpha*ballInfo->location.x + (1-alpha)*ballInfo->lastLocation.x),
		             (float)(alpha*ballInfo->location.y + (1-alpha)*ballInfo->lastLocation.y),
		             (float)(alpha*ballInfo->location.z + (1-alpha)*ballInfo->lastLocation.z));
		glScalef(BALL_SCALE, BALL_SCALE, BALL_SCALE);
		glCallList(ballDisplayList);
		glPopMatrix();
		// and the shadow
		glEnable(GL_BLEND);
		glDisable(GL_LIGHTING);
		glPushMatrix();
		glTranslatef((float)(alpha*ballInfo->location.x + (1-alpha)*ballInfo->lastLocation.x +
		                     -SHADOW_CONSTANT*(alpha*ballInfo->location.y + (1-alpha)*ballInfo->lastLocation.y)),
		             SHADOW_HEIGHT,
		             (float)(alpha*ballInfo->location.z + (1-alpha)*ballInfo->lastLocation.z));
		glScalef(BALL_SCALE, BALL_SCALE, BALL_SCALE);
		glCallList(ballDisplayList);
		glPopMatrix();
		glDisable(GL_BLEND);
		glEnable(GL_LIGHTING);
	}
}

int cleanBallRenderer(void)
{
	cleanMesh(ballMesh);
	cleanMesh(shadowMesh);
	return 0;
}
