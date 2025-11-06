/*
	game divides into two sections, menus and the game. this is the game screen. only main.c is higher but lots of initialization code is still hidden to
	main.c. almost all visible 3d stuff is delegated to mutable_world and immutable_world, but here we draw the
	skybox and some status information for user.
*/

#include "globals.h"
#include "immutable_world.h"
#include "mutable_world.h"
#include "render.h"
#include "font.h"
#include "game_screen.h"
#include "common_logic.h"

#define STATISTICS_TEXT_HEIGHT -1.34f
#define OTHER_STATS_X -0.03f
#define METER_X 0.31f
#define OUTS_X -0.63f
#define INFO_X -0.53f
#define BASES_X 0.57f
#define EVENT_TIMER_THRESHOLD (1.5 * (1 / (UPDATE_INTERVAL*1.0f/1000)))

static int gameInfoEventTimer;
static int gameInfoEvent;

static void drawSkyBox(StateInfo* stateInfo);
static void drawStatistics(StateInfo* stateInfo, double alpha);
static int initLights(StateInfo* stateInfo);
static void initCamSettings(StateInfo* stateInfo);
static void loadGameScreenSettings(StateInfo* stateInfo);

static Vector3D cam, look, up;
static Vector3D statCam, statLook, statUp;
static Vector3D skyBoxCam, skyBoxLook;
static float lightPos[4];

static Vector3D lastCamTargetLocation;
static Vector3D lastCamLocation;
static Vector3D camLocation;
static Vector3D camTargetLocation;

static float lastMeterX;
static float lastSwingMeterX;

static GLuint skyTexture;
static GLuint meterTexture;
static GLuint selectionTextureBatting;
static GLuint selectionTextureFielding;
static GLuint basesTexture;
static GLuint basesMarkerTexture;

static MeshObject* planeMesh;
static GLuint planeDisplayList;

static MeshObject* skyMesh;
static GLuint skyDisplayList;


int initGameScreen(StateInfo* stateInfo)
{
	int result;

	if(tryLoadingTextureGL(&skyTexture, "data/textures/skybox.tga", "sky") != 0) return -1;
	if(tryLoadingTextureGL(&meterTexture, "data/textures/meter.tga", "meter") != 0) return -1;
	if(tryLoadingTextureGL(&selectionTextureBatting, "data/textures/selectionBall1.tga", "selection1") != 0) return -1;
	if(tryLoadingTextureGL(&selectionTextureFielding, "data/textures/selectionBall3.tga", "selection3") != 0) return -1;
	if(tryLoadingTextureGL(&basesTexture, "data/textures/bases.tga", "bases") != 0) return -1;
	if(tryLoadingTextureGL(&basesMarkerTexture, "data/textures/basesMarker.tga", "basesMarker") != 0) return -1;
	skyMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/skybox.obj", "Cube", skyMesh, &skyDisplayList) != 0) return -1;
	planeMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/plane.obj", "Plane", planeMesh, &planeDisplayList) != 0) return -1;

	initCamSettings(stateInfo);

	lastMeterX = 0;
	lastSwingMeterX = 0;

	result = initLights(stateInfo);
	if(result != 0) {
		printf("Could not init lights. Exiting.");
	}

	result = initImmutableWorld(stateInfo);
	if(result != 0) {
		printf("Could not init immutable world.");
		return -1;
	}

	result = initMutableWorld(stateInfo);
	if(result != 0) {
		printf("Could not init ball. Exiting.");
		return -1;
	}

	return 0;
}

void updateGameScreen(StateInfo* stateInfo, MenuInfo* menuInfo)
{
	BallInfo* ballInfo = &(stateInfo->localGameInfo->ballInfo);
	// if we just changed here, load basic settings.
	if(stateInfo->changeScreen == 1) {
		stateInfo->changeScreen = 0;
		stateInfo->updated = 1;
		loadGameScreenSettings(stateInfo);
	}
	// with home-key, one can return to main menu.
	if(((stateInfo->keyStates)->released[0][KEY_HOME] || (stateInfo->keyStates)->released[1][KEY_HOME])) {
		if(stateInfo->localGameInfo->gAI.pause == 0) {
			stateInfo->localGameInfo->gAI.pause = 1;
		} else if(stateInfo->localGameInfo->gAI.pause == 1) {
			stateInfo->changeScreen = 1;
			stateInfo->updated = 0;
			stateInfo->screen = MAIN_MENU;
			menuInfo->mode = MENU_ENTRY_NORMAL;
		}
	}
	if(stateInfo->localGameInfo->gAI.pause == 1) {
		if(((stateInfo->keyStates)->released[0][KEY_2] || (stateInfo->keyStates)->released[1][KEY_2])) {
			stateInfo->localGameInfo->gAI.pause = 0;
		}
	}
	// update camera
	lastCamTargetLocation.x = camTargetLocation.x;
	lastCamTargetLocation.y = camTargetLocation.y;
	lastCamTargetLocation.z = camTargetLocation.z;

	lastCamLocation.y = camLocation.y;
	lastCamLocation.z = camLocation.z;
	lastCamLocation.x = camLocation.x;
	// we follow the ball with our camera
	camTargetLocation.x = ballInfo->location.x;
	camTargetLocation.y = ballInfo->location.y*0.5f;
	camTargetLocation.z = ballInfo->location.z;

	// and our camera also moves a bit with the ball.

	camLocation.x = 0.1f*camTargetLocation.x;
	// if we are behind the homebase
	if(stateInfo->localGameInfo->ballInfo.location.z > HOME_RADIUS) {
		camLocation.y = camTargetLocation.y - camTargetLocation.z * 0.1f + 3.0f + 5.0f + (float)fabs(camTargetLocation.x)/3;
		camLocation.z = camTargetLocation.z - 0.3f*camTargetLocation.z + 12.0f + 15.0f + (float)fabs(camTargetLocation.x)/2;
	}
	// if runner coming from third base
	else if(stateInfo->localGameInfo->gAI.homeRunCameraFlag == 0 ) {
		camLocation.y = camTargetLocation.y - camTargetLocation.z * 0.1f + 3.0f;
		camLocation.z = camTargetLocation.z - 0.3f*camTargetLocation.z + 12.0f;
	}
	// and the normal mode
	else {
		camLocation.y = camTargetLocation.y - camTargetLocation.z * 0.1f + 3.0f + 5.0f;
		camLocation.z = camTargetLocation.z - 0.3f*camTargetLocation.z + 12.0f + 8.0f;
	}
	// cant update this at the drawing-function as that doesnt happen synchroniusly.
	// so when info event happens, we set gameInfoEventTimer to 0 and this will start increasing it until
	// we hit the threshold.
	if(gameInfoEventTimer != -1) {
		gameInfoEventTimer+=1;

		if(gameInfoEventTimer > (int)EVENT_TIMER_THRESHOLD) {
			gameInfoEventTimer = -1;
		}
	}

	// and here will a lot of logic code.
	updateMutableWorld(stateInfo, menuInfo);

}

void drawGameScreen(StateInfo* stateInfo, double alpha, const RenderState* rs)
{
	// Ensure the 3D rendering state is correctly set up before drawing the game screen.
	begin_3d_render(rs);

	look.x = (float)(alpha*camTargetLocation.x + (1-alpha)*lastCamTargetLocation.x);
	look.y = (float)(alpha*camTargetLocation.y + (1-alpha)*lastCamTargetLocation.y);
	look.z = (float)(alpha*camTargetLocation.z + (1-alpha)*lastCamTargetLocation.z);
	cam.y = (float)(alpha*camLocation.y + (1-alpha)*lastCamLocation.y);
	cam.z = (float)(alpha*camLocation.z + (1-alpha)*lastCamLocation.z);
	cam.x = (float)(alpha*camLocation.x + (1-alpha)*lastCamLocation.x);
	// with skybox we move to origin
	skyBoxLook.x = look.x - cam.x;
	skyBoxLook.y = look.y - cam.y;
	skyBoxLook.z = look.z - cam.z;
	// first we draw the skybox. It should not be lit or write to the depth buffer.
	glDisable(GL_LIGHTING);
	glDepthMask(GL_FALSE);
	glLoadIdentity();
	gluLookAt(skyBoxCam.x, skyBoxCam.y, skyBoxCam.z, skyBoxLook.x, skyBoxLook.y, skyBoxLook.z, up.x, up.y, up.z);

	drawSkyBox(stateInfo);

	// Re-enable depth writes for the rest of the scene.
	glDepthMask(GL_TRUE);

	glLoadIdentity();
	gluLookAt(cam.x, cam.y, cam.z, look.x, look.y, look.z, up.x, up.y, up.z);
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

	drawImmutableWorld(stateInfo, alpha);
	drawMutableWorld(stateInfo, alpha);

	// statistics
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glLoadIdentity();
	gluLookAt(statCam.x, statCam.y, statCam.z, statLook.x, statLook.y, statLook.z, statUp.x, statUp.y, statUp.z);

	drawStatistics(stateInfo, alpha);
}


// lights
static int initLights(StateInfo* stateInfo)
{
	// our lighting is a point light that is so far away that its practically a directional light.
	//
	// some settings, quite random, but reasonable.
	float globalAmb[] = {0.8f, 0.8f, 0.8f, 1.0f};
	float diffuse[] = {1.0f,1.0f,1.0f,1.0f};
	float ambient[] = {0.8f, 0.8f, 0.8f, 1.0f};
	float specular[] = {1.0f, 1.0f,1.0f,1.0f};
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	lightPos[0] = LIGHT_SOURCE_POSITION_X;
	lightPos[1] = LIGHT_SOURCE_POSITION_Y;
	lightPos[2] = LIGHT_SOURCE_POSITION_Z;
	lightPos[3] = 1.0f;
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

	return 0;
}

static void drawSkyBox(StateInfo* stateInfo)
{
	glBindTexture(GL_TEXTURE_2D, skyTexture);
	glPushMatrix();
	glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
	glCallList(skyDisplayList);
	glPopMatrix();
}

static void drawStatistics(StateInfo* stateInfo, double alpha)
{
	int i;
	char str[4] = "B  ";
	char str2[4] = "S  ";
	char str3[6] = "R  - ";
	char* str4;
	char str5[2] = "x";
	char str6[4] = "I  ";
	float swingMeterX;
	float meterX;
	// we draw the background for writing text. it will appear at the bottom of screen
	// because of the camera settings.
	drawFontBackground();
	if(stateInfo->globalGameInfo->period < 4) {
		switch(stateInfo->localGameInfo->gAI.outs) {
		case 0:
			break;
		case 3:
			printText("XXX", 3, OUTS_X, STATISTICS_TEXT_HEIGHT, 2);
			break;
		case 2:
			printText("XX", 2, OUTS_X, STATISTICS_TEXT_HEIGHT, 2);
			break;
		case 1:
			printText("X", 1, OUTS_X, STATISTICS_TEXT_HEIGHT, 2);
			break;
		default:
			printText("XXX", 3, OUTS_X, STATISTICS_TEXT_HEIGHT, 2);
			break;
		}
	}
	// inning?
	if(stateInfo->globalGameInfo->period < 4) {
		str6[2] = (char)(((int)'0')+ stateInfo->globalGameInfo->inning/2 + 1);
	} else {
		str6[2] = (char)(((int)'0'));
	}
	printText(str6, 3, OTHER_STATS_X - 0.04f, STATISTICS_TEXT_HEIGHT, 2);

	// here for outs and runs we have to take care of that sometimes we will have more than 9 of them
	if(stateInfo->localGameInfo->gAI.balls < 10)
		str[2] = (char)(((int)'0')+stateInfo->localGameInfo->gAI.balls);
	else {
		str[2] = (char)(((int)'0')+(stateInfo->localGameInfo->gAI.balls%10));
		str[1] = (char)(((int)'0')+(stateInfo->localGameInfo->gAI.balls/10));
	}
	printText(str, 3, OTHER_STATS_X + 0.04f, STATISTICS_TEXT_HEIGHT, 2);

	str2[2] = (char)(((int)'0')+stateInfo->localGameInfo->gAI.strikes);
	printText(str2, 3, OTHER_STATS_X + 0.12f, STATISTICS_TEXT_HEIGHT, 2);

	if(stateInfo->globalGameInfo->teams[0].runs < 10)
		str3[2] = (char)(((int)'0')+stateInfo->globalGameInfo->teams[0].runs);
	else {
		str3[2] = (char)(((int)'0')+(stateInfo->globalGameInfo->teams[0].runs%10));
		str3[1] = (char)(((int)'0')+(stateInfo->globalGameInfo->teams[0].runs/10));
	}
	if(stateInfo->globalGameInfo->teams[1].runs < 10)
		str3[4] = (char)(((int)'0')+stateInfo->globalGameInfo->teams[1].runs);
	else {
		str3[4] = (char)(((int)'0')+(stateInfo->globalGameInfo->teams[1].runs%10));
		str3[3] = (char)(((int)'0')+(stateInfo->globalGameInfo->teams[1].runs/10));
	}
	printText(str3, 5, OTHER_STATS_X + 0.2f, STATISTICS_TEXT_HEIGHT, 2);
	// so we have these events thats are triggered in other parts of code just by gameInfoEvent = i;
	// we have a counter so that the info will disappear after some time.
	if(stateInfo->localGameInfo->gAI.gameInfoEvent != 0) {
		gameInfoEventTimer = 0;
		gameInfoEvent = stateInfo->localGameInfo->gAI.gameInfoEvent;
		stateInfo->localGameInfo->gAI.gameInfoEvent = 0;
	}
	if(gameInfoEventTimer != -1) {
		if(gameInfoEvent == 1) {
			printText("        OUT", 11, INFO_X, STATISTICS_TEXT_HEIGHT, 2);
		}
		if(gameInfoEvent == 2) {
			printText("     WOUNDED", 12, INFO_X, STATISTICS_TEXT_HEIGHT, 2);
		}
		if(gameInfoEvent == 3) {
			printText("        RUN", 11, INFO_X, STATISTICS_TEXT_HEIGHT, 2);
		}
		if(gameInfoEvent == 4) {
			printText("  OUT OF BOUNDS", 15, INFO_X, STATISTICS_TEXT_HEIGHT, 2);
		}
		if(gameInfoEvent == 5) {
			printText("      STRIKE", 12, INFO_X, STATISTICS_TEXT_HEIGHT, 2);
		}
		if(gameInfoEvent == 6) {
			printText("        BALL", 12, INFO_X, STATISTICS_TEXT_HEIGHT, 2);
		}
		if(gameInfoEvent == 7) {
			printText("HALF-INNING ENDS", 16, INFO_X, STATISTICS_TEXT_HEIGHT, 2);
		}
		if(gameInfoEvent == 8) {
			printText("    NEXT PAIR", 13, INFO_X, STATISTICS_TEXT_HEIGHT, 2);
		}
		if(gameInfoEvent == 9) {
			printText("     TWO RUNS", 13, INFO_X, STATISTICS_TEXT_HEIGHT, 2);
		}
	}
	// when free walk decision and batter decision are both waiting at the same time, choose walk.
	else if(stateInfo->localGameInfo->gAI.waitingForFreeWalkDecision == 1) {
		printText(" Take a walk", 12, INFO_X, STATISTICS_TEXT_HEIGHT, 2);
	}
	// when waiting for batter decision we show "select" and players name and number
	// so that the decision making is easier.
	else {
		char str[5] = "S P ";
		int speed;
		int power;
		int index;
		int shouldContinue = 1;
		// index selection a bit different when homerunbatting contest.
		if(stateInfo->globalGameInfo->period < 4) {
			if(stateInfo->localGameInfo->pII.batterSelectionIndex == -1) shouldContinue = 0;
			else index = stateInfo->localGameInfo->pII.batterSelectionIndex;
		} else {
			int battingTeamIndex = (stateInfo->globalGameInfo->
			                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
			index = stateInfo->globalGameInfo->teams[battingTeamIndex].
			        batterRunnerIndices[0][stateInfo->localGameInfo->gAI.runnerBatterPairCounter];
			if(index == -1) shouldContinue = 0;
		}
		if(shouldContinue == 1) {
			speed = stateInfo->localGameInfo->playerInfo[index].bTPI.speed;
			power = stateInfo->localGameInfo->playerInfo[index].bTPI.power;
			str[1] = (char)(((int)'0')+(speed));
			str[3] = (char)(((int)'0')+(power));
			printText(str, 5, INFO_X, STATISTICS_TEXT_HEIGHT, 2);
			str4 = stateInfo->localGameInfo->playerInfo[index].bTPI.name;
			printText(str4, strlen(str4), INFO_X + 0.14f, STATISTICS_TEXT_HEIGHT, 2);
			if(stateInfo->localGameInfo->playerInfo[index].bTPI.joker != 0 && stateInfo->globalGameInfo->period < 4) {
				str5[0] = 'J';
			} else {
				str5[0] = (char)(((int)'0')+stateInfo->localGameInfo->playerInfo[index].bTPI.number);
			}
			printText(str5, 1, INFO_X + 0.11f, STATISTICS_TEXT_HEIGHT, 2);
		}
	}
	// then draw, bases- and meterTexture. and selections for meter. meterValues has been set in action_implementation.

	meterX = 0.16f*stateInfo->localGameInfo->pRAI.meterValue;
	swingMeterX = 0.16f*stateInfo->localGameInfo->pRAI.swingMeterValue;

	//players in bases
	glBindTexture(GL_TEXTURE_2D, basesTexture);
	glPushMatrix();
	glTranslatef(BASES_X, 1.0f, STATISTICS_TEXT_HEIGHT); // these numeric parameters come from
	glScalef(0.08f, 0.02f, 0.02f);
	glCallList(planeDisplayList);
	glPopMatrix();

	// meter
	glBindTexture(GL_TEXTURE_2D, meterTexture);
	glPushMatrix();
	glTranslatef(METER_X + 0.08f, 1.0f, STATISTICS_TEXT_HEIGHT);
	glScalef(0.08f, 0.02f, 0.02f);
	glCallList(planeDisplayList);
	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D, selectionTextureFielding);
	glPushMatrix();
	glTranslatef(METER_X + (float)(alpha*meterX + (1-alpha)*lastMeterX), 1.0f, STATISTICS_TEXT_HEIGHT);
	glScalef(0.002f, 0.01f, 0.02f);
	glCallList(planeDisplayList);
	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D, selectionTextureBatting);
	glPushMatrix();
	glTranslatef(METER_X + (float)(alpha*swingMeterX + (1-alpha)*lastSwingMeterX), 1.0f, STATISTICS_TEXT_HEIGHT);
	glScalef(0.002f, 0.01f, 0.02f);
	glCallList(planeDisplayList);
	glPopMatrix();

	lastMeterX = meterX;
	lastSwingMeterX = swingMeterX;


	// and then draw little disks over basesTexture to indicate where baserunners are.
	for(i = 0; i < BASE_COUNT; i++) {
		int index = stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[i];
		if(index != -1) {
			float left = BASES_X - 0.075f;
			float interval = 0.15f/4;
			float phase = 0.0f;
			int base = 0;
			float distance;
			// for batter we say at 0 until we have passed the homeline.
			if(stateInfo->localGameInfo->playerInfo[index].bTPI.base == 0) {
				if(stateInfo->localGameInfo->playerInfo[index].tPI.location.z -
				        HOME_LINE_Z > 0) {
					phase = 0.0f;
				} else {
					distance = stateInfo->fieldPositions->firstBaseRun.z - HOME_LINE_Z;
					phase = (stateInfo->localGameInfo->playerInfo[index].tPI.location.z -
					         HOME_LINE_Z) / distance;
				}
				base = 0;
			} else if(stateInfo->localGameInfo->playerInfo[index].bTPI.base == 1) {
				distance = stateInfo->fieldPositions->secondBaseRun.x - stateInfo->fieldPositions->firstBaseRun.x;
				phase = (stateInfo->localGameInfo->playerInfo[index].tPI.location.x -
				         stateInfo->fieldPositions->firstBaseRun.x) / distance;
				base = 1;
			} else if(stateInfo->localGameInfo->playerInfo[index].bTPI.base == 2) {
				distance = stateInfo->fieldPositions->thirdBaseRun.x - stateInfo->fieldPositions->secondBaseRun.x;
				phase = (stateInfo->localGameInfo->playerInfo[index].tPI.location.x -
				         stateInfo->fieldPositions->secondBaseRun.x) / distance;
				base = 2;
			} else if(stateInfo->localGameInfo->playerInfo[index].bTPI.base == 3) {
				distance = HOME_LINE_Z - stateInfo->fieldPositions->thirdBaseRun.z;
				phase = (stateInfo->localGameInfo->playerInfo[index].tPI.location.z -
				         stateInfo->fieldPositions->thirdBase.z) / distance;
				base = 3;
			} else if(stateInfo->localGameInfo->playerInfo[index].bTPI.base == 4) {
				// and for player who arrives homebase we just set  the marker to be at the last position.
				phase = 0.0f;
				base = 4;
			}
			// and then just draw.
			glBindTexture(GL_TEXTURE_2D, basesMarkerTexture);
			glPushMatrix();
			glTranslatef(left + interval*base + phase*interval,
			             1.0f, STATISTICS_TEXT_HEIGHT);
			glScalef(0.005f, 0.005f, 0.005f);
			glCallList(planeDisplayList);
			glPopMatrix();
		}
	}
}

static void loadGameScreenSettings(StateInfo* stateInfo)
{
	gameInfoEventTimer = -1;
	// initialize cam
	initCamSettings(stateInfo);
	// this will initialize all player settings etc with knowledge in structures from main menu.
	loadMutableWorldSettings(stateInfo);
}

static void initCamSettings(StateInfo* stateInfo)
{
	lastCamTargetLocation.x = 0.0f;
	lastCamTargetLocation.y = 0.0f;
	lastCamTargetLocation.z = 0.0f;
	camTargetLocation.x = 0.0f;
	camTargetLocation.y = 0.0f;
	camTargetLocation.z = -1.0f;
	lastCamLocation.x = 0.0f;
	lastCamLocation.y = 0.0f;
	lastCamLocation.z = 0.0f;
	camLocation.x = 0.0f;
	camLocation.y = 0.0f;
	camLocation.z = 0.0f;

	cam.x = camLocation.x;
	cam.y = 0.0f;
	cam.z = 0.0f;
	up.x = 0.0f;
	up.y = 1.0f;
	up.z = 0.0f;
	look.x = 0.0f;
	look.y = 0.0f;
	look.z = 0.0f;

	skyBoxCam.x = 0.0f;
	skyBoxCam.y = 0.0f;
	skyBoxCam.z = 0.0f;
	skyBoxLook.x = 0.0f;
	skyBoxLook.y = 0.0f;
	skyBoxLook.z = -1.0f;
	// these values are kinda trial and error values to move the statistics window to right place. i think proper deriving is unnecessary as we use perspective
	// projection at the same time.

	// to move statistics to safe zone of tv
	statCam.x = 0.0f;
	statCam.y = 1.9f;
	statCam.z = -1.69f;
	statUp.x = 0.0f;
	statUp.y = 0.0f;
	statUp.z = -1.0f;
	statLook.x = 0.0f;
	statLook.y = 0.0f;
	statLook.z = -1.69f;

}

int cleanGameScreen(StateInfo* stateInfo)
{
	int result;
	cleanMesh(skyMesh);
	cleanMesh(planeMesh);

	result = cleanImmutableWorld(stateInfo);
	if(result != 0) {
		printf("Could not clean immutable world properly.\n");
		return -1;
	}

	result = cleanMutableWorld(stateInfo);
	if(result != 0) {
		printf("Could not clean mutable world properly.\n");
		return -1;
	}

	return 0;
}
