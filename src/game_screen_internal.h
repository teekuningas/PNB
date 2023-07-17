#ifndef GAME_SCREEN_INTERNAL
#define GAME_SCREEN_INTERNAL

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

static Vector3D cam, look, up;
static Vector3D statCam, statLook, statUp;
static Vector3D skyBoxCam, skyBoxLook;
float lightPos[4];

// camera parameters
static Vector3D lastCamTargetLocation;
static Vector3D lastCamLocation;
static Vector3D camLocation;
static Vector3D camTargetLocation;

float lastMeterX;
float lastSwingMeterX;

extern StateInfo stateInfo;

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

#endif /* GAME_SCREEN_INTERNAL */
