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
#include "state_validator.h"

static int initGL(GLFWwindow** window, int fullscreen, RenderState* renderState);
static int clean(StateInfo* stateInfo, MenuData* menuData, ResourceManager* rm);
static void
draw(StateInfo* stateInfo, MenuData* menuData, GLFWwindow* window, double alpha, ResourceManager* rm, RenderState* rs);
static int update(StateInfo* stateInfo, MenuData* menuData, GLFWwindow* window, unsigned int* rng_seed);
static void applyFixture(
    const FixtureRequest* request, StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, unsigned int* rng_seed
);

static MenuData menuData;
static StateInfo stateInfo;
static MatchSession match;
static ClientInputState clientInput;
static AIControllerState aiController;
static GameRulesState rules;
static GameConclusion gameConclusion;
static MenuInfo menuInfo;
static KeyStates keyStates;
static FieldPositions fieldPositions;
static RenderState renderState;
static ResourceManager* resourceManager;

int main(int argc, char* argv[])
{

    double alpha;
    int done = 0;
    int fullscreen = 1;
    int soundEnabled = 1;
    int result;

    unsigned int currentTime = 0;
    unsigned int newTime;
    unsigned int frameTime;
    unsigned int accumulator = 0;
    unsigned int updateInterval = UPDATE_INTERVAL;

    // The app-level random stream: menus, hutunkeitto and cup simulation draw from it directly,
    // and starting a match splits two independent children off it (initialize_game_from_menu) —
    // one for the engine, which lives in MatchSession as World state, and one for the AI
    // controller, which lives in AIControllerState. Everything routes through seeded_rand() in
    // rng.c; there are no bare rand() consumers, so the global C RNG is intentionally left
    // unseeded and the whole app stays reproducible from this one seed (see the sim test tier).
    unsigned int rng_seed = (unsigned int)time(NULL);

    // Parse command-line arguments
    FixtureRequest fixtureRequest;
    fixture_parse_args(argc, argv, &fixtureRequest);

    // State debugging is ON by default: one 5 KB memcpy per pitch and a 0.5 MB history ring. This
    // default is app-level only — the test tiers set their own (the sim harness inits with no dump
    // path), so they are unaffected either way.
    const char* debugStatePath = "debug.log";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--windowed") == 0) {
            fullscreen = 0;
        }
        if (strcmp(argv[i], "--no-sound") == 0) {
            soundEnabled = 0;
        }
        if (strcmp(argv[i], "--no-debug-state") == 0) {
            debugStatePath = NULL;
        }
        if (strcmp(argv[i], "--debug-state") == 0 && i + 1 < argc) {
            debugStatePath = argv[i + 1];
            i++;
        }
    }
    state_validator_init(debugStatePath);

    printf("v. 1.5 beta\n");

    // Initialize stateInfo structure
    stateInfo.match = &match;
    stateInfo.clientInput = &clientInput;
    stateInfo.aiController = &aiController;
    stateInfo.rules = &rules;
    stateInfo.gameConclusion = &gameConclusion;
    stateInfo.keyStates = &keyStates;
    stateInfo.fieldPositions = &fieldPositions;
    stateInfo.teamData = NULL;
    stateInfo.playSoundEffect = 0;
    stateInfo.stopSoundEffect = 0;
    stateInfo.soundEnabled = soundEnabled;
    stateInfo.cup = NULL;
    stateInfo.currently_played_cup_match_index = -1;

    resourceManager = resource_manager_init();
    if (resourceManager == NULL) {
        printf("Could not init resource manager. Exiting.");
        return -1;
    }

    GLFWwindow* window = NULL;
    result = initGL(&window, fullscreen, &renderState);
    if (result != 0) {
        printf("Could not init GL. Exiting.");
        return -1;
    }

    result = fill_player_data(&stateInfo, "data/teams.xml");
    if (result != 0) {
        printf("Could not init team data. Exiting.");
        return -1;
    }

    result = init_main_menu(&stateInfo, &menuData, &menuInfo, resourceManager, &renderState);
    if (result != 0) {
        printf("Could not init main menu. Exiting.");
        return -1;
    }

    result = init_input(&stateInfo);
    if (result != 0) {
        printf("Could not init input. Exiting.");
        return -1;
    }
    result = init_sound(&stateInfo);
    if (result != 0) {
        printf("Could not init sound system. Exiting.");
        return -1;
    }
    result = init_font();
    if (result != 0) {
        printf("Could not init font. Exiting.");
        return -1;
    }

    // draw loading screen before loading all the player meshes which will take time
    stateInfo.screen = SCREEN_LOADING;
    // we draw twice as at least my debian's graphics are drawn wrong sometimes at the first time.
    draw_loading_screen(&stateInfo, &menuData, &menuInfo, resourceManager, &renderState);
    draw(&stateInfo, &menuData, window, 1.0, resourceManager, &renderState);

    result = init_game_screen(&stateInfo, resourceManager);
    if (result != 0) {
        printf("Could not init game screen. Exiting.");
        return -1;
    }

    stateInfo.screen = SCREEN_MAIN_MENU;
    stateInfo.changeScreen = 1;
    stateInfo.updated = 0;

    // Apply fixture if requested (for visual testing)
    if (fixtureRequest.enabled) {
        applyFixture(&fixtureRequest, &stateInfo, &menuData, &menuInfo, &rng_seed);
    }

    // to keep our fps steady. we are trying to draw as often as we can and update in fixed intervals.
    while (done == 0) {
        newTime = (unsigned int)(1000 * glfwGetTime());
        frameTime = newTime - currentTime;
        currentTime = newTime;
        accumulator += frameTime;
        // update the scene every 20ms and if for some reason there is delay, keep updating until catched up
        while (accumulator >= updateInterval) {
            result = update(&stateInfo, &menuData, window, &rng_seed);
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
        if (stateInfo.updated == 1) {
            draw(&stateInfo, &menuData, window, alpha, resourceManager, &renderState);
        }

        glfwPollEvents();
    }
    // and we will clean up when everything ends
    result = clean(&stateInfo, &menuData, resourceManager);
    if (result != 0) {
        printf("Cleaning up unsuccessful. Exiting anyway.");
        return -1;
    }

    return 0;
}

static int update(StateInfo* stateInfo, MenuData* menuData, GLFWwindow* window, unsigned int* rng_seed)
{
    update_input(stateInfo, window);
    update_sound(stateInfo);
    switch (stateInfo->screen) {
    case SCREEN_GAME:
        update_game_screen(stateInfo, &menuInfo);
        break;
    case SCREEN_MAIN_MENU:
        update_main_menu(stateInfo, menuData, &menuInfo, &keyStates, rng_seed);
        break;
    default:
        return 1;
    }
    return 0;
}

static void
draw(StateInfo* stateInfo, MenuData* menuData, GLFWwindow* window, double alpha, ResourceManager* rm, RenderState* rs)

{
    switch (stateInfo->screen) {
    case SCREEN_GAME:
        // Everything within draw_game_screen is currently drawn in 3d context
        draw_game_screen(stateInfo, alpha, rm, rs);
        break;
    case SCREEN_MAIN_MENU:
        draw_main_menu(stateInfo, menuData, &menuInfo, alpha, rm, rs);
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
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    monitor = glfwGetPrimaryMonitor();
    mode = glfwGetVideoMode(monitor);

    if (fullscreen == 0) {
        width = (int)(mode->width * (3.0 / 4));
        height = (int)(mode->height * (3.0 / 4));

        *window = glfwCreateWindow(width, height, "PNB", NULL, NULL);
        if (!window) {
            fprintf(stderr, "Failed to open GLFW window\n");
            glfwTerminate();
            return -1;
        }
    } else {
        glfwWindowHint(GLFW_DECORATED, GL_FALSE);
        width = (int)(mode->width);
        height = (int)(mode->height);

        *window = glfwCreateWindow(width, height, "PNB", NULL, NULL);
        if (!*window) {
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
    gluPerspective(45.0f, PERSPECTIVE_ASPECT_RATIO, 0.1f, 250.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return 0;
}

static int clean(StateInfo* stateInfo, MenuData* menuData, ResourceManager* rm)
{
    int result;
    int retvalue = 0;
    result = clean_player_data(stateInfo);
    if (result != 0) {
        printf("Could not clean player data completely\n");
        retvalue = -1;
    }
    result = clean_game_screen(stateInfo);
    if (result != 0) {
        printf("Could not clean game screen completely\n");
        retvalue = -1;
    }

    result = clean_main_menu(menuData);
    if (result != 0) {
        printf("Could not clean main menu completely\n");
        retvalue = -1;
    }
    result = clean_font();
    if (result != 0) {
        printf("Could not clean font completely\n");
        retvalue = -1;
    }
    result = clean_sound(stateInfo);
    if (result != 0) {
        printf("Could not clean sound completely\n");
        retvalue = -1;
    }
    resource_manager_shutdown(rm);
    glfwTerminate();
    return retvalue;
}

// Apply a fixture for visual testing
// This sets up a game at a specific period/state for rapid testing
static void applyFixture(
    const FixtureRequest* request, StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, unsigned int* rng_seed
)
{
    printf("Applying fixture: %s\n", request->name);
    GameSetup gameSetup;

    if (strcmp(request->name, "super-inning") == 0) {
        // Create super inning game setup
        fixture_create_super_inning(
            &gameSetup, request->team1, request->team2, request->team1_control, request->team2_control
        );
        initialize_game_from_menu(stateInfo, &gameSetup, rng_seed);

        // Set period state (super inning = period 2)
        stateInfo->rules->scoreboard.isCupGame = 0;
        stateInfo->rules->scoreboard.period = 2;
        // Inning counter: period 2 starts after period 0 and 1 complete
        // Each period uses halfInningsInPeriod half-innings
        // So period 2 starts at: 2 * halfInningsInPeriod
        stateInfo->rules->scoreboard.inning = stateInfo->rules->scoreboard.halfInningsInPeriod * 2;

        // Set prior period scores (for realistic display in game over screen)
        stateInfo->rules->scoreboard.teams[0].period0Runs = 0;
        stateInfo->rules->scoreboard.teams[1].period0Runs = 0;
        stateInfo->rules->scoreboard.teams[0].period1Runs = 0;
        stateInfo->rules->scoreboard.teams[1].period1Runs = 0;
        stateInfo->rules->scoreboard.teams[0].period2Runs = 0;
        stateInfo->rules->scoreboard.teams[1].period2Runs = 0;
        stateInfo->rules->scoreboard.teams[0].runs = 0;
        stateInfo->rules->scoreboard.teams[1].runs = 0;

    } else if (strcmp(request->name, "homerun-contest") == 0) {
        // Create homerun contest game setup
        fixture_create_homerun_contest(
            &gameSetup, request->team1, request->team2, request->team1_control, request->team2_control
        );
        initialize_game_from_menu(stateInfo, &gameSetup, rng_seed);

        // Set period state (homerun = period 4)
        stateInfo->rules->scoreboard.isCupGame = 0;
        stateInfo->rules->scoreboard.period = 4;
        // Inning counter: when super-inning ends, inning is at halfInningsInPeriod*2 + 2
        // For 8 half-innings: inning = 10 (even)
        // This makes team 0 bat first: (10 + 0 + 4) % 2 = 0
        stateInfo->rules->scoreboard.inning = stateInfo->rules->scoreboard.halfInningsInPeriod * 2 + 2;

        // Set prior period scores (for realistic display in game over screen)
        stateInfo->rules->scoreboard.teams[0].period0Runs = 0;
        stateInfo->rules->scoreboard.teams[1].period0Runs = 0;
        stateInfo->rules->scoreboard.teams[0].period1Runs = 0;
        stateInfo->rules->scoreboard.teams[1].period1Runs = 0;
        stateInfo->rules->scoreboard.teams[0].period2Runs = 0;
        stateInfo->rules->scoreboard.teams[1].period2Runs = 0;
        stateInfo->rules->scoreboard.teams[0].period3Runs = 0;
        stateInfo->rules->scoreboard.teams[1].period3Runs = 0;
        stateInfo->rules->scoreboard.teams[0].runs = 0;
        stateInfo->rules->scoreboard.teams[1].runs = 0;

    } else if (strcmp(request->name, "cup-final-super-inning") == 0) {
        // This fixture starts a playable super-inning in the final match of a cup.
        fixture_create_cup_final_super_inning(
            &gameSetup, request->team1, request->team2, request->team1_control, request->team2_control
        );
        initialize_game_from_menu(stateInfo, &gameSetup, rng_seed);

        // Set up the tournament context with a full, plausible history using the new API
        stateInfo->rules->scoreboard.isCupGame = 1;

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
        stateInfo->rules->scoreboard.period = 2;
        stateInfo->rules->scoreboard.inning = stateInfo->rules->scoreboard.halfInningsInPeriod * 2;

        // Set prior period scores to 0 for a clean super-inning
        stateInfo->rules->scoreboard.teams[0].period0Runs = 0;
        stateInfo->rules->scoreboard.teams[1].period0Runs = 0;
        stateInfo->rules->scoreboard.teams[0].period1Runs = 0;
        stateInfo->rules->scoreboard.teams[1].period1Runs = 0;
        stateInfo->rules->scoreboard.teams[0].period2Runs = 0;
        stateInfo->rules->scoreboard.teams[1].period2Runs = 0;
        stateInfo->rules->scoreboard.teams[0].runs = 0;
        stateInfo->rules->scoreboard.teams[1].runs = 0;

        // Jump directly to game screen
        stateInfo->screen = SCREEN_GAME;
        stateInfo->changeScreen = 1;

    } else {
        printf("Unknown fixture: %s\n", request->name);
        printf("Available fixtures: super-inning, homerun-contest, cup-final-super-inning\n");
        exit(-1);
    }

    // Jump directly to game screen
    stateInfo->screen = SCREEN_GAME;
    stateInfo->changeScreen = 1;
}
