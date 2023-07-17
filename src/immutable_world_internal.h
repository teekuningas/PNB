#ifndef IMMUTABLE_WORLD_INTERNAL
#define IMMUTABLE_WORLD_INTERNAL

#define GROUND_UNIT_COUNT 30
#define FENCE_HEIGHT 3.0f
#define FENCE_PIECE_WIDTH 4.0f
#define RUNNER_BASE_OFFSET 0.8f

typedef struct _GroundUnit {
	GLuint texture;
	int x;
	int y;
} GroundUnit;

// functions used internally by this c-file
static int initGround();
static void drawGround();

static int initFence();
static void drawFence();

static int initPlate();
static void drawPlate();

static void initFieldPositions();

static GLuint plateTexture;
static GLuint fenceTexture;

static MeshObject* plateMesh;
static GLuint plateDisplayList;

static MeshObject* planeMesh;
static GLuint planeDisplayList;

static GroundUnit groundUnit[GROUND_UNIT_COUNT];

// we'll get the field positions for players etc relative to our real rendered field
static FieldPositions fieldPositions;

extern StateInfo stateInfo;

#endif /* IMMUTABLE_WORLD_INTERNAL */
