#ifndef PLAYER_INTERNAL_H
#define PLAYER_INTERNAL_H

#define PLAYER_SCALE 0.24f
#define SELECTION_BALL_SCALE 0.2f

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

extern StateInfo stateInfo;

static void textureSelection(int team, int joker, int type);
static void modelSelection(int index);

#endif /* PLAYER_INTERNAL_H */
