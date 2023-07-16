#include "globals.h"
#include "render.h"

#include "immutable_world.h"
#include "immutable_world_internal.h"

/*
	here we have everything rendered everything or at least most of that stays still in the game, like fences and ground. also this is the natural place
	to fill our FieldPositions structure.
*/

int initImmutableWorld()
{
	int result;
	stateInfo.fieldPositions = &fieldPositions;

	result = initGround();
	if(result != 0)
	{
		printf("Initialization of ground failed.");
		return result;
	}
	result = initFence();
	if(result != 0)
	{
		printf("Initialization of fence failed.");
		return result;
	}
	result = initPlate();
	if(result != 0)
	{
		printf("Initialization of plate failed.");
		return result;
	}
	initFieldPositions();

	return 0;
}

void initFieldPositions()
{
	// y-coordinate here is adjusted to be height of a ball when player has it, just because thats basically only way it is used.
	// some of the values are hard coded, which is fine. cannot expect to calculate all players' positions relative to something.
	fieldPositions.pitchPlate.x = 0.0f;
	fieldPositions.pitchPlate.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.pitchPlate.z = 0.0f;
	fieldPositions.pitcher.x = 1.5f;
	fieldPositions.pitcher.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.pitcher.z = 0.0f;
	fieldPositions.firstBaseRun.x = -20.0f;
	fieldPositions.firstBaseRun.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.firstBaseRun.z = -23.5f;
	fieldPositions.firstBase.x = fieldPositions.firstBaseRun.x;
	fieldPositions.firstBase.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.firstBase.z = fieldPositions.firstBaseRun.z + 0.5f;
	fieldPositions.secondBaseRun.x = +30.0f;
	fieldPositions.secondBaseRun.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.secondBaseRun.z = -43.8f;
	fieldPositions.secondBase.x = fieldPositions.secondBaseRun.x;
	fieldPositions.secondBase.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.secondBase.z = fieldPositions.secondBaseRun.z + 1.0f;
	fieldPositions.thirdBaseRun.x = -30.0f;
	fieldPositions.thirdBaseRun.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.thirdBaseRun.z = -43.8f;
	fieldPositions.thirdBase.x = fieldPositions.thirdBaseRun.x;
	fieldPositions.thirdBase.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.thirdBase.z = fieldPositions.thirdBaseRun.z + 1.0f;
	fieldPositions.leftPoint.x = -31.2f;
	fieldPositions.leftPoint.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.leftPoint.z = -33.0f;
	fieldPositions.runLeftPoint.x = fieldPositions.leftPoint.x;
	fieldPositions.runLeftPoint.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.runLeftPoint.z = -25.0f;
	fieldPositions.backLeftPoint.x = fieldPositions.leftPoint.x;
	fieldPositions.backLeftPoint.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.backLeftPoint.z = -83.8f;
	fieldPositions.rightPoint.x = 31.5f;
	fieldPositions.rightPoint.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.rightPoint.z = -32.0f;
	fieldPositions.backRightPoint.x = fieldPositions.rightPoint.x;
	fieldPositions.backRightPoint.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.backRightPoint.z = fieldPositions.backLeftPoint.z;
	fieldPositions.bottomRightCatcher.x = fieldPositions.secondBase.x - 21.0f;
	fieldPositions.bottomRightCatcher.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.bottomRightCatcher.z = fieldPositions.secondBase.z + 22.0f;
	fieldPositions.middleLeftCatcher.x = fieldPositions.thirdBase.x + 18.0f;
	fieldPositions.middleLeftCatcher.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.middleLeftCatcher.z = fieldPositions.thirdBase.z + 6.0f;
	fieldPositions.middleRightCatcher.x = -fieldPositions.middleLeftCatcher.x - 9.0f;
	fieldPositions.middleRightCatcher.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.middleRightCatcher.z = fieldPositions.secondBase.z - 6.0f;
	fieldPositions.backLeftCatcher.x = fieldPositions.backLeftPoint.x + 20.0f;
	fieldPositions.backLeftCatcher.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.backLeftCatcher.z = fieldPositions.backLeftPoint.z + 10.0f;
	fieldPositions.backRightCatcher.x = -fieldPositions.backLeftCatcher.x;
	fieldPositions.backRightCatcher.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.backRightCatcher.z = fieldPositions.backLeftCatcher.z;
	fieldPositions.homeRunPoint.x = fieldPositions.pitchPlate.x - HOME_RADIUS;
	fieldPositions.homeRunPoint.y = BALL_HEIGHT_WITH_PLAYER;
	fieldPositions.homeRunPoint.z = HOME_LINE_Z;
}

void drawImmutableWorld(double alpha)
{
	drawGround();
	drawFence();
	drawPlate();

}

static void drawPlate()
{
	// models' width and length are 2, 2, so thats why we divide by 2.
	// 0.5f is just so that it wouldnt be so high that shoes and ball will disappear in it.
	#if defined(__wii__)
	guMtxIdentity(model);
	guMtxScaleApply(model, model, PLATE_WIDTH/2, 0.5f, PLATE_WIDTH/2);
	guMtxConcat(view,model,modelview);
	GX_LoadPosMtxImm(modelview, GX_PNMTX0);
	guMtxInverse(modelview,mvi);
	guMtxTranspose(mvi,modelview);
	GX_LoadNrmMtxImm(modelview, GX_PNMTX0);
	GX_LoadTexObj(&plateTexture, GX_TEXMAP0);
	GX_CallDispList(plateDisplayList, plateListSize);
	#else
	glBindTexture(GL_TEXTURE_2D, plateTexture);
	glPushMatrix();
	glScalef(PLATE_WIDTH/2, 0.5f, PLATE_WIDTH/2);
	glCallList(plateDisplayList);
	glPopMatrix();
	#endif
}

// Draw fence and ground

// optimized the inner loops a bit, took out everything i could from loops.

static void drawFence()
{
	int i;
	// BACK FENCE
	#if defined(__wii__)
	GX_LoadTexObj(&fenceTexture, GX_TEXMAP0);
	guMtxIdentity(model);
	guMtxRotAxisDeg(rot, &rotXAxis, 90.0f);
	guMtxConcat(rot, model, model);
	guMtxTransApply(model, model, 0.0f, 1.0f, 0.0f);
	guMtxScaleApply(model, model, FENCE_PIECE_WIDTH/2, FENCE_HEIGHT/2, 1.0f);

	for(i = 0; i < (int)(5*GROUND_WIDTH/FENCE_PIECE_WIDTH); i++)
	{
		memcpy(fenceModel, model, sizeof (Mtx));
		guMtxTransApply(fenceModel, fenceModel, FENCE_PIECE_WIDTH/2 + FIELD_LEFT + i*FENCE_PIECE_WIDTH, 0.0f, FIELD_BACK);
		guMtxConcat(view,fenceModel,modelview);
		GX_LoadPosMtxImm(modelview, GX_PNMTX0);
		guMtxInverse(modelview,mvi);
		guMtxTranspose(mvi,modelview);
		GX_LoadNrmMtxImm(modelview, GX_PNMTX0);
		GX_CallDispList(planeDisplayList, planeListSize);
	}
	#else
	glBindTexture(GL_TEXTURE_2D, fenceTexture);
	for(i = 0; i < (int)(5*GROUND_WIDTH/FENCE_PIECE_WIDTH); i++)
	{
		glPushMatrix();
		glTranslatef(FENCE_PIECE_WIDTH/2 + FIELD_LEFT + i*FENCE_PIECE_WIDTH, 0.0f, FIELD_BACK);
		glScalef(FENCE_PIECE_WIDTH/2, FENCE_HEIGHT/2, 1.0f); // again, width and height of the model is 2
		glTranslatef(0.0f, 1.0f, 0.0f); // moves fence up so that its bottom is at the level of origin ( preparing for scale )
		glRotatef(90.0f,1.0f, 0.0f, 0.0f);
		glCallList(planeDisplayList);
		glPopMatrix();
	}
	#endif
	// FRONT FENCE
	#if defined(__wii__)
	GX_LoadTexObj(&fenceTexture, GX_TEXMAP0);
	// draw two times, first normally then rotated.
	guMtxIdentity(model);
	guMtxRotAxisDeg(rot, &rotXAxis, 90.0f);
	guMtxConcat(rot, model, model);
	guMtxTransApply(model, model, 0.0f, 1.0f, 0.0f);
	guMtxScaleApply(model, model, FENCE_PIECE_WIDTH/2, FENCE_HEIGHT/2, 1.0f);

	for(i = 0; i < (int)(5*GROUND_WIDTH/FENCE_PIECE_WIDTH); i++)
	{
		memcpy(fenceModel, model, sizeof (Mtx));
		guMtxTransApply(fenceModel, fenceModel, FENCE_PIECE_WIDTH/2 + FIELD_LEFT + i*FENCE_PIECE_WIDTH, 0.0f, FIELD_FRONT);
		guMtxConcat(view,fenceModel,modelview);
		GX_LoadPosMtxImm(modelview, GX_PNMTX0);
		guMtxInverse(modelview,mvi);
		guMtxTranspose(mvi,modelview);
		GX_LoadNrmMtxImm(modelview, GX_PNMTX0);
		GX_CallDispList(planeDisplayList, planeListSize);
	}
	guMtxIdentity(model);
	guMtxRotAxisDeg(rot, &rotXAxis, -90.0f);
	guMtxConcat(rot, model, model);
	guMtxTransApply(model, model, 0.0f, 1.0f, 0.0f);
	guMtxScaleApply(model, model, FENCE_PIECE_WIDTH/2, FENCE_HEIGHT/2, 1.0f);

	for(i = 0; i < (int)(5*GROUND_WIDTH/FENCE_PIECE_WIDTH); i++)
	{
		memcpy(fenceModel, model, sizeof (Mtx));
		guMtxTransApply(fenceModel, fenceModel, FENCE_PIECE_WIDTH/2 + FIELD_LEFT + i*FENCE_PIECE_WIDTH, 0.0f, FIELD_FRONT);
		guMtxConcat(view,fenceModel,modelview);
		GX_LoadPosMtxImm(modelview, GX_PNMTX0);
		guMtxInverse(modelview,mvi);
		guMtxTranspose(mvi,modelview);
		GX_LoadNrmMtxImm(modelview, GX_PNMTX0);
		GX_CallDispList(planeDisplayList, planeListSize);
	}
	#else
	glBindTexture(GL_TEXTURE_2D, fenceTexture);
	for(i = 0; i < (int)(5*GROUND_WIDTH/FENCE_PIECE_WIDTH); i++)
	{
		glPushMatrix();
		glTranslatef(FENCE_PIECE_WIDTH/2 + FIELD_LEFT + i*FENCE_PIECE_WIDTH, 0.0f, FIELD_FRONT);
		glScalef(FENCE_PIECE_WIDTH/2, FENCE_HEIGHT/2, 1.0f); // again, width and height of the model is 2
		glTranslatef(0.0f, 1.0f, 0.0f); // moves fence up so that its bottom is at the level of origin ( preparing for scale )
		glPushMatrix();
		// as the plane is visible only from the other side, we draw it two times, both rotated differently
		// so that it can be seen from both sides.
		glRotatef(90.0f,1.0f, 0.0f, 0.0f);
		glCallList(planeDisplayList);
		glPopMatrix();
		glRotatef(-90.0f,1.0f, 0.0f, 0.0f);
		glCallList(planeDisplayList);
		glPopMatrix();
	}
	#endif
	// LEFT FENCE
	#if defined(__wii__)
	guMtxIdentity(model);
	guMtxRotAxisDeg(rot, &rotXAxis, 90.0f);
	guMtxConcat(rot, model, model);
	guMtxRotAxisDeg(rot, &rotYAxis, 90.0f);
	guMtxConcat(rot, model, model);
	guMtxTransApply(model, model, 0.0f, 1.0f, 0.0f);
	guMtxScaleApply(model, model, 1.0f, FENCE_HEIGHT/2, FENCE_PIECE_WIDTH/2);
	for(i = 0; i < (int)(6*GROUND_LENGTH/FENCE_PIECE_WIDTH); i++)
	{
		memcpy(fenceModel, model, sizeof (Mtx));
		guMtxTransApply(fenceModel, fenceModel, FIELD_LEFT,0.0f, FIELD_BACK + FENCE_PIECE_WIDTH/2 + i*FENCE_PIECE_WIDTH);
		guMtxConcat(view,fenceModel,modelview);
		GX_LoadPosMtxImm(modelview, GX_PNMTX0);
		guMtxInverse(modelview,mvi);
		guMtxTranspose(mvi,modelview);
		GX_LoadNrmMtxImm(modelview, GX_PNMTX0);
		GX_CallDispList(planeDisplayList, planeListSize);
	}
	#else
	glBindTexture(GL_TEXTURE_2D, fenceTexture);
	for(i = 0; i < (int)(6*GROUND_LENGTH/FENCE_PIECE_WIDTH); i++)
	{
		glPushMatrix();
		glTranslatef(FIELD_LEFT,0.0f, FIELD_BACK + FENCE_PIECE_WIDTH/2 + i*FENCE_PIECE_WIDTH);
		glScalef(1.0f, FENCE_HEIGHT/2, FENCE_PIECE_WIDTH/2);
		glTranslatef(0.0f, 1.0f, 0.0f);
		glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
		glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
		glCallList(planeDisplayList);
		glPopMatrix();
	}
	#endif
	// RIGHT FENCE
	#if defined(__wii__)
	guMtxIdentity(model);
	guMtxRotAxisDeg(rot, &rotXAxis, 90.0f);
	guMtxConcat(rot, model, model);
	guMtxRotAxisDeg(rot, &rotYAxis, -90.0f);
	guMtxConcat(rot, model, model);
	guMtxTransApply(model, model, 0.0f, 1.0f, 0.0f);
	guMtxScaleApply(model, model, 1.0f, FENCE_HEIGHT/2, FENCE_PIECE_WIDTH/2);
	for(i = 0; i < (int)(6*GROUND_LENGTH/FENCE_PIECE_WIDTH); i++)
	{
		memcpy(fenceModel, model, sizeof (Mtx));
		guMtxTransApply(fenceModel, fenceModel, FIELD_RIGHT, 0.0f, FIELD_BACK + FENCE_PIECE_WIDTH/2 + i*FENCE_PIECE_WIDTH);
		guMtxConcat(view,fenceModel,modelview);
		GX_LoadPosMtxImm(modelview, GX_PNMTX0);
		guMtxInverse(modelview,mvi);
		guMtxTranspose(mvi,modelview);
		GX_LoadNrmMtxImm(modelview, GX_PNMTX0);
		GX_CallDispList(planeDisplayList, planeListSize);
	}
	#else
	glBindTexture(GL_TEXTURE_2D, fenceTexture);
	for(i = 0; i < (int)(6*GROUND_LENGTH/FENCE_PIECE_WIDTH) ; i++)
	{
		glPushMatrix();
		glTranslatef(FIELD_RIGHT, 0.0f, FIELD_BACK + FENCE_PIECE_WIDTH/2 + i*FENCE_PIECE_WIDTH);
		glScalef(1.0f, FENCE_HEIGHT/2, FENCE_PIECE_WIDTH/2);
		glTranslatef(0.0f, 1.0f, 0.0f);
		glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
		glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
		glCallList(planeDisplayList);
		glPopMatrix();
	}
	#endif
}

static void drawGround()
{
	int i;
	// here we use groundUnit[12].texture for all grass ground pieces.
	#if defined(__wii__)
	// models' width and length is 2
	guMtxIdentity(model);
	guMtxScaleApply(model, model, GROUND_LENGTH/2, 1.0f, GROUND_WIDTH/2);
	guMtxRotAxisDeg(rot, &rotYAxis, 90.0f);
	guMtxConcat(rot, model, model);
	// move to left side AND small fix to make pitch plate the origin.
	guMtxTransApply(model, model, -GROUND_WIDTH + GROUND_OFFSET_X, 0.0f, GROUND_OFFSET_Z);

	for(i = 0; i < GROUND_UNIT_COUNT; i++)
	{
		memcpy(groundModel, model, sizeof (Mtx));
		guMtxTransApply(groundModel, groundModel,GROUND_WIDTH*groundUnit[i].y ,0.0f, -GROUND_LENGTH*groundUnit[i].x);
		guMtxConcat(view,groundModel,modelview);
		GX_LoadPosMtxImm(modelview, GX_PNMTX0);
		guMtxInverse(modelview,mvi);
		guMtxTranspose(mvi,modelview);
		GX_LoadNrmMtxImm(modelview, GX_PNMTX0);
		if(i < 12)
		{
			GX_LoadTexObj(&(groundUnit[i].texture), GX_TEXMAP0);
		}
		else
		{
			GX_LoadTexObj(&(groundUnit[12].texture), GX_TEXMAP0);
		}
		GX_CallDispList(planeDisplayList, planeListSize);

	}
	#else
	for(i = 0; i < GROUND_UNIT_COUNT; i++)
	{
		if(i < 12)
		{
			glBindTexture(GL_TEXTURE_2D, groundUnit[i].texture);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, groundUnit[12].texture);
		}
		glPushMatrix();
		glTranslatef(GROUND_WIDTH*groundUnit[i].y,0.0f, -GROUND_LENGTH*groundUnit[i].x);
		glTranslatef(-GROUND_WIDTH + GROUND_OFFSET_X, 0.0f, GROUND_OFFSET_Z);
		glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
		glScalef(GROUND_LENGTH/2, 1.0f, GROUND_WIDTH/2);
		glCallList(planeDisplayList);
		glPopMatrix();
	}
	#endif
}
// cleaning is good for people.
int cleanImmutableWorld()
{
	cleanMesh(planeMesh);
	cleanMesh(plateMesh);
	#if defined(__wii__)
	free(planeDisplayList);
	free(plateDisplayList);
	#endif

	return 0;
}

static int initFence()
{
	#if defined(__wii__)
	TPL_OpenTPLFromMemory(&fenceTPL, (void *)fence_tpl, fence_tpl_size);
	TPL_GetTexture(&fenceTPL, fence, &fenceTexture);
	#else
	if(tryLoadingTextureGL(&fenceTexture, "data/textures/fence.tga", "fence") != 0) return -1;
	#endif

	return 0;
}

static int initPlate()
{
	#if defined(__wii__)
	TPL_OpenTPLFromMemory(&plateTPL, (void *)plate_tpl, plate_tpl_size);
	TPL_GetTexture(&plateTPL, plate, &plateTexture);
	plateMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGX("data/models/plate.obj", "Cylinder",
		plateMesh, &plateListSize, &plateDisplayList) != 0) return -1;
	#else

	if(tryLoadingTextureGL(&plateTexture, "data/textures/plate.tga", "plate") != 0) return -1;
	plateMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/plate.obj", "Cylinder", plateMesh, &plateDisplayList) != 0) return -1;

	#endif
	return 0;
}

static int initGround()
{
	int i, j, counter;
	// first we create the play area, in order of groundUnit[0] being lowerleft, groundUnit[1] being second in left etc.
	for(i = 0; i < 3; i++)
	{
		for(j = 0; j < 4; j++)
		{
			groundUnit[i*4 + j].x = j;
			groundUnit[i*4 + j].y = i;
		}
	}
	counter = 12;
	// and then we continue and add the grass pieces on every side.
	for(i = -1; i < 5; i++)
	{
		for(j = -1; j < 4; j++)
		{
			if(i < 4 && i > -1 && j > -1 && j < 3) continue;
			else
			{
				groundUnit[counter].x = i;
				groundUnit[counter].y = j;
				counter++;
			}
		}
	}
	// then just load the textures.
	#if defined(__wii__)

	TPL_OpenTPLFromMemory(&groundTPL, (void *)osa1_tpl, osa1_tpl_size);
	TPL_GetTexture(&groundTPL, groundPart1, &groundUnit[0].texture);

	TPL_OpenTPLFromMemory(&groundTPL, (void *)osa2_tpl, osa2_tpl_size);
	TPL_GetTexture(&groundTPL, groundPart2, &groundUnit[1].texture);

	TPL_OpenTPLFromMemory(&groundTPL, (void *)osa3_tpl, osa3_tpl_size);
	TPL_GetTexture(&groundTPL, groundPart3, &groundUnit[2].texture);

	TPL_OpenTPLFromMemory(&groundTPL, (void *)osa4_tpl, osa4_tpl_size);
	TPL_GetTexture(&groundTPL, groundPart4, &groundUnit[3].texture);

	TPL_OpenTPLFromMemory(&groundTPL, (void *)osa5_tpl, osa5_tpl_size);
	TPL_GetTexture(&groundTPL, groundPart5, &groundUnit[4].texture);

	TPL_OpenTPLFromMemory(&groundTPL, (void *)osa6_tpl, osa6_tpl_size);
	TPL_GetTexture(&groundTPL, groundPart6, &groundUnit[5].texture);

	TPL_OpenTPLFromMemory(&groundTPL, (void *)osa7_tpl, osa7_tpl_size);
	TPL_GetTexture(&groundTPL, groundPart7, &groundUnit[6].texture);

	TPL_OpenTPLFromMemory(&groundTPL, (void *)osa8_tpl, osa8_tpl_size);
	TPL_GetTexture(&groundTPL, groundPart8, &groundUnit[7].texture);

	TPL_OpenTPLFromMemory(&groundTPL, (void *)osa9_tpl, osa9_tpl_size);
	TPL_GetTexture(&groundTPL, groundPart9, &groundUnit[8].texture);

	TPL_OpenTPLFromMemory(&groundTPL, (void *)osa10_tpl, osa10_tpl_size);
	TPL_GetTexture(&groundTPL, groundPart10, &groundUnit[9].texture);

	TPL_OpenTPLFromMemory(&groundTPL, (void *)osa11_tpl, osa11_tpl_size);
	TPL_GetTexture(&groundTPL, groundPart11, &groundUnit[10].texture);

	TPL_OpenTPLFromMemory(&groundTPL, (void *)osa12_tpl, osa12_tpl_size);
	TPL_GetTexture(&groundTPL, groundPart12, &groundUnit[11].texture);

	TPL_OpenTPLFromMemory(&groundTPL, (void *)grassTexture_tpl, grassTexture_tpl_size);
	TPL_GetTexture(&groundTPL, grassTexture, &groundUnit[12].texture);
	planeMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGX("data/models/plane.obj", "Plane",
		planeMesh, &planeListSize, &planeDisplayList) != 0) return -1;
	#else
	if(tryLoadingTextureGL(&(groundUnit[0].texture), "data/textures/kentta/osa1.tga", "part1") != 0) return -1;
	if(tryLoadingTextureGL(&(groundUnit[1].texture), "data/textures/kentta/osa2.tga", "part2") != 0) return -1;
	if(tryLoadingTextureGL(&(groundUnit[2].texture), "data/textures/kentta/osa3.tga", "part3") != 0) return -1;
	if(tryLoadingTextureGL(&(groundUnit[3].texture), "data/textures/kentta/osa4.tga", "part4") != 0) return -1;
	if(tryLoadingTextureGL(&(groundUnit[4].texture), "data/textures/kentta/osa5.tga", "part5") != 0) return -1;
	if(tryLoadingTextureGL(&(groundUnit[5].texture), "data/textures/kentta/osa6.tga", "part6") != 0) return -1;
	if(tryLoadingTextureGL(&(groundUnit[6].texture), "data/textures/kentta/osa7.tga", "part7") != 0) return -1;
	if(tryLoadingTextureGL(&(groundUnit[7].texture), "data/textures/kentta/osa8.tga", "part8") != 0) return -1;
	if(tryLoadingTextureGL(&(groundUnit[8].texture), "data/textures/kentta/osa9.tga", "part9") != 0) return -1;
	if(tryLoadingTextureGL(&(groundUnit[9].texture), "data/textures/kentta/osa10.tga", "part10") != 0) return -1;
	if(tryLoadingTextureGL(&(groundUnit[10].texture), "data/textures/kentta/osa11.tga", "part11") != 0) return -1;
	if(tryLoadingTextureGL(&(groundUnit[11].texture), "data/textures/kentta/osa12.tga", "part12") != 0) return -1;
	if(tryLoadingTextureGL(&(groundUnit[12].texture), "data/textures/grassTexture.tga", "grassTexture") != 0) return -1;
	planeMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/plane.obj", "Plane", planeMesh, &planeDisplayList) != 0) return -1;

	#endif
	return 0;
}