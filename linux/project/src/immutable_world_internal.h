#ifndef IMMUTABLE_WORLD_INTERNAL
#define IMMUTABLE_WORLD_INTERNAL

#if defined(__wii__)

#include "osa1_tpl.h"
#define groundPart1 0
#include "osa2_tpl.h"
#define groundPart2 0
#include "osa3_tpl.h"
#define groundPart3 0
#include "osa4_tpl.h"
#define groundPart4 0
#include "osa5_tpl.h"
#define groundPart5 0
#include "osa6_tpl.h"
#define groundPart6 0
#include "osa7_tpl.h"
#define groundPart7 0
#include "osa8_tpl.h"
#define groundPart8 0
#include "osa9_tpl.h"
#define groundPart9 0
#include "osa10_tpl.h"
#define groundPart10 0
#include "osa11_tpl.h"
#define groundPart11 0
#include "osa12_tpl.h"
#define groundPart12 0
#include "grassTexture_tpl.h"
#define grassTexture 0

#include "fence_tpl.h"
#define fence 0
#include "plate_tpl.h"
#define plate 0

#endif

#define GROUND_UNIT_COUNT 30
#define FENCE_HEIGHT 3.0f
#define FENCE_PIECE_WIDTH 4.0f
#define RUNNER_BASE_OFFSET 0.8f

typedef struct _GroundUnit
{
	#if defined(__wii__)
	GXTexObj texture;
	#else
	GLuint texture;
	#endif
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

#if defined(__wii__)
extern Mtx view;
static Mtx model, modelview, mvi, rot;
static Mtx fenceModel;
static Mtx groundModel;

static MeshObject* planeMesh;
static void *planeDisplayList;
static u32 planeListSize;

static MeshObject* plateMesh;
static void *plateDisplayList;
static u32 plateListSize;

static TPLFile plateTPL;
static GXTexObj plateTexture;

static GXTexObj fenceTexture;
static TPLFile fenceTPL;

static TPLFile groundTPL;

// to rotate fence from the horizontal ground mesh
static guVector rotYAxis = {0,1,0};
static guVector rotXAxis = {1,0,0};
#else

static GLuint plateTexture;
static GLuint fenceTexture;

static MeshObject* plateMesh;
static GLuint plateDisplayList;

static MeshObject* planeMesh;
static GLuint planeDisplayList;

#endif

static GroundUnit groundUnit[GROUND_UNIT_COUNT];

// we'll get the field positions for players etc relative to our real rendered field
static FieldPositions fieldPositions;

extern StateInfo stateInfo;

#endif /* IMMUTABLE_WORLD_INTERNAL */