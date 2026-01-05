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
#define OTHER_STATS_X -0.12f
#define METER_X 0.32f
#define OUTS_X -0.82f
#define INFO_X -0.68f
#define BASES_X 0.50f
#define EVENT_TIMER_THRESHOLD (1.5 * (1 / (UPDATE_INTERVAL*1.0f/1000)))

static void drawSkyBox(const StateInfo* stateInfo, ResourceManager* rm);
static void drawStatistics2D(const StateInfo* stateInfo, double alpha, ResourceManager* rm, const RenderState* rs);
static int initLights(StateInfo* stateInfo);
static void initCamSettings(StateInfo* stateInfo);
static void loadGameScreenSettings(StateInfo* stateInfo, unsigned int* rng_seed);

int initGameScreen(StateInfo* stateInfo, ResourceManager* rm)
{
	int result;

	resource_manager_load_all_game_assets(rm);

	initCamSettings(stateInfo);

	stateInfo->localGameInfo->uiState.lastMeterX = 0;
	stateInfo->localGameInfo->uiState.lastSwingMeterX = 0;

	result = initLights(stateInfo);
	if(result != 0) {
		printf("Could not init lights. Exiting.");
	}

	result = initImmutableWorld(stateInfo, rm);
	if(result != 0) {
		printf("Could not init immutable world.");
		return -1;
	}

	result = initMutableWorld(stateInfo, rm);
	if(result != 0) {
		printf("Could not init ball. Exiting.");
		return -1;
	}

	return 0;
}

void updateGameScreen(StateInfo* stateInfo, MenuInfo* menuInfo, unsigned int* rng_seed)
{
	BallInfo* ballInfo = &(stateInfo->localGameInfo->ballInfo);
	CameraState* cs = &(stateInfo->localGameInfo->cameraState);
	// if we just changed here, load basic settings.
	if(stateInfo->changeScreen == 1) {
		stateInfo->changeScreen = 0;
		stateInfo->updated = 1;
		loadGameScreenSettings(stateInfo, rng_seed);
	}
	// with home-key, one can return to main menu.
	if(((stateInfo->keyStates)->released[0][KEY_HOME] || (stateInfo->keyStates)->released[1][KEY_HOME])) {
		if(stateInfo->localGameInfo->gameControl.pause == 0) {
			stateInfo->localGameInfo->gameControl.pause = 1;
		} else if(stateInfo->localGameInfo->gameControl.pause == 1) {
			stateInfo->changeScreen = 1;
			stateInfo->updated = 0;
			stateInfo->screen = SCREEN_MAIN_MENU;
			menuInfo->mode = MENU_ENTRY_NORMAL;
		}
	}
	if(stateInfo->localGameInfo->gameControl.pause == 1) {
		if(((stateInfo->keyStates)->released[0][KEY_2] || (stateInfo->keyStates)->released[1][KEY_2])) {
			stateInfo->localGameInfo->gameControl.pause = 0;
		}
	}
	// update camera
	cs->lastCamTargetLocation.x = cs->camTargetLocation.x;
	cs->lastCamTargetLocation.y = cs->camTargetLocation.y;
	cs->lastCamTargetLocation.z = cs->camTargetLocation.z;

	cs->lastCamLocation.y = cs->camLocation.y;
	cs->lastCamLocation.z = cs->camLocation.z;
	cs->lastCamLocation.x = cs->camLocation.x;
	// we follow the ball with our camera
	cs->camTargetLocation.x = ballInfo->location.x;
	cs->camTargetLocation.y = ballInfo->location.y*0.5f;
	cs->camTargetLocation.z = ballInfo->location.z;

	// and our camera also moves a bit with the ball.

	cs->camLocation.x = 0.1f*cs->camTargetLocation.x;
	// if we are behind the homebase
	if(stateInfo->localGameInfo->ballInfo.location.z > HOME_RADIUS) {
		cs->camLocation.y = cs->camTargetLocation.y - cs->camTargetLocation.z * 0.1f + 3.0f + 5.0f + (float)fabs(cs->camTargetLocation.x)/3;
		cs->camLocation.z = cs->camTargetLocation.z - 0.3f*cs->camTargetLocation.z + 12.0f + 15.0f + (float)fabs(cs->camTargetLocation.x)/2;
	}
	// if runner coming from third base
	else if(stateInfo->localGameInfo->cameraState.homeRunCameraFlag == 0 ) {
		cs->camLocation.y = cs->camTargetLocation.y - cs->camTargetLocation.z * 0.1f + 3.0f;
		cs->camLocation.z = cs->camTargetLocation.z - 0.3f*cs->camTargetLocation.z + 12.0f;
	}
	// and the normal mode
	else {
		cs->camLocation.y = cs->camTargetLocation.y - cs->camTargetLocation.z * 0.1f + 3.0f + 5.0f;
		cs->camLocation.z = cs->camTargetLocation.z - 0.3f*cs->camTargetLocation.z + 12.0f + 8.0f;
	}
	// cant update this at the drawing-function as that doesnt happen synchroniusly.
	// so when info event happens, we set gameInfoEventTimer to 0 and this will start increasing it until
	// we hit the threshold.
	if(stateInfo->localGameInfo->uiState.gameInfoEventTimer != -1) {
		stateInfo->localGameInfo->uiState.gameInfoEventTimer+=1;

		if(stateInfo->localGameInfo->uiState.gameInfoEventTimer > (int)EVENT_TIMER_THRESHOLD) {
			stateInfo->localGameInfo->uiState.gameInfoEventTimer = -1;
		}
	}

	// and here will a lot of logic code.
	updateMutableWorld(stateInfo, menuInfo, rng_seed);

}

void drawGameScreen(const StateInfo* stateInfo, double alpha, ResourceManager* rm, const RenderState* rs)
{
	// Ensure the 3D rendering state is correctly set up before drawing the game screen.
	begin_3d_render(rs);
	CameraState* cs = (CameraState*)&(stateInfo->localGameInfo->cameraState);
	Vector3D look, cam, skyBoxLook;

	look.x = (float)(alpha*cs->camTargetLocation.x + (1-alpha)*cs->lastCamTargetLocation.x);
	look.y = (float)(alpha*cs->camTargetLocation.y + (1-alpha)*cs->lastCamTargetLocation.y);
	look.z = (float)(alpha*cs->camTargetLocation.z + (1-alpha)*cs->lastCamTargetLocation.z);
	cam.y = (float)(alpha*cs->camLocation.y + (1-alpha)*cs->lastCamLocation.y);
	cam.z = (float)(alpha*cs->camLocation.z + (1-alpha)*cs->lastCamLocation.z);
	cam.x = (float)(alpha*cs->camLocation.x + (1-alpha)*cs->lastCamLocation.x);
	// with skybox we move to origin
	skyBoxLook.x = look.x - cam.x;
	skyBoxLook.y = look.y - cam.y;
	skyBoxLook.z = look.z - cam.z;
	// first we draw the skybox. It should not be lit or write to the depth buffer.
	glDisable(GL_LIGHTING);
	glDepthMask(GL_FALSE);
	glLoadIdentity();
	gluLookAt(cs->skyBoxCam.x, cs->skyBoxCam.y, cs->skyBoxCam.z, skyBoxLook.x, skyBoxLook.y, skyBoxLook.z, cs->up.x, cs->up.y, cs->up.z);

	drawSkyBox(stateInfo, rm);

	// Re-enable depth writes for the rest of the scene.
	glDepthMask(GL_TRUE);

	glLoadIdentity();
	gluLookAt(cam.x, cam.y, cam.z, look.x, look.y, look.z, cs->up.x, cs->up.y, cs->up.z);
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_POSITION, cs->lightPos);

	drawImmutableWorld(stateInfo, alpha, rm);
	drawMutableWorld(stateInfo, alpha, rm);

	// statistics - Switch to 2D
	begin_2d_render(rs);
	drawStatistics2D(stateInfo, alpha, rm, rs);
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
	CameraState* cs = &(stateInfo->localGameInfo->cameraState);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	cs->lightPos[0] = LIGHT_SOURCE_POSITION_X;
	cs->lightPos[1] = LIGHT_SOURCE_POSITION_Y;
	cs->lightPos[2] = LIGHT_SOURCE_POSITION_Z;
	cs->lightPos[3] = 1.0f;
	glLightfv(GL_LIGHT0, GL_POSITION, cs->lightPos);

	return 0;
}

static void drawSkyBox(const StateInfo* stateInfo, ResourceManager* rm)
{
	glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, "data/textures/skybox.tga"));
	glPushMatrix();
	glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
	glCallList(resource_manager_get_model(rm, "data/models/skybox.obj"));
	glPopMatrix();
}

static void drawStatistics2D(const StateInfo* stateInfo, double alpha, ResourceManager* rm, const RenderState* rs)
{
	char* str4;
	char str5[2] = "x";
	float swingMeterX;
	float meterX;
	UIState* us = &(stateInfo->localGameInfo->uiState);

	// Layout Constants
	const float BASE_Y = VIRTUAL_HEIGHT - 100.0f; // Lowered bar
	const float TEXT_Y = BASE_Y + 20.0f; // Shifted 10px down relative to old (Old was BASE_Y(-120)+30 = -90)
	const float CENTER_X = VIRTUAL_WIDTH / 2.0f;
	const float SCALE_FACTOR = 1350.0f; // Increased for more spread
	const float FONT_SIZE = 40.0f;

	// Background
	// Draw a dark strip at the bottom
	glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, "data/textures/empty_background.tga"));
	// Using a simple quad logic here to avoid messing up with scaling of the plane model
	// Actually draw_texture_2d does exactly this.
	draw_texture_2d(resource_manager_get_texture(rm, "data/textures/empty_background.tga"), 0, BASE_Y, VIRTUAL_WIDTH, 80.0f); // Height 80 (was 120)

	// OUTS (Far Left)
	float outs_x = CENTER_X + (OUTS_X * SCALE_FACTOR);
	if(stateInfo->globalGameInfo->period < 4) {
		switch(stateInfo->localGameInfo->gameState.outs) {
		case 3:
			printText2D("XXX", 3, outs_x, TEXT_Y, FONT_SIZE);
			break;
		case 2:
			printText2D("XX", 2, outs_x, TEXT_Y, FONT_SIZE);
			break;
		case 1:
			printText2D("X", 1, outs_x, TEXT_Y, FONT_SIZE);
			break;
		case 0:
		default:
			break;
		}
	}

	// INFO AREA (Left-Center)
	float info_x = CENTER_X + (INFO_X * SCALE_FACTOR);

	if(stateInfo->localGameInfo->gameState.event != EVENT_NONE) {
		us->gameInfoEventTimer = 0;
		us->gameInfoEvent = (int)stateInfo->localGameInfo->gameState.event;
		stateInfo->localGameInfo->gameState.event = EVENT_NONE;
	}

	if(us->gameInfoEventTimer != -1) {
		const char* msg = "";
		switch(us->gameInfoEvent) {
		case EVENT_OUT:
			msg = "OUT";
			break;
		case EVENT_WOUNDED:
			msg = "WOUNDED";
			break;
		case EVENT_RUN_SCORED:
			msg = "RUN";
			break;
		case EVENT_OUT_OF_BOUNDS:
			msg = "OUT OF BOUNDS";
			break;
		case EVENT_STRIKE:
			msg = "STRIKE";
			break;
		case EVENT_BALL:
			msg = "BALL";
			break;
		case EVENT_INNING_ENDING:
			msg = "HALF-INNING ENDS";
			break;
		case EVENT_NEXT_PAIR:
			msg = "NEXT PAIR";
			break;
		case EVENT_TWO_RUNS_SCORED:
			msg = "TWO RUNS";
			break;
		}
		printText2D(msg, (unsigned int)strlen(msg), info_x, TEXT_Y, FONT_SIZE);
	} else if(stateInfo->localGameInfo->gameControl.waitingForFreeWalkDecision == 1) {
		printText2D("Take a walk", 11, info_x, TEXT_Y, FONT_SIZE);
	} else {
		// Player selection info
		int index;
		int shouldContinue = 1;
		if(stateInfo->globalGameInfo->period < 4) {
			if(stateInfo->localGameInfo->pII.batterSelectionIndex == -1) shouldContinue = 0;
			else index = stateInfo->localGameInfo->pII.batterSelectionIndex;
		} else {
			int battingTeamIndex = (stateInfo->globalGameInfo->
			                        inning+stateInfo->globalGameInfo->playsFirst+stateInfo->globalGameInfo->period)%2;
			index = stateInfo->globalGameInfo->teams[battingTeamIndex].
			        batterRunnerIndices[0][stateInfo->localGameInfo->gameModeState.runnerBatterPairCounter];
			if(index == -1) shouldContinue = 0;
		}
		if(shouldContinue == 1) {
			int speed = stateInfo->localGameInfo->playerInfo[index].bTPI.speed;
			int power = stateInfo->localGameInfo->playerInfo[index].bTPI.power;
			char p_str[32];
			sprintf(p_str, "S%d P%d", speed, power);
			printText2D(p_str, (unsigned int)strlen(p_str), info_x, TEXT_Y, FONT_SIZE);

			if(stateInfo->localGameInfo->playerInfo[index].bTPI.joker != JOKER_REGULAR && stateInfo->globalGameInfo->period < 4) {
				str5[0] = 'J';
			} else {
				str5[0] = (char)(((int)'0')+stateInfo->localGameInfo->playerInfo[index].bTPI.number);
			}
			// Number at +180 (gap 40 from S/P), Name at +250 (gap 42 from Number)
			printText2D(str5, 1, info_x + 180.0f, TEXT_Y, FONT_SIZE);

			str4 = stateInfo->localGameInfo->playerInfo[index].bTPI.name;
			printText2D(str4, (unsigned int)strlen(str4), info_x + 250.0f, TEXT_Y, FONT_SIZE);
		}
	}

	// STATS (Center)
	float stats_x = CENTER_X + (OTHER_STATS_X * SCALE_FACTOR);

	// Inning
	char inn_str[16] = "I  ";
	if(stateInfo->globalGameInfo->period < 4) {
		inn_str[2] = (char)(((int)'0')+ stateInfo->globalGameInfo->inning/2 + 1);
	} else {
		inn_str[2] = '0';
	}
	printText2D(inn_str, 3, stats_x - 140.0f, TEXT_Y, FONT_SIZE); // Moved Left

	// Balls
	char ball_str[16] = "B  ";
	if(stateInfo->localGameInfo->gameState.balls < 10) {
		ball_str[2] = (char)(((int)'0')+stateInfo->localGameInfo->gameState.balls);
	} else {
		sprintf(ball_str, "B%d", stateInfo->localGameInfo->gameState.balls);
	}
	printText2D(ball_str, (unsigned int)strlen(ball_str), stats_x - 20.0f, TEXT_Y, FONT_SIZE); // Evenly spaced

	// Strikes
	char strike_str[16] = "S  ";
	strike_str[2] = (char)(((int)'0')+stateInfo->localGameInfo->gameState.strikes);
	printText2D(strike_str, 3, stats_x + 100.0f, TEXT_Y, FONT_SIZE); // Evenly spaced

	// Runs
	char runs_str[32];
	sprintf(runs_str, "R %d - %d", stateInfo->globalGameInfo->teams[0].runs, stateInfo->globalGameInfo->teams[1].runs);
	printText2D(runs_str, (unsigned int)strlen(runs_str), stats_x + 220.0f, TEXT_Y, FONT_SIZE); // Moved Right


	// METER (Right-Center)
	float meter_screen_x = CENTER_X + (METER_X * SCALE_FACTOR);
	float meter_y = BASE_Y + 17.0f; // Shifted 12px down relative to old (Old was BASE_Y(-120)+25 = -95)
	float meter_w = 200.0f;
	float meter_h = 40.0f;

	draw_texture_2d(resource_manager_get_texture(rm, "data/textures/meter.tga"), meter_screen_x, meter_y, meter_w, meter_h);

	// Meter Markers
	meterX = 0.16f*stateInfo->localGameInfo->pRAI.meterValue;
	swingMeterX = 0.16f*stateInfo->localGameInfo->pRAI.swingMeterValue;

	float field_marker_x = meter_screen_x + (alpha*meterX + (1-alpha)*us->lastMeterX) / 0.16f * meter_w;
	float swing_marker_x = meter_screen_x + (alpha*swingMeterX + (1-alpha)*us->lastSwingMeterX) / 0.16f * meter_w;

	us->lastMeterX = meterX;
	us->lastSwingMeterX = swingMeterX;

	draw_texture_2d(resource_manager_get_texture(rm, "data/textures/selectionBall3.tga"), field_marker_x, meter_y, 5.0f, 40.0f);
	draw_texture_2d(resource_manager_get_texture(rm, "data/textures/selectionBall1.tga"), swing_marker_x, meter_y, 5.0f, 40.0f);


	// BASES (Far Right)
	float bases_screen_x = CENTER_X + (BASES_X * SCALE_FACTOR);
	float bases_y = BASE_Y + 17.0f; // Shifted 12px down relative to old (Old was BASE_Y(-120)+25 = -95)
	float bases_w = 200.0f;
	float bases_h = 40.0f;

	draw_texture_2d(resource_manager_get_texture(rm, "data/textures/bases.tga"), bases_screen_x, bases_y, bases_w, bases_h);

	// Base Runners
	for(int i = 0; i < BASE_COUNT; i++) {
		int index = stateInfo->localGameInfo->pII.battingTeamOnFieldIndices[i];
		if(index != -1) {
			float phase = 0.0f;
			int base = 0;
			float distance;
			// Logic copied from original
			if(stateInfo->localGameInfo->playerInfo[index].bTPI.baseId == BASE_HOME) {
				if(stateInfo->localGameInfo->playerInfo[index].tPI.location.z - HOME_LINE_Z > 0) {
					phase = 0.0f;
				} else {
					distance = stateInfo->fieldPositions->firstBaseRun.z - HOME_LINE_Z;
					phase = (stateInfo->localGameInfo->playerInfo[index].tPI.location.z - HOME_LINE_Z) / distance;
				}
				base = 0;
			} else if(stateInfo->localGameInfo->playerInfo[index].bTPI.baseId == BASE_FIRST) {
				distance = stateInfo->fieldPositions->secondBaseRun.x - stateInfo->fieldPositions->firstBaseRun.x;
				phase = (stateInfo->localGameInfo->playerInfo[index].tPI.location.x - stateInfo->fieldPositions->firstBaseRun.x) / distance;
				base = 1;
			} else if(stateInfo->localGameInfo->playerInfo[index].bTPI.baseId == BASE_SECOND) {
				distance = stateInfo->fieldPositions->thirdBaseRun.x - stateInfo->fieldPositions->secondBaseRun.x;
				phase = (stateInfo->localGameInfo->playerInfo[index].tPI.location.x - stateInfo->fieldPositions->secondBaseRun.x) / distance;
				base = 2;
			} else if(stateInfo->localGameInfo->playerInfo[index].bTPI.baseId == BASE_THIRD) {
				distance = HOME_LINE_Z - stateInfo->fieldPositions->thirdBaseRun.z;
				phase = (stateInfo->localGameInfo->playerInfo[index].tPI.location.z - stateInfo->fieldPositions->thirdBase.z) / distance;
				base = 3;
			} else if(stateInfo->localGameInfo->playerInfo[index].bTPI.baseId == BASE_HOME_SCORED) {
				phase = 0.0f;
				base = 4;
			}

			// Map to screen
			// width is bases_w
			float interval_px = bases_w / 4.0f;
			float runner_x = bases_screen_x + (base * interval_px) + (phase * interval_px);

			draw_texture_2d(resource_manager_get_texture(rm, "data/textures/basesMarker.tga"), runner_x, bases_y + 10.0f, 15.0f, 15.0f);
		}
	}
}


static void loadGameScreenSettings(StateInfo* stateInfo, unsigned int* rng_seed)
{
	stateInfo->localGameInfo->uiState.gameInfoEventTimer = -1;
	// initialize cam
	initCamSettings(stateInfo);
	// this will initialize all player settings etc with knowledge in structures from main menu.
	loadMutableWorldSettings(stateInfo, rng_seed);
}

static void initCamSettings(StateInfo* stateInfo)
{
	CameraState* cs = &(stateInfo->localGameInfo->cameraState);

	cs->lastCamTargetLocation.x = 0.0f;
	cs->lastCamTargetLocation.y = 0.0f;
	cs->lastCamTargetLocation.z = 0.0f;
	cs->camTargetLocation.x = 0.0f;
	cs->camTargetLocation.y = 0.0f;
	cs->camTargetLocation.z = -1.0f;
	cs->lastCamLocation.x = 0.0f;
	cs->lastCamLocation.y = 0.0f;
	cs->lastCamLocation.z = 0.0f;
	cs->camLocation.x = 0.0f;
	cs->camLocation.y = 0.0f;
	cs->camLocation.z = 0.0f;

	cs->cam.x = cs->camLocation.x;
	cs->cam.y = 0.0f;
	cs->cam.z = 0.0f;
	cs->up.x = 0.0f;
	cs->up.y = 1.0f;
	cs->up.z = 0.0f;
	cs->look.x = 0.0f;
	cs->look.y = 0.0f;
	cs->look.z = 0.0f;

	cs->skyBoxCam.x = 0.0f;
	cs->skyBoxCam.y = 0.0f;
	cs->skyBoxCam.z = 0.0f;
	cs->skyBoxLook.x = 0.0f;
	cs->skyBoxLook.y = 0.0f;
	cs->skyBoxLook.z = -1.0f;
	// these values are kinda trial and error values to move the statistics window to right place. i think proper deriving is unnecessary as we use perspective
	// projection at the same time.

	// to move statistics to safe zone of tv
	cs->statCam.x = 0.0f;
	cs->statCam.y = 1.9f;
	cs->statCam.z = -1.69f;
	cs->statUp.x = 0.0f;
	cs->statUp.y = 0.0f;
	cs->statUp.z = -1.0f;
	cs->statLook.x = 0.0f;
	cs->statLook.y = 0.0f;
	cs->statLook.z = -1.69f;

}

int cleanGameScreen(StateInfo* stateInfo)
{
	int result;

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