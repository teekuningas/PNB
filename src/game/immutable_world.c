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

static int initGround(StateInfo* stateInfo, ResourceManager* rm);
static void drawGround(const StateInfo* stateInfo, ResourceManager* rm);

static int initFence(ResourceManager* rm);
static void drawFence(ResourceManager* rm);

static int initPlate(ResourceManager* rm);
static void drawPlate(ResourceManager* rm);

int initImmutableWorld(StateInfo* stateInfo, ResourceManager* rm)
{
	int result;

	result = initGround(stateInfo, rm);
	if(result != 0) {
		printf("Initialization of ground failed.");
		return result;
	}
	result = initFence(rm);
	if(result != 0) {
		printf("Initialization of fence failed.");
		return result;
	}
	result = initPlate(rm);
	if(result != 0) {
		printf("Initialization of plate failed.");
		return result;
	}
	field_init_positions(stateInfo->fieldPositions);

	return 0;
}


void drawImmutableWorld(const StateInfo* stateInfo, double alpha, ResourceManager* rm)
{
	drawGround(stateInfo, rm);
	drawFence(rm);
	drawPlate(rm);

}

static void drawPlate(ResourceManager* rm)
{
	// models' width and length are 2, 2, so thats why we divide by 2.
	// 0.5f is just so that it wouldnt be so high that shoes and ball will disappear in it.
	glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, "data/textures/plate.tga"));
	glPushMatrix();
	glScalef(PLATE_WIDTH/2, 0.5f, PLATE_WIDTH/2);
	glCallList(resource_manager_get_model(rm, "data/models/plate.obj"));
	glPopMatrix();
}

// Draw fence and ground
// optimized the inner loops a bit, took out everything i could from loops.
static void drawFence(ResourceManager* rm)
{
	int i;
	GLuint fenceTexture = resource_manager_get_texture(rm, "data/textures/fence.tga");
	GLuint planeModel = resource_manager_get_model(rm, "data/models/plane.obj");

	// BACK FENCE
	glBindTexture(GL_TEXTURE_2D, fenceTexture);
	for(i = 0; i < (int)(5*GROUND_WIDTH/FENCE_PIECE_WIDTH); i++) {
		glPushMatrix();
		glTranslatef(FENCE_PIECE_WIDTH/2 + FIELD_LEFT + i*FENCE_PIECE_WIDTH, 0.0f, FIELD_BACK);
		glScalef(FENCE_PIECE_WIDTH/2, FENCE_HEIGHT/2, 1.0f); // again, width and height of the model is 2
		glTranslatef(0.0f, 1.0f, 0.0f); // moves fence up so that its bottom is at the level of origin ( preparing for scale )
		glRotatef(90.0f,1.0f, 0.0f, 0.0f);
		glCallList(planeModel);
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
		glCallList(planeModel);
		glPopMatrix();
		glRotatef(-90.0f,1.0f, 0.0f, 0.0f);
		glCallList(planeModel);
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
		glCallList(planeModel);
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
		glCallList(planeModel);
		glPopMatrix();
	}
}

static void drawGround(const StateInfo* stateInfo, ResourceManager* rm)
{
	int i;
	const GroundUnit* gu = stateInfo->match->groundUnit;
	GLuint planeModel = resource_manager_get_model(rm, "data/models/plane.obj");

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
		glCallList(planeModel);
		glPopMatrix();
	}
}
// cleaning is good for people.
int cleanImmutableWorld(StateInfo* stateInfo)
{
	return 0;
}

static int initFence(ResourceManager* rm)
{
	return 0;
}

static int initPlate(ResourceManager* rm)
{
	return 0;
}

static int initGround(StateInfo* stateInfo, ResourceManager* rm)
{
	int i, j, counter;
	GroundUnit* gu = stateInfo->match->groundUnit;
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
			if(i < 4 &&i > -1 &&j > -1 &&j < 3) continue;
			else {
				gu[counter].x = i;
				gu[counter].y = j;
				counter++;
			}
		}
	}
	// then just load the textures.
	gu[0].texture = resource_manager_get_texture(rm, "data/textures/kentta/osa1.tga");
	gu[1].texture = resource_manager_get_texture(rm, "data/textures/kentta/osa2.tga");
	gu[2].texture = resource_manager_get_texture(rm, "data/textures/kentta/osa3.tga");
	gu[3].texture = resource_manager_get_texture(rm, "data/textures/kentta/osa4.tga");
	gu[4].texture = resource_manager_get_texture(rm, "data/textures/kentta/osa5.tga");
	gu[5].texture = resource_manager_get_texture(rm, "data/textures/kentta/osa6.tga");
	gu[6].texture = resource_manager_get_texture(rm, "data/textures/kentta/osa7.tga");
	gu[7].texture = resource_manager_get_texture(rm, "data/textures/kentta/osa8.tga");
	gu[8].texture = resource_manager_get_texture(rm, "data/textures/kentta/osa9.tga");
	gu[9].texture = resource_manager_get_texture(rm, "data/textures/kentta/osa10.tga");
	gu[10].texture = resource_manager_get_texture(rm, "data/textures/kentta/osa11.tga");
	gu[11].texture = resource_manager_get_texture(rm, "data/textures/kentta/osa12.tga");
	gu[12].texture = resource_manager_get_texture(rm, "data/textures/grassTexture.tga");

	return 0;
}
