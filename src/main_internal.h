#ifndef MAIN_INTERNAL
#define MAIN_INTERNAL

#define PERSPECTIVE_ASPECT_RATIO (16.0f/9.0f)

int fullscreen;

static int initGL();
static int clean();
static void draw(double alpha);
static int update();

StateInfo stateInfo;

static TeamData teamData[TEAM_COUNT]; // 4 teams

#endif /* MAIN_INTERNAL */