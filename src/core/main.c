#include "globals.h"
#include "game_screen.h"
#include "input.h"
#include "sound.h"
#include "font.h"
#include "fill_player_data.h"
#include "main_menu.h"
#include "loading_screen_menu.h"
#include "menu_helpers.h"
#include "menu_types.h"
#include "resource_manager.h"

static int initGL(GLFWwindow** window, int fullscreen, RenderState* renderState);
static int clean(StateInfo* stateInfo, MenuData* menuData, ResourceManager* rm);
static void draw(StateInfo* stateInfo, MenuData* menuData, GLFWwindow* window, double alpha, ResourceManager* rm, RenderState* rs);
static int update(StateInfo* stateInfo, MenuData* menuData, GLFWwindow* window);

static MenuData menuData;
static StateInfo stateInfo;
static LocalGameInfo localGameInfo;
static GlobalGameInfo globalGameInfo;
static TournamentState tournamentState;
static MenuInfo menuInfo;
static KeyStates keyStates;
static FieldPositions fieldPositions;
static RenderState renderState;
static ResourceManager* resourceManager;

int main ( int argc, char *argv[] )
{

	double alpha;
	int done = 0;
	int fullscreen = 1;
	int result;

	unsigned int currentTime = 0;
	unsigned int newTime;
	unsigned int frameTime;
	unsigned int accumulator = 0;
	unsigned int updateInterval = UPDATE_INTERVAL;

	// Initialize the random number generator
	srand((unsigned int)time(NULL));

	// Handle the commandline arguments
	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "--windowed") == 0) {
			fullscreen = 0;
		}
	}

	printf("v. 1.5 beta\n");

	// Initialize stateInfo structure
	stateInfo.localGameInfo = &localGameInfo;
	stateInfo.globalGameInfo = &globalGameInfo;
	stateInfo.tournamentState = &tournamentState;
	stateInfo.keyStates = &keyStates;
	stateInfo.fieldPositions = &fieldPositions;
	stateInfo.teamData = NULL;
	stateInfo.stopSoundEffect = 0;

	resourceManager = resource_manager_init();
	if (resourceManager == NULL) {
		printf("Could not init resource manager. Exiting.");
		return -1;
	}

	GLFWwindow* window = NULL;
	result = initGL(&window, fullscreen, &renderState);
	if(result != 0) {
		printf("Could not init GL. Exiting.");
		return -1;
	}

	result = fillPlayerData(&stateInfo, "data/teams.xml");
	if(result != 0) {
		printf("Could not init team data. Exiting.");
		return -1;
	}

	result = initMainMenu(&stateInfo, &menuData, &menuInfo, resourceManager, &renderState);
	if(result != 0) {
		printf("Could not init main menu. Exiting.");
		return -1;
	}

	result = initInput(&stateInfo);
	if(result != 0) {
		printf("Could not init input. Exiting.");
		return -1;
	}
	result = initSound(&stateInfo);
	if(result != 0) {
		printf("Could not init sound system. Exiting.");
		return -1;
	}
	result = initFont();
	if(result != 0) {
		printf("Could not init font. Exiting.");
		return -1;
	}

	// draw loading screen before loading all the player meshes which will take time
	stateInfo.screen = LOADING_SCREEN;
	// we draw twice as at least my debian's graphics are drawn wrong sometimes at the first time.
	drawLoadingScreen(&stateInfo, &menuData, &menuInfo, resourceManager, &renderState);
	draw(&stateInfo, &menuData, window, 1.0, resourceManager, &renderState);

	result = initGameScreen(&stateInfo);
	if(result != 0) {
		printf("Could not init game screen. Exiting.");
		return -1;
	}

	stateInfo.screen = MAIN_MENU;
	stateInfo.changeScreen = 1;
	stateInfo.updated = 0;

	// to keep our fps steady. we are trying to draw as often as we can and update in fixed intervals.
	while(done == 0) {
		newTime = (unsigned int)(1000*glfwGetTime());
		frameTime = newTime - currentTime;
		currentTime = newTime;
		accumulator += frameTime;
		// update the scene every 20ms and if for some reason there is delay, keep updating until catched up
		while ( accumulator >= updateInterval ) {
			result = update(&stateInfo, &menuData, window);
			if (result != 0 || glfwWindowShouldClose(window)) {
				done = 1;
			}
			accumulator -= updateInterval;
		}

		alpha = (double)accumulator / updateInterval;
		// draw the scene, alpha will give us nice little smoothing effect.
		// like if we are in the middle of updateInterval, the "real" position of the object
		// isn't what it was on laste update call nor it is what it will be in the next call to update.
		// so we will draw it to the middle.
		if(stateInfo.updated == 1) {
			draw(&stateInfo, &menuData, window, alpha, resourceManager, &renderState);
		}

		glfwPollEvents();
	}
	// and we will clean up when everything ends
	result = clean(&stateInfo, &menuData, resourceManager);
	if(result != 0) {
		printf("Cleaning up unsuccessful. Exiting anyway.");
		return -1;
	}

	return 0;

}

static int update(StateInfo* stateInfo, MenuData* menuData, GLFWwindow* window)
{
	updateInput(stateInfo, window);
	updateSound(stateInfo);
	switch(stateInfo->screen) {
	case GAME_SCREEN:
		updateGameScreen(stateInfo, &menuInfo);
		break;
	case MAIN_MENU:
		updateMainMenu(stateInfo, menuData, &menuInfo, &keyStates);
		break;
	default:
		return 1;
	}
	return 0;
}


static void draw(StateInfo* stateInfo, MenuData* menuData, GLFWwindow* window, double alpha, ResourceManager* rm, RenderState* rs)


{
	switch(stateInfo->screen) {
	case GAME_SCREEN:
		// Everything within drawGameScreen is currently drawn in 3d context
		drawGameScreen(stateInfo, alpha, rs);
		break;
	case MAIN_MENU:
		drawMainMenu(stateInfo, menuData, &menuInfo, alpha, rm, rs);
		break;
	case LOADING_SCREEN:
		break;
	}
	glfwSwapBuffers(window);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
}

static int initGL(GLFWwindow** window, int fullscreen, RenderState* renderState)
{
	const GLFWvidmode* mode;
	GLFWmonitor* monitor;
	int width;
	int height;

	// Initialize glfw
	if( !glfwInit() ) {
		fprintf( stderr, "Failed to initialize GLFW\n" );
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	monitor = glfwGetPrimaryMonitor();
	mode = glfwGetVideoMode(monitor);

	if(fullscreen == 0) {
		width = (int)(mode->width * (3.0/4));
		height = (int)(mode->height * (3.0/4));

		*window = glfwCreateWindow(width, height, "PNB", NULL, NULL);
		if(!window) {
			fprintf(stderr, "Failed to open GLFW window\n");
			glfwTerminate();
			return -1;
		}
	} else {
		glfwWindowHint(GLFW_DECORATED, GL_FALSE);
		width = (int)(mode->width);
		height = (int)(mode->height);

		*window = glfwCreateWindow(width, height, "PNB", NULL, NULL);
		if(!*window) {
			fprintf(stderr, "Failed to open GLFW window\n");
			glfwTerminate();
			return -1;
		}
		glfwSetWindowMonitor(*window, monitor, 0, 0, width, height, mode->refreshRate);
	}

	renderState->window_width = width;
	renderState->window_height = height;

	glfwMakeContextCurrent(*window);

	// Initialize GLEW
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		printf("glew");
		return -1;
	}

	glfwSwapInterval(0);

	// and then initialize openGL settings. nothing really weird here.
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glViewport(0, 0, width, height);
	// this blendfunc works like that it doesnt draw anything with color data from shadow mesh, but it will
	// use this alpha value to reduce intensity of the background of the mesh.
	glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
	glColor4f(1.0f, 1.0f, 1.0f, 0.5f);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f,PERSPECTIVE_ASPECT_RATIO,0.1f,250.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	return 0;
}

static int clean(StateInfo* stateInfo, MenuData* menuData, ResourceManager* rm)
{
	int result;
	int retvalue = 0;
	result = cleanPlayerData(stateInfo);
	if(result != 0) {
		printf("Could not clean player data completely\n");
		retvalue = -1;
	}
	result = cleanGameScreen(stateInfo);
	if(result != 0) {
		printf("Could not clean game screen completely\n");
		retvalue = -1;
	}

	result = cleanMainMenu(menuData);
	if(result != 0) {
		printf("Could not clean main menu completely\n");
		retvalue = -1;
	}
	result = cleanFont();
	if(result != 0) {
		printf("Could not clean font completely\n");
		retvalue = -1;
	}
	result = cleanSound(stateInfo);
	if(result != 0) {
		printf("Could not clean sound completely\n");
		retvalue = -1;
	}
	resource_manager_shutdown(rm);
	glfwTerminate();
	return retvalue;
}
