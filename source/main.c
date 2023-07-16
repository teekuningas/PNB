#include "globals.h"
#include "game_screen.h"
#include "input.h"
#include "sound.h"
#include "font.h"
#include "fill_player_data.h"
#include "main_menu.h"

#include "main_internal.h"


/*
	the core part of the code. initializes graphics and everything in our program and also includes our main loop that keeps the game in sync and
	is the code that decides should we call menu's or game screen's functions.
*/

int main ( int argc, char *argv[] )
{
	double alpha;
	int done = 0;
	int result;

	#if defined(__wii__)
	unsigned long long initTime = ticks_to_millisecs(gettime());
	#endif
	unsigned int currentTime = 0;
	unsigned int newTime;
	unsigned int frameTime;
    unsigned int accumulator = 0;
	unsigned int updateInterval = UPDATE_INTERVAL;
	#if defined(__wii__)
	#else
	if(argc >= 2)
	{
		fullscreen = 0;
	}
	else
	{
		fullscreen = 1;
	}
	#endif

	printf("v. 1.0.1\n");

	#if defined(__wii__)
		result = initGX();
		if(result != 0)
		{
			printf("Could not init GX. Exiting.");
			return -1;
		}

	#else
		result = initGL();
		if(result != 0)
		{
			printf("Could not init GL. Exiting.");
			return -1;
		}
	#endif

	result = fillPlayerData(&(teamData[0]));
	if(result != 0)
	{
		printf("Could not init team data. Exiting.");
		return -1;
	}
	stateInfo.teamData = &(teamData[0]);

	srand ( (unsigned int)time(NULL) );

	result = initMainMenu();
	if(result != 0)
	{
		printf("Could not init main menu. Exiting.");
		return -1;
	}

	result = initInput();
	if(result != 0)
	{
		printf("Could not init input. Exiting.");
		return -1;
	}
	result = initSound();
	if(result != 0)
	{
		printf("Could not init sound system. Exiting.");
		return -1;
	}
	result = initFont();
	if(result != 0)
	{
		printf("Could not init font. Exiting.");
		return -1;
	}

	// draw loading screen before loading all the player meshes which will take time
	stateInfo.screen = -1;
	// we draw twice as at least my debian's graphics are drawn wrong sometimes at the first time.
	drawLoadingScreen();
	draw(1.0);
	drawLoadingScreen();
	draw(1.0);

	result = initGameScreen();
	if(result != 0)
	{
		printf("Could not init game screen. Exiting.");
		return -1;
	}

	stateInfo.screen = 0;
	stateInfo.changeScreen = 1;
	stateInfo.updated = 0;

	// to keep our fps steady. we are trying to draw as often as we can and update in fixed intervals.
	while(done == 0)
	{
		#if defined(__wii__)
		newTime = (unsigned int)(ticks_to_millisecs(gettime()) - initTime);
		#else
		newTime = (unsigned int)(1000*glfwGetTime());
		#endif
		frameTime = newTime - currentTime;
		currentTime = newTime;
		accumulator += frameTime;
		// update the scene every 20ms and if for some reason there is delay, keep updating until catched up
        while ( accumulator >= updateInterval )
        {
			result = update();
			#if defined(__wii__)
			if (result != 0) done = 1;
			#else
			if (result != 0 || !glfwGetWindowParam( GLFW_OPENED )) done = 1;
			#endif
            accumulator -= updateInterval;
        }

		alpha = (double)accumulator / updateInterval;
		// draw the scene, alpha will give us nice little smoothing effect.
		// like if we are in the middle of updateInterval, the "real" position of the object
		// isn't what it was on laste update call nor it is what it will be in the next call to update.
		// so we will draw it to the middle.
		if(stateInfo.updated == 1)
		{
			draw(alpha);
		}


	}
	// and we will clean up when everything ends
	result = clean();
	if(result != 0)
	{
		printf("Cleaning up unsuccessful. Exiting anyway.");
		return -1;
	}
	return 0;

}

static int update()
{
	updateInput();
	updateSound();
	switch(stateInfo.screen) {
		case GAME_SCREEN:
			updateGameScreen();
			break;
		case MAIN_MENU:
			updateMainMenu();
			break;
		default:
			return 1;
	}
	return 0;
}


static void draw(double alpha)
{
	switch(stateInfo.screen) {
		case GAME_SCREEN:
			drawGameScreen(alpha);
			break;
		case MAIN_MENU:
			drawMainMenu(alpha);
			break;
	}
	#if defined(__wii__)
	GX_DrawDone();
	// flip framebuffer and get ready for the next one
	fb ^= 1;
	GX_CopyDisp(frameBuffer[fb],GX_TRUE);
	VIDEO_SetNextFramebuffer(frameBuffer[fb]);
	VIDEO_Flush();
	#else
	glfwSwapBuffers();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	#endif
}

#if defined(__wii__)
static int initGX()
{
	// initialization code for GX of libogc
	f32 yscale;
	u32 xfbHeight;

	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);

	// allocate 2 framebuffers for double buffering
	frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(frameBuffer[fb]);
	VIDEO_SetBlack(0);
	VIDEO_Flush();

	// setup the fifo and then init the flipper
	void *gp_fifo = NULL;
	gp_fifo = memalign(32,DEFAULT_FIFO_SIZE);
	memset(gp_fifo,0,DEFAULT_FIFO_SIZE);
	GX_Init(gp_fifo,DEFAULT_FIFO_SIZE);

	// clears the bg to color and clears the z buffer
	GXColor background = {0xa0, 0xff, 0xff, 0xff};
	GX_SetCopyClear(background, 0x00ffffff);

	// other gx setup
	GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
	yscale = GX_GetYScaleFactor(rmode->efbHeight,rmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopySrc(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopyDst(rmode->fbWidth,xfbHeight);
	GX_SetCopyFilter(rmode->aa,rmode->sample_pattern,GX_TRUE,rmode->vfilter);
	GX_SetFieldMode(rmode->field_rendering,((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));
	GX_SetColorUpdate(GX_TRUE);

	if (rmode->aa) {
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	}
	else
	{
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
	}

	GX_CopyDisp(frameBuffer[fb],GX_TRUE);
	GX_SetDispCopyGamma(GX_GM_1_0);

	GX_SetNumChans(1);
	GX_SetNumTexGens(1);
	GX_SetNumTevStages(1);
	// basic texture unit settings
	GX_SetTevOp(GX_TEVSTAGE0,GX_MODULATE);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

	guMtxIdentity(mt);
	guMtxScaleApply(mt, mt, 1.0f, -1.0f, 1.0f);
	GX_LoadTexMtxImm(mt, GX_TEXMTX0, GX_MTX2x4);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);

	GX_SetCullMode(GX_CULL_FRONT);

	// set the format for meshes
	GX_ClearVtxDesc();

	GX_SetVtxDesc(GX_VA_POS, GX_INDEX16);
	GX_SetVtxDesc(GX_VA_NRM, GX_INDEX16);
	GX_SetVtxDesc(GX_VA_CLR0, GX_INDEX16);
	GX_SetVtxDesc(GX_VA_TEX0, GX_INDEX16);

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	GX_InvVtxCache();
	GX_InvalidateTexAll();

	guPerspective(perspective, 45, PERSPECTIVE_ASPECT_RATIO, 0.1F, 250.0F);
	GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE);

	// init filesystem

	if (!fatInitDefault())
	{
		printf("fatInitDefault failure: terminating\n");
		return -1;
	}

	pdir=opendir("/");

	if (!pdir)
	{
	    printf ("opendir() failure; terminating\n");
		return -1;
	}

	return 0;
}
#else
static int initGL()
{
	GLFWvidmode mode;
	int width;
	int height;
	// First we'll just initialize GLFW so that we get a nice window.
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		printf("init");
		return -1;
	}
	glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 4); // 4x antialiasing
	glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, GL_TRUE);
	// Open a window and create its OpenGL context
	if(fullscreen == 0)
	{
		glfwGetDesktopMode(&mode);
		width = (int)(mode.Width * (3.0/4));
		height = (int)(mode.Height * (3.0/4));
		if( !glfwOpenWindow( 0, 0, 0,0,0,0, 32,0, GLFW_WINDOW ) )
		{
			fprintf( stderr, "Failed to open GLFW window\n" );
			printf("window");
			glfwTerminate();
			return -1;
		}
		glfwSetWindowTitle( "PNB" );
		glfwSetWindowSize(width, height);
		glfwSetWindowPos( (mode.Width - width) / 2, 0);

	}
	else
	{
		glfwGetDesktopMode(&mode);
		width = mode.Width;
		height = mode.Height;
		if( !glfwOpenWindow( width, height, 0,0,0,0, 32,0, GLFW_FULLSCREEN ) )
		{
			fprintf( stderr, "Failed to open GLFW window\n" );
			printf("window");
			glfwTerminate();
			return -1;
		}
	}

	// Initialize GLEW
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		printf("glew");
		return -1;
	}

	glfwSwapInterval(0);
	// and then initialize openGL settings. nothing really weird here.
	glEnable( GL_TEXTURE_2D );
	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
	glViewport(0, 0, width, height);						// Reset The Current Viewport
	// this blendfunc works like that it doesnt draw anything with color data from shadow mesh, but it will
	// use this alpha value to reduce intensity of the background of the mesh.
	glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
	glColor4f(1.0f, 1.0f, 1.0f, 0.5f);

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix
	gluPerspective(45.0f,PERSPECTIVE_ASPECT_RATIO,0.1f,250.0f);
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	return 0;
}
#endif

static int clean()
{
	int result;
	result = cleanPlayerData();
	if(result != 0)
	{
		printf("Could not clean player data completely\n");
		return -1;
	}
	result = cleanGameScreen();
	if(result != 0)
	{
		printf("Could not clean game screen completely\n");
		return -1;
	}

	result = cleanMainMenu();
	if(result != 0)
	{
		printf("Could not clean main menu completely\n");
		return -1;
	}
	result = cleanFont();
	if(result != 0)
	{
		printf("Could not clean font completely\n");
		return -1;
	}
	result = cleanSound();
	if(result != 0)
	{
		printf("Could not clean sound completely\n");
		return -1;
	}
	#if defined(__wii__)
	closedir(pdir);
	#else
	glfwTerminate();
	#endif
	return 0;
}
