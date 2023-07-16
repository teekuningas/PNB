#ifndef PLAYER_INTERNAL_H
#define PLAYER_INTERNAL_H

#if defined(__wii__)
#include "team1_tpl.h"
#define team1 0
#include "team2_tpl.h"
#define team2 0
#include "team3_tpl.h"
#define team3 0
#include "team4_tpl.h"
#define team4 0
#include "team5_tpl.h"
#define team5 0
#include "team6_tpl.h"
#define team6 0
#include "team7_tpl.h"
#define team7 0
#include "team8_tpl.h"
#define team8 0
#include "team1_joker_tpl.h"
#define team1Joker 0
#include "team2_joker_tpl.h"
#define team2Joker 0
#include "team3_joker_tpl.h"
#define team3Joker 0
#include "team4_joker_tpl.h"
#define team4Joker 0
#include "team5_joker_tpl.h"
#define team5Joker 0
#include "team6_joker_tpl.h"
#define team6Joker 0
#include "team7_joker_tpl.h"
#define team7Joker 0
#include "team8_joker_tpl.h"
#define team8Joker 0
#include "selectionBall1_tpl.h"
#define selectionBall1 0
#include "selectionBall2_tpl.h"
#define selectionBall2 0
#include "selectionBall3_tpl.h"
#define selectionBall3 0

#endif

#define PLAYER_SCALE 0.24f
#define SELECTION_BALL_SCALE 0.2f

#if defined(__wii__)

// vertex transformations
extern Mtx view;
static Mtx model, modelview, mvi, rot;

static guVector rotYAxis = {0,1,0};

static TPLFile team1TPL;
static GXTexObj team1Texture;
static TPLFile team2TPL;
static GXTexObj team2Texture;
static TPLFile team3TPL;
static GXTexObj team3Texture;
static TPLFile team4TPL;
static GXTexObj team4Texture;
static TPLFile team5TPL;
static GXTexObj team5Texture;
static TPLFile team6TPL;
static GXTexObj team6Texture;
static TPLFile team7TPL;
static GXTexObj team7Texture;
static TPLFile team8TPL;
static GXTexObj team8Texture;

static TPLFile team1JokerTPL;
static GXTexObj team1JokerTexture;
static TPLFile team2JokerTPL;
static GXTexObj team2JokerTexture;
static TPLFile team3JokerTPL;
static GXTexObj team3JokerTexture;
static TPLFile team4JokerTPL;
static GXTexObj team4JokerTexture;
static TPLFile team5JokerTPL;
static GXTexObj team5JokerTexture;
static TPLFile team6JokerTPL;
static GXTexObj team6JokerTexture;
static TPLFile team7JokerTPL;
static GXTexObj team7JokerTexture;
static TPLFile team8JokerTPL;
static GXTexObj team8JokerTexture;

static TPLFile selection1TPL;
static GXTexObj selection1Texture;
static TPLFile selection2TPL;
static GXTexObj selection2Texture;
static TPLFile selection3TPL;
static GXTexObj selection3Texture;

static MeshObject* playerBareHandsStandingMesh;
static void *playerBareHandsStandingDisplayList;
static u32 playerBareHandsStandingListSize;

static MeshObject* playerGloveWithBallStandingMesh;
static void *playerGloveWithBallStandingDisplayList;
static u32 playerGloveWithBallStandingListSize;

static MeshObject* playerGloveWithoutBallStandingMesh;
static void *playerGloveWithoutBallStandingDisplayList;
static u32 playerGloveWithoutBallStandingListSize;

static MeshObject* markerMesh;
static void *markerDisplayList;
static u32 markerListSize;

static MeshObject* shadowMesh;
static void *shadowDisplayList;
static u32 shadowListSize;
u8 *shadowColors;

static MeshObject* playerBareHandsWalkingMesh[16];
static void *playerBareHandsWalkingDisplayList[16];
static u32 playerBareHandsWalkingListSize[16];

static MeshObject* playerBareHandsRunningMesh[20];
static void *playerBareHandsRunningDisplayList[20];
static u32 playerBareHandsRunningListSize[20];

static MeshObject* playerGloveWithoutBallWalkingMesh[16];
static void *playerGloveWithoutBallWalkingDisplayList[16];
static u32 playerGloveWithoutBallWalkingListSize[16];

static MeshObject* playerGloveWithBallWalkingMesh[16];
static void *playerGloveWithBallWalkingDisplayList[16];
static u32 playerGloveWithBallWalkingListSize[16];

static MeshObject* playerGloveWithoutBallRunningMesh[20];
static void *playerGloveWithoutBallRunningDisplayList[20];
static u32 playerGloveWithoutBallRunningListSize[20];

static MeshObject* playerGloveWithBallRunningMesh[20];
static void *playerGloveWithBallRunningDisplayList[20];
static u32 playerGloveWithBallRunningListSize[20];

static MeshObject* pitchDownMesh[9];
static void *pitchDownDisplayList[9];
static u32 pitchDownListSize[9];

static MeshObject* pitchUpMesh[13];
static void *pitchUpDisplayList[13];
static u32 pitchUpListSize[13];

static MeshObject* throwLoadMesh[11];
static void *throwLoadDisplayList[11];
static u32 throwLoadListSize[11];

static MeshObject* throwReleaseMesh[21];
static void *throwReleaseDisplayList[21];
static u32 throwReleaseListSize[21];

static MeshObject* buntMesh[34];
static void *buntDisplayList[34];
static u32 buntListSize[34];

static MeshObject* swingMesh[34];
static void *swingDisplayList[34];
static u32 swingListSize[34];

static MeshObject* battingStopMesh[34];
static void *battingStopDisplayList[34];
static u32 battingStopListSize[34];


#else

static GLuint team1Texture;
static GLuint team2Texture;
static GLuint team3Texture;
static GLuint team4Texture;
static GLuint team5Texture;
static GLuint team6Texture;
static GLuint team7Texture;
static GLuint team8Texture;
static GLuint team1JokerTexture;
static GLuint team2JokerTexture;
static GLuint team3JokerTexture;
static GLuint team4JokerTexture;
static GLuint team5JokerTexture;
static GLuint team6JokerTexture;
static GLuint team7JokerTexture;
static GLuint team8JokerTexture;
static GLuint selection1Texture;
static GLuint selection2Texture;
static GLuint selection3Texture;

static MeshObject* playerBareHandsStandingMesh;
static GLuint playerBareHandsStandingDisplayList;

static MeshObject* playerGloveWithBallStandingMesh;
static GLuint playerGloveWithBallStandingDisplayList;

static MeshObject* playerGloveWithoutBallStandingMesh;
static GLuint playerGloveWithoutBallStandingDisplayList;

static MeshObject* markerMesh;
static GLuint markerDisplayList;

static MeshObject* playerBareHandsWalkingMesh[16];
static GLuint playerBareHandsWalkingDisplayList[16];

static MeshObject* playerBareHandsRunningMesh[20];
static GLuint playerBareHandsRunningDisplayList[20];

static MeshObject* playerGloveWithoutBallWalkingMesh[16];
static GLuint playerGloveWithoutBallWalkingDisplayList[16];

static MeshObject* playerGloveWithBallWalkingMesh[16];
static GLuint playerGloveWithBallWalkingDisplayList[16];

static MeshObject* playerGloveWithoutBallRunningMesh[20];
static GLuint playerGloveWithoutBallRunningDisplayList[20];

static MeshObject* playerGloveWithBallRunningMesh[20];
static GLuint playerGloveWithBallRunningDisplayList[20];

static MeshObject* pitchDownMesh[9];
static GLuint pitchDownDisplayList[9];

static MeshObject* pitchUpMesh[13];
static GLuint pitchUpDisplayList[13];

static MeshObject* throwLoadMesh[11];
static GLuint throwLoadDisplayList[11];

static MeshObject* throwReleaseMesh[21];
static GLuint throwReleaseDisplayList[21];

static MeshObject* swingMesh[34];
static GLuint swingDisplayList[34];

static MeshObject* buntMesh[34];
static GLuint buntDisplayList[34];

static MeshObject* battingStopMesh[34];
static GLuint battingStopDisplayList[34];

static MeshObject* shadowMesh;
static GLuint shadowDisplayList;

#endif
//state info
extern StateInfo stateInfo;

static void textureSelection(int team, int joker, int type);
static void modelSelection(int index);

#endif /* PLAYER_INTERNAL_H */