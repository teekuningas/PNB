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
	#if defined(__wii__)
	int result;
	TPL_OpenTPLFromMemory(&ballTPL, (void *)pallo_tpl, pallo_tpl_size);
	TPL_GetTexture(&ballTPL, pallo, &ballTexture);

	ballMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGX("data/models/pallo.obj", "Icosphere",
		ballMesh, &ballListSize, &ballDisplayList) != 0) return -1;
	// shadow needs a bit special treatment
	shadowMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	result = LoadObj("data/models/shadow.obj", "Circle", shadowMesh);
	if(result != 0)
	{
		printf("\nError with LoadObj. Error code: %d\n", result);
		return -1;
	}
	free(shadowMesh->uColorIndex);
	// this will be free'd with the mesh struct
	shadowColors = memalign( 32, ( sizeof(u8) * 4 ) );
	shadowColors[0] = 255; shadowColors[1] = 255;
	shadowColors[2] = 255; shadowColors[3] = 127;
	shadowMesh->uColorIndex = shadowColors;

	shadowListSize = prepareMesh(shadowMesh, &shadowDisplayList);

	#else
	if(tryLoadingTextureGL(&ballTexture, "data/textures/pallo.tga", "ball") != 0) return -1;
	ballMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/pallo.obj", "Icosphere", ballMesh, &ballDisplayList) != 0) return -1;
	shadowMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/shadow.obj", "Circle", shadowMesh, &shadowDisplayList) != 0) return -1;
	#endif

	return 0;
}

void drawBall(double alpha, BallInfo* ballInfo)
{
	if(ballInfo->visible == 1)
	{
		// we draw ball and its shadow. shadow's x offset is just proportional to ball's height.
		#if defined(__wii__)
		guMtxIdentity(model);
		guMtxScaleApply(model, model, BALL_SCALE, BALL_SCALE, BALL_SCALE);
		guMtxTransApply(model, model, alpha*ballInfo->location.x + (1-alpha)*ballInfo->lastLocation.x,
			alpha*ballInfo->location.y + (1-alpha)*ballInfo->lastLocation.y,
			alpha*ballInfo->location.z + (1-alpha)*ballInfo->lastLocation.z);
		guMtxConcat(view,model,modelview);
		GX_LoadPosMtxImm(modelview, GX_PNMTX0);
		guMtxInverse(modelview,mvi);
		guMtxTranspose(mvi,modelview);
		GX_LoadNrmMtxImm(modelview, GX_PNMTX0);
		GX_LoadTexObj(&(ballTexture), GX_TEXMAP0);
		GX_CallDispList(ballDisplayList, ballListSize);

		// and the shadow
		GX_SetBlendMode(GX_BM_BLEND, GX_BL_ZERO, GX_BL_SRCALPHA, GX_LO_SET);
		guMtxIdentity(model);
		guMtxScaleApply(model, model, BALL_SCALE*3, BALL_SCALE*3, BALL_SCALE*3);
		guMtxTransApply(model, model, alpha*ballInfo->location.x + (1-alpha)*ballInfo->lastLocation.x +
			-SHADOW_CONSTANT*(alpha*ballInfo->location.y + (1-alpha)*ballInfo->lastLocation.y),
			SHADOW_HEIGHT,
			alpha*ballInfo->location.z + (1-alpha)*ballInfo->lastLocation.z);
		guMtxConcat(view,model,modelview);
		GX_LoadPosMtxImm(modelview, GX_PNMTX0);
		guMtxInverse(modelview,mvi);
		guMtxTranspose(mvi,modelview);
		GX_LoadNrmMtxImm(modelview, GX_PNMTX0);
		GX_CallDispList(shadowDisplayList, shadowListSize);
		GX_SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_SET);

		#else

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
		#endif
	}
}


// cleaning keeps the house tidy
int cleanBall()
{
	cleanMesh(ballMesh);
	cleanMesh(shadowMesh);
	#if defined(__wii__)
	free(ballDisplayList);
	#endif
	return 0;
}

