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
	glBindTexture(GL_TEXTURE_2D, plateTexture);
	glPushMatrix();
	glScalef(PLATE_WIDTH/2, 0.5f, PLATE_WIDTH/2);
	glCallList(plateDisplayList);
	glPopMatrix();
}

// Draw fence and ground

// optimized the inner loops a bit, took out everything i could from loops.

static void drawFence()
{
	int i;
	// BACK FENCE
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
	// FRONT FENCE
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
	// LEFT FENCE
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
	// RIGHT FENCE
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
}

static void drawGround()
{
	int i;
	// here we use groundUnit[12].texture for all grass ground pieces.
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
}
// cleaning is good for people.
int cleanImmutableWorld()
{
	cleanMesh(planeMesh);
	cleanMesh(plateMesh);

	return 0;
}

static int initFence()
{
	if(tryLoadingTextureGL(&fenceTexture, "data/textures/fence.tga", "fence") != 0) return -1;

	return 0;
}

static int initPlate()
{
	if(tryLoadingTextureGL(&plateTexture, "data/textures/plate.tga", "plate") != 0) return -1;
	plateMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/plate.obj", "Cylinder", plateMesh, &plateDisplayList) != 0) return -1;
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
	return 0;
}
