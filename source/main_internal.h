#ifndef MAIN_INTERNAL
#define MAIN_INTERNAL

#define PERSPECTIVE_ASPECT_RATIO (16.0f/9.0f)

#if defined(__wii__)
// some static variables needed to initialize gx
	static int initGX();
	static void *frameBuffer[2] = { NULL, NULL};
	static GXRModeObj *rmode;
	static u32	fb = 0; 	// initial framebuffer index
	static Mtx mt;
	static Mtx44 perspective;

	static DIR *pdir;

#else
	static int initGL();
	int fullscreen;
#endif

static int clean();
static void draw(double alpha);
static int update();

StateInfo stateInfo;

static TeamData teamData[TEAM_COUNT]; // 4 teams

#endif /* MAIN_INTERNAL */