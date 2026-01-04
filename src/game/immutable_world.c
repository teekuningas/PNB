/*
	here we have everything rendered everything or at least most of that stays still in the game, like fences and ground. also this is the natural place
	to fill our FieldPositions structure.
*/

#include "globals.h"
#include "render.h"
#include "immutable_world.h"
#include "field_layout.h"

#define GROUND_UNIT_COUNT 30
#define FENCE_HEIGHT 3.0f
#define FENCE_PIECE_WIDTH 4.0f
#define RUNNER_BASE_OFFSET 0.8f

static int initGround(StateInfo* stateInfo);
static void drawGround(const StateInfo* stateInfo);

static int initFence();
static void drawFence();

static int initPlate();
static void drawPlate();



static GLuint plateTexture;
static GLuint fenceTexture;

static MeshObject* plateMesh;
static GLuint plateDisplayList;

static MeshObject* planeMesh;
static GLuint planeDisplayList;

int initImmutableWorld(StateInfo* stateInfo)
{
	int result;

	result = initGround(stateInfo);
	if(result != 0) {
		printf("Initialization of ground failed.");
		return result;
	}
	result = initFence();
	if(result != 0) {
		printf("Initialization of fence failed.");
		return result;
	}
	result = initPlate();
	if(result != 0) {
		printf("Initialization of plate failed.");
		return result;
	}
	field_init_positions(stateInfo->fieldPositions);

	return 0;
}


void drawImmutableWorld(const StateInfo* stateInfo, double alpha)
{
	drawGround(stateInfo);
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
	for(i = 0; i < (int)(5*GROUND_WIDTH/FENCE_PIECE_WIDTH); i++) {
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
	for(i = 0; i < (int)(5*GROUND_WIDTH/FENCE_PIECE_WIDTH); i++) {
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
	for(i = 0; i < (int)(6*GROUND_LENGTH/FENCE_PIECE_WIDTH); i++) {
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
	for(i = 0; i < (int)(6*GROUND_LENGTH/FENCE_PIECE_WIDTH) ; i++) {
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

static void drawGround(const StateInfo* stateInfo)
{
	int i;
	const GroundUnit* gu = stateInfo->localGameInfo->groundUnit;
	// here we use groundUnit[12].texture for all grass ground pieces.
	for(i = 0; i < GROUND_UNIT_COUNT; i++) {
		if(i < 12) {
			glBindTexture(GL_TEXTURE_2D, gu[i].texture);
		} else {
			glBindTexture(GL_TEXTURE_2D, gu[12].texture);
		}
		glPushMatrix();
		glTranslatef(GROUND_WIDTH*gu[i].y,0.0f, -GROUND_LENGTH*gu[i].x);
		glTranslatef(-GROUND_WIDTH + GROUND_OFFSET_X, 0.0f, GROUND_OFFSET_Z);
		glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
		glScalef(GROUND_LENGTH/2, 1.0f, GROUND_WIDTH/2);
		glCallList(planeDisplayList);
		glPopMatrix();
	}
}
// cleaning is good for people.
int cleanImmutableWorld(StateInfo* stateInfo)
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

static int initGround(StateInfo* stateInfo)
{
	int i, j, counter;
	GroundUnit* gu = stateInfo->localGameInfo->groundUnit;
	// first we create the play area, in order of groundUnit[0] being lowerleft, groundUnit[1] being second in left etc.
	for(i = 0; i < 3; i++) {
		for(j = 0; j < 4; j++) {
			gu[i*4 + j].x = j;
			gu[i*4 + j].y = i;
		}
	}
	counter = 12;
	// and then we continue and add the grass pieces on every side.
	for(i = -1; i < 5; i++) {
		for(j = -1; j < 4; j++) {
			if(i < 4 && i > -1 && j > -1 && j < 3) continue;
			else {
				gu[counter].x = i;
				gu[counter].y = j;
				counter++;
			}
		}
	}
	// then just load the textures.
	if(tryLoadingTextureGL(&(gu[0].texture), "data/textures/kentta/osa1.tga", "part1") != 0) return -1;
	if(tryLoadingTextureGL(&(gu[1].texture), "data/textures/kentta/osa2.tga", "part2") != 0) return -1;
	if(tryLoadingTextureGL(&(gu[2].texture), "data/textures/kentta/osa3.tga", "part3") != 0) return -1;
	if(tryLoadingTextureGL(&(gu[3].texture), "data/textures/kentta/osa4.tga", "part4") != 0) return -1;
	if(tryLoadingTextureGL(&(gu[4].texture), "data/textures/kentta/osa5.tga", "part5") != 0) return -1;
	if(tryLoadingTextureGL(&(gu[5].texture), "data/textures/kentta/osa6.tga", "part6") != 0) return -1;
	if(tryLoadingTextureGL(&(gu[6].texture), "data/textures/kentta/osa7.tga", "part7") != 0) return -1;
	if(tryLoadingTextureGL(&(gu[7].texture), "data/textures/kentta/osa8.tga", "part8") != 0) return -1;
	if(tryLoadingTextureGL(&(gu[8].texture), "data/textures/kentta/osa9.tga", "part9") != 0) return -1;
	if(tryLoadingTextureGL(&(gu[9].texture), "data/textures/kentta/osa10.tga", "part10") != 0) return -1;
	if(tryLoadingTextureGL(&(gu[10].texture), "data/textures/kentta/osa11.tga", "part11") != 0) return -1;
	if(tryLoadingTextureGL(&(gu[11].texture), "data/textures/kentta/osa12.tga", "part12") != 0) return -1;
	if(tryLoadingTextureGL(&(gu[12].texture), "data/textures/grassTexture.tga", "grassTexture") != 0) return -1;
	planeMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/plane.obj", "Plane", planeMesh, &planeDisplayList) != 0) return -1;
	return 0;
}
