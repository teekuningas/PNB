#include "globals.h"
#include "render.h"

#include "ball.h"
#include "ball_internal.h"


// initializes ball as an entity in the empty space. ball has to be located to the field in a different place
int initBall()
{
	// we define the texture and the mesh of the ball and its shadow.
	// shadow is basically made by just removing color intensity of background of the shadow mesh, shadow mesh
	// itself doesnt have any texture.
	if(tryLoadingTextureGL(&ballTexture, "data/textures/pallo.tga", "ball") != 0) return -1;
	ballMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/pallo.obj", "Icosphere", ballMesh, &ballDisplayList) != 0) return -1;
	shadowMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/shadow.obj", "Circle", shadowMesh, &shadowDisplayList) != 0) return -1;

	return 0;
}

void drawBall(double alpha, BallInfo* ballInfo)
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


// cleaning keeps the house tidy
int cleanBall()
{
	cleanMesh(ballMesh);
	cleanMesh(shadowMesh);
	return 0;
}

