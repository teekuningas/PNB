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
#include "fixtures.h"
#include "common_logic.h"
#include "cup.h"

static int initGL(GLFWwindow** window, int fullscreen, RenderState* renderState);
static int clean(StateInfo* stateInfo, MenuData* menuData, ResourceManager* rm);
static void draw(StateInfo* stateInfo, MenuData* menuData, GLFWwindow* window, double alpha, ResourceManager* rm, RenderState* rs);
static int update(StateInfo* stateInfo, MenuData* menuData, GLFWwindow* window);
static void applyFixture(const FixtureRequest* request, StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo);

static MenuData menuData;
static StateInfo stateInfo;
static LocalGameInfo localGameInfo;
static GlobalGameInfo globalGameInfo;
static GameConclusion gameConclusion;
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

	// Parse command-line arguments
	FixtureRequest fixtureRequest;
	fixture_parse_args(argc, argv, &fixtureRequest);

	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "--windowed") == 0) {
			fullscreen = 0;
		}
	}

	printf("v. 1.5 beta\n");

	// Initialize stateInfo structure
	stateInfo.localGameInfo = &localGameInfo;
	stateInfo.globalGameInfo = &globalGameInfo;
	stateInfo.gameConclusion = &gameConclusion;
	stateInfo.keyStates = &keyStates;
	stateInfo.fieldPositions = &fieldPositions;
	stateInfo.teamData = NULL;
	stateInfo.stopSoundEffect = 0;
	stateInfo.cup = NULL;
	stateInfo.currently_played_cup_match_index = -1;

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
	stateInfo.screen = SCREEN_LOADING;
	// we draw twice as at least my debian's graphics are drawn wrong sometimes at the first time.
	drawLoadingScreen(&stateInfo, &menuData, &menuInfo, resourceManager, &renderState);
	draw(&stateInfo, &menuData, window, 1.0, resourceManager, &renderState);

	result = initGameScreen(&stateInfo);
	if(result != 0) {
		printf("Could not init game screen. Exiting.");
		return -1;
	}

	stateInfo.screen = SCREEN_MAIN_MENU;
	stateInfo.changeScreen = 1;
	stateInfo.updated = 0;

	// Apply fixture if requested (for visual testing)
	if (fixtureRequest.enabled) {
		applyFixture(&fixtureRequest, &stateInfo, &menuData, &menuInfo);
	}

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
	case SCREEN_GAME:
		updateGameScreen(stateInfo, &menuInfo);
		break;
	case SCREEN_MAIN_MENU:
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
	case SCREEN_GAME:
		// Everything within drawGameScreen is currently drawn in 3d context
		drawGameScreen(stateInfo, alpha, rs);
		break;
	case SCREEN_MAIN_MENU:
		drawMainMenu(stateInfo, menuData, &menuInfo, alpha, rm, rs);
		break;
	case SCREEN_LOADING:
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

// Apply a fixture for visual testing
// This sets up a game at a specific period/state for rapid testing
static void applyFixture(const FixtureRequest* request, StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo)
{
	printf("Applying fixture: %s\n", request->name);
	GameSetup gameSetup;

	if (strcmp(request->name, "super-inning") == 0) {
		// Create super inning game setup
		fixture_create_super_inning(&gameSetup,
		                            request->team1,
		                            request->team2,
		                            request->team1_control,
		                            request->team2_control);
		initializeGameFromMenu(stateInfo, &gameSetup);

		// Set period state (super inning = period 2)
		stateInfo->globalGameInfo->isCupGame = 0;
		stateInfo->globalGameInfo->period = 2;
		// Inning counter: period 2 starts after period 0 and 1 complete
		// Each period uses halfInningsInPeriod half-innings
		// So period 2 starts at: 2 * halfInningsInPeriod
		stateInfo->globalGameInfo->inning = stateInfo->globalGameInfo->halfInningsInPeriod * 2;

		// Set prior period scores (for realistic display in game over screen)
		stateInfo->globalGameInfo->teams[0].period0Runs = 0;
		stateInfo->globalGameInfo->teams[1].period0Runs = 0;
		stateInfo->globalGameInfo->teams[0].period1Runs = 0;
		stateInfo->globalGameInfo->teams[1].period1Runs = 0;
		stateInfo->globalGameInfo->teams[0].period2Runs = 0;
		stateInfo->globalGameInfo->teams[1].period2Runs = 0;
		stateInfo->globalGameInfo->teams[0].runs = 0;
		stateInfo->globalGameInfo->teams[1].runs = 0;

	} else if (strcmp(request->name, "homerun-contest") == 0) {
		// Create homerun contest game setup
		fixture_create_homerun_contest(&gameSetup,
		                               request->team1,
		                               request->team2,
		                               request->team1_control,
		                               request->team2_control);
		initializeGameFromMenu(stateInfo, &gameSetup);

		// Set period state (homerun = period 4)
		stateInfo->globalGameInfo->isCupGame = 0;
		stateInfo->globalGameInfo->period = 4;
		// Inning counter: when super-inning ends, inning is at halfInningsInPeriod*2 + 2
		// For 8 half-innings: inning = 10 (even)
		// This makes team 0 bat first: (10 + 0 + 4) % 2 = 0
		stateInfo->globalGameInfo->inning = stateInfo->globalGameInfo->halfInningsInPeriod * 2 + 2;

		// Set prior period scores (for realistic display in game over screen)
		stateInfo->globalGameInfo->teams[0].period0Runs = 0;
		stateInfo->globalGameInfo->teams[1].period0Runs = 0;
		stateInfo->globalGameInfo->teams[0].period1Runs = 0;
		stateInfo->globalGameInfo->teams[1].period1Runs = 0;
		stateInfo->globalGameInfo->teams[0].period2Runs = 0;
		stateInfo->globalGameInfo->teams[1].period2Runs = 0;
		stateInfo->globalGameInfo->teams[0].period3Runs = 0;
		stateInfo->globalGameInfo->teams[1].period3Runs = 0;
		stateInfo->globalGameInfo->teams[0].runs = 0;
		stateInfo->globalGameInfo->teams[1].runs = 0;

	} else if (strcmp(request->name, "cup-final-super-inning") == 0) {
		// This fixture starts a playable super-inning in the final match of a cup.
		fixture_create_cup_final_super_inning(&gameSetup,
		                                      request->team1,
		                                      request->team2,
		                                      request->team1_control,
		                                      request->team2_control);
		initializeGameFromMenu(stateInfo, &gameSetup);

		// Set up the tournament context with a full, plausible history using the new API
		stateInfo->globalGameInfo->isCupGame = 1;

		// Specific seeding for this test fixture: alternates top and bottom bracket
		// Creates matchups: (0v2), (4v6), (1v3), (5v7) in quarter-finals
		TeamID initial_teams[] = {0, 2, 4, 6, 1, 3, 5, 7};
		if (stateInfo->cup != NULL) {
			cup_destroy(stateInfo->cup);
		}
		stateInfo->cup = cup_create(8, 1, request->team1, 4, initial_teams);
		if (stateInfo->cup == NULL) {
			fprintf(stderr, "Error: Failed to create cup for fixture.\n");
			return;
		}

		// Simulate quarter-finals (winners: 0, 4, 1, 5)
		cup_update_match_result(stateInfo->cup, 3, 0); // Match 3 (0 vs 2) -> 0 wins
		cup_update_match_result(stateInfo->cup, 4, 4); // Match 4 (4 vs 6) -> 4 wins
		cup_update_match_result(stateInfo->cup, 5, 1); // Match 5 (1 vs 3) -> 1 wins
		cup_update_match_result(stateInfo->cup, 6, 5); // Match 6 (5 vs 7) -> 5 wins

		// Simulate semi-finals (winners: 0, 1)
		cup_update_match_result(stateInfo->cup, 1, 0); // Match 1 (0 vs 4) -> 0 wins
		cup_update_match_result(stateInfo->cup, 2, 1); // Match 2 (1 vs 5) -> 1 wins

		// The final (match 0) is now set up with teams 0 and 1.
		stateInfo->currently_played_cup_match_index = 0;

		// Set game state to a super-inning
		stateInfo->globalGameInfo->period = 2;
		stateInfo->globalGameInfo->inning = stateInfo->globalGameInfo->halfInningsInPeriod * 2;

		// Set prior period scores to 0 for a clean super-inning
		stateInfo->globalGameInfo->teams[0].period0Runs = 0;
		stateInfo->globalGameInfo->teams[1].period0Runs = 0;
		stateInfo->globalGameInfo->teams[0].period1Runs = 0;
		stateInfo->globalGameInfo->teams[1].period1Runs = 0;
		stateInfo->globalGameInfo->teams[0].period2Runs = 0;
		stateInfo->globalGameInfo->teams[1].period2Runs = 0;
		stateInfo->globalGameInfo->teams[0].runs = 0;
		stateInfo->globalGameInfo->teams[1].runs = 0;

		// Jump directly to game screen
		stateInfo->screen = SCREEN_GAME;
		stateInfo->changeScreen = 1;

	} else {
		printf("Unknown fixture: %s\n", request->name);
		printf("Available fixtures: super-inning, homerun-contest\n");
		exit(-1);
	}

	// Jump directly to game screen
	stateInfo->screen = SCREEN_GAME;
	stateInfo->changeScreen = 1;
}
