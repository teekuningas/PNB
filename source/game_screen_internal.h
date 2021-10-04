#ifndef GAME_SCREEN_INTERNAL
#define GAME_SCREEN_INTERNAL

#if defined(__wii__)

#include "sky_tpl.h"
#define sky 0

#include "lineTexture_tpl.h"
#define lineTexture 0	
#include "lineTexture2_tpl.h"
#define lineTexture2 0	

#include "bases_tpl.h"
#define bases 0

#include "basesMarker_tpl.h"
#define basesMarker 0

#include "meter_tpl.h"
#define meter 0

#endif

#define LIGHT_SOURCE_POSITION_X 30.0f
#define LIGHT_SOURCE_POSITION_Y 50.0f
#define LIGHT_SOURCE_POSITION_Z -50.0f
#define STATISTICS_TEXT_HEIGHT -1.34f
#define OTHER_STATS_X -0.03f
#define METER_X 0.31f
#define OUTS_X -0.63f
#define INFO_X -0.53f
#define BASES_X 0.57f
#define EVENT_TIMER_THRESHOLD (1.5 * (1 / (UPDATE_INTERVAL*1.0f/1000)))

static int gameInfoEventTimer;
static int gameInfoEvent;

static void drawSkyBox();
static void drawStatistics(double alpha);

static int initLights();
static void initCamSettings();
static void loadGameScreenSettings();

#if defined(__wii__)
Mtx view;
guVector lightPos;
guVector light;
GXLightObj lo;
static Mtx model, modelview, mvi, rot;
static guVector rotYAxis = {0,1,0};
static guVector cam, look, up;
static guVector statCam, statLook, statUp;
static guVector skyBoxCam, skyBoxLook;
#else

static Vector3D cam, look, up;
static Vector3D statCam, statLook, statUp;
static Vector3D skyBoxCam, skyBoxLook;
float lightPos[4];

#endif
// camera parameters
static Vector3D lastCamTargetLocation;
static Vector3D lastCamLocation;
static Vector3D camLocation;
static Vector3D camTargetLocation;

float lastMeterX;
float lastSwingMeterX;

extern StateInfo stateInfo;

#if defined(__wii__)

static GXTexObj skyTexture;
static TPLFile skyTPL;
static GXTexObj meterTexture;
static TPLFile meterTPL;
static GXTexObj selectionTextureBatting;
static TPLFile selectionBattingTPL;
static GXTexObj selectionTextureFielding;
static TPLFile selectionFieldingTPL;
static GXTexObj basesTexture;
static TPLFile basesTPL;
static GXTexObj basesMarkerTexture;
static TPLFile basesMarkerTPL;

static MeshObject* skyMesh;
static void *skyDisplayList;
static u32 skyListSize;	

static MeshObject* planeMesh;
static void *planeDisplayList;
static u32 planeListSize;	
#else
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
#endif

#endif /* GAME_SCREEN_INTERNAL */