/*
	here we are just gonna render our players. draw function is the key part, it will take all the information about players and use that to draw
	them. it means that all the data about how the players look like in the scene will be in the playerInfo-structure.
*/

#include "globals.h"
#include "render.h"
#include "player.h"

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


int initPlayer()
{
	int i;

	char bare_hands_walking_string[75] = "data/models/player_bare_hands_walking/player_bare_hands_walking_000001.obj";
	for(i = 0; i < 16; i++) {
		bare_hands_walking_string[69] = (char)(((int)'0')+(i+1)%10);
		bare_hands_walking_string[68] = (char)(((int)'0')+((i+1)/10));
		playerBareHandsWalkingMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(bare_hands_walking_string, "Sphere", playerBareHandsWalkingMesh[i], &(playerBareHandsWalkingDisplayList[i])) != 0) return -1;
	}

	char bare_hands_running_string[75] = "data/models/player_bare_hands_running/player_bare_hands_running_000001.obj";
	for(i = 0; i < 20; i++) {
		bare_hands_running_string[69] = (char)(((int)'0')+(i+1)%10);
		bare_hands_running_string[68] = (char)(((int)'0')+((i+1)/10));
		playerBareHandsRunningMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(bare_hands_running_string, "Sphere", playerBareHandsRunningMesh[i], &playerBareHandsRunningDisplayList[i]) != 0) return -1;
	}

	char without_ball_walking_string[91] = "data/models/player_glove_without_ball_walking/player_glove_without_ball_walking_000001.obj";
	for(i = 0; i < 16; i++) {
		without_ball_walking_string[85] = (char)(((int)'0')+(i+1)%10);
		without_ball_walking_string[84] = (char)(((int)'0')+((i+1)/10));
		playerGloveWithoutBallWalkingMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(without_ball_walking_string, "Cube", playerGloveWithoutBallWalkingMesh[i], &playerGloveWithoutBallWalkingDisplayList[i]) != 0) return -1;
	}

	char with_ball_walking_string[85] = "data/models/player_glove_with_ball_walking/player_glove_with_ball_walking_000001.obj";
	for(i = 0; i < 16; i++) {
		with_ball_walking_string[79] = (char)(((int)'0')+(i+1)%10);
		with_ball_walking_string[78] = (char)(((int)'0')+((i+1)/10));
		playerGloveWithBallWalkingMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(with_ball_walking_string, "Cube", playerGloveWithBallWalkingMesh[i], &playerGloveWithBallWalkingDisplayList[i]) != 0) return -1;
	}

	char without_ball_running_string[91] = "data/models/player_glove_without_ball_running/player_glove_without_ball_running_000001.obj";
	for(i = 0; i < 20; i++) {
		without_ball_running_string[85] = (char)(((int)'0')+(i+1)%10);
		without_ball_running_string[84] = (char)(((int)'0')+((i+1)/10));
		playerGloveWithoutBallRunningMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(without_ball_running_string, "Cube", playerGloveWithoutBallRunningMesh[i], &playerGloveWithoutBallRunningDisplayList[i]) != 0) return -1;
	}

	char with_ball_running_string[85] = "data/models/player_glove_with_ball_running/player_glove_with_ball_running_000001.obj";
	for(i = 0; i < 20; i++) {
		with_ball_running_string[79] = (char)(((int)'0')+(i+1)%10);
		with_ball_running_string[78] = (char)(((int)'0')+((i+1)/10));
		playerGloveWithBallRunningMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(with_ball_running_string, "Cube", playerGloveWithBallRunningMesh[i], &playerGloveWithBallRunningDisplayList[i]) != 0) return -1;
	}

	char pitch_down_string[40] = "data/models/pitch/pitch_down_000001.obj";
	for(i = 0; i < 9; i++) {

		pitch_down_string[34] = (char)(((int)'0')+(i+1)%10);
		pitch_down_string[33] = (char)(((int)'0')+((i+1)/10));
		pitchDownMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(pitch_down_string, "Cube", pitchDownMesh[i], &pitchDownDisplayList[i]) != 0) return -1;
	}

	char pitch_up_string[38] = "data/models/pitch/pitch_up_000001.obj";
	for(i = 0; i < 13; i++) {
		pitch_up_string[32] = (char)(((int)'0')+(i+1)%10);
		pitch_up_string[31] = (char)(((int)'0')+((i+1)/10));
		pitchUpMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(pitch_up_string, "Cube", pitchUpMesh[i], &pitchUpDisplayList[i]) != 0) return -1;
	}

	char throw_load_string[40] = "data/models/throw/throw_load_000001.obj";
	for(i = 0; i < 11; i++) {
		throw_load_string[34] = (char)(((int)'0')+(i+1)%10);
		throw_load_string[33] = (char)(((int)'0')+((i+1)/10));
		throwLoadMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(throw_load_string, "Cube", throwLoadMesh[i], &throwLoadDisplayList[i]) != 0) return -1;
	}

	char throw_release_string[43] = "data/models/throw/throw_release_000001.obj";
	for(i = 0; i < 21; i++) {
		throw_release_string[37] = (char)(((int)'0')+(i+1)%10);
		throw_release_string[36] = (char)(((int)'0')+((i+1)/10));
		throwReleaseMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(throw_release_string, "Cube", throwReleaseMesh[i], &throwReleaseDisplayList[i]) != 0) return -1;
	}

	char swing_string[37] = "data/models/batting/swing_000001.obj";
	for(i = 0; i < 34; i++) {
		swing_string[31] = (char)(((int)'0')+(i+1)%10);
		swing_string[30] = (char)(((int)'0')+((i+1)/10));
		swingMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(swing_string, "Sphere", swingMesh[i], &swingDisplayList[i]) != 0) return -1;
	}

	char bunt_string[36] = "data/models/batting/bunt_000001.obj";
	for(i = 0; i < 34; i++) {
		bunt_string[30] = (char)(((int)'0')+(i+1)%10);
		bunt_string[29] = (char)(((int)'0')+((i+1)/10));
		buntMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(bunt_string, "Sphere", buntMesh[i], &buntDisplayList[i]) != 0) return -1;
	}

	char batting_stop_string[44] = "data/models/batting/batting_stop_000001.obj";
	for(i = 0; i < 34; i++) {
		batting_stop_string[38] = (char)(((int)'0')+(i+1)%10);
		batting_stop_string[37] = (char)(((int)'0')+((i+1)/10));
		battingStopMesh[i] = (MeshObject *)malloc ( sizeof(MeshObject));
		if(tryPreparingMeshGL(batting_stop_string, "Sphere", battingStopMesh[i], &battingStopDisplayList[i]) != 0) return -1;
	}

	if(tryLoadingTextureGL(&team1Texture, "data/textures/team1.tga", "team1") != 0) return -1;
	if(tryLoadingTextureGL(&team2Texture, "data/textures/team2.tga", "team2") != 0) return -1;
	if(tryLoadingTextureGL(&team3Texture, "data/textures/team3.tga", "team3") != 0) return -1;
	if(tryLoadingTextureGL(&team4Texture, "data/textures/team4.tga", "team4") != 0) return -1;
	if(tryLoadingTextureGL(&team5Texture, "data/textures/team5.tga", "team5") != 0) return -1;
	if(tryLoadingTextureGL(&team6Texture, "data/textures/team6.tga", "team6") != 0) return -1;
	if(tryLoadingTextureGL(&team7Texture, "data/textures/team7.tga", "team7") != 0) return -1;
	if(tryLoadingTextureGL(&team8Texture, "data/textures/team8.tga", "team8") != 0) return -1;
	if(tryLoadingTextureGL(&team1JokerTexture, "data/textures/team1_joker.tga", "team1Joker") != 0) return -1;
	if(tryLoadingTextureGL(&team2JokerTexture, "data/textures/team2_joker.tga", "team2Joker") != 0) return -1;
	if(tryLoadingTextureGL(&team3JokerTexture, "data/textures/team3_joker.tga", "team3Joker") != 0) return -1;
	if(tryLoadingTextureGL(&team4JokerTexture, "data/textures/team4_joker.tga", "team4Joker") != 0) return -1;
	if(tryLoadingTextureGL(&team5JokerTexture, "data/textures/team5_joker.tga", "team5Joker") != 0) return -1;
	if(tryLoadingTextureGL(&team6JokerTexture, "data/textures/team6_joker.tga", "team6Joker") != 0) return -1;
	if(tryLoadingTextureGL(&team7JokerTexture, "data/textures/team7_joker.tga", "team7Joker") != 0) return -1;
	if(tryLoadingTextureGL(&team8JokerTexture, "data/textures/team8_joker.tga", "team8Joker") != 0) return -1;
	if(tryLoadingTextureGL(&selection1Texture, "data/textures/selectionBall1.tga", "selection1") != 0) return -1;
	if(tryLoadingTextureGL(&selection2Texture, "data/textures/selectionBall2.tga", "selection2") != 0) return -1;
	if(tryLoadingTextureGL(&selection3Texture, "data/textures/selectionBall3.tga", "selection3") != 0) return -1;
	playerBareHandsStandingMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/player_bare_hands_standing.obj", "Sphere", playerBareHandsStandingMesh, &playerBareHandsStandingDisplayList) != 0) return -1;
	playerGloveWithBallStandingMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/player_glove_with_ball_standing.obj", "Cube", playerGloveWithBallStandingMesh, &playerGloveWithBallStandingDisplayList) != 0) return -1;
	playerGloveWithoutBallStandingMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/player_glove_without_ball_standing.obj", "Cube", playerGloveWithoutBallStandingMesh, &playerGloveWithoutBallStandingDisplayList) != 0) return -1;
	markerMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/pallo.obj", "Icosphere", markerMesh, &markerDisplayList) != 0) return -1;
	shadowMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/shadow.obj", "Circle", shadowMesh, &shadowDisplayList) != 0) return -1;

	return 0;
}

void drawPlayer(double alpha, PlayerInfo *playerInfo)
{
	int i;
	int j = 0;
	double angle;
	// so we draw every players
	for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT + PLAYERS_IN_TEAM; i++) {
		j++;
		// calculate the angle in which player is facing
		angle = atan2(-playerInfo[i].tPI.orientation.z, playerInfo[i].tPI.orientation.x) * 180.0f / PI;
		// for every player we also move and rotate the players to right places
		glPushMatrix();
		glTranslatef((float)(alpha*playerInfo[i].tPI.location.x + (1-alpha)*playerInfo[i].tPI.lastLocation.x),
		             (float)playerInfo[i].tPI.location.y, (float)(alpha*playerInfo[i].tPI.location.z + (1-alpha)*playerInfo[i].tPI.lastLocation.z));
		glScalef(PLAYER_SCALE, PLAYER_SCALE, PLAYER_SCALE);
		glRotatef((float)(angle + 90), 0.0f, 1.0f, 0.0f);

		// and we render. first texture is selected, as there are different teams and within teams there are jokers
		// and nonjokers. and then the model is selected and it also calls the display list.
		textureSelection(playerInfo[i].cPI.team, playerInfo[i].bTPI.joker, 0);
		modelSelection(i);
		// and then there is the shadow.
		// we add 0.001f*j to SHADOW_HEIGHT so that there will be no visual problems of meshes being exactly at the same height.
		glPopMatrix();
		glEnable(GL_BLEND);
		glDisable(GL_LIGHTING);
		glPushMatrix();
		glTranslatef((float)(alpha*playerInfo[i].tPI.location.x + (1-alpha)*playerInfo[i].tPI.lastLocation.x - 0.2f),
		             SHADOW_HEIGHT + 0.001f*j, (float)(alpha*playerInfo[i].tPI.location.z + (1-alpha)*playerInfo[i].tPI.lastLocation.z));
		glScalef(1.0f, 1.0f, 0.6f);
		glBindTexture(GL_TEXTURE_2D, 0);
		glCallList(shadowDisplayList);
		glPopMatrix();
		glDisable(GL_BLEND);
		glEnable(GL_LIGHTING);
		// and then there is the marker at the top of player who is controlled
		if(i == stateInfo.localGameInfo->pII.controlIndex) {
			textureSelection(playerInfo[i].cPI.team, 0, 1);
			glPushMatrix();
			glTranslatef((float)(alpha*playerInfo[i].tPI.location.x + (1-alpha)*playerInfo[i].tPI.lastLocation.x),
			             (float)playerInfo[i].tPI.location.y + 2.0f, (float)(alpha*playerInfo[i].tPI.location.z + (1-alpha)*playerInfo[i].tPI.lastLocation.z));
			glScalef(SELECTION_BALL_SCALE, SELECTION_BALL_SCALE, SELECTION_BALL_SCALE);
			glCallList(markerDisplayList);
			glPopMatrix();
		}
	}
}

static void textureSelection(int team, int joker, int type)
{
	// theres two types of texture selections, texture selection for player and texture selection for marker.
	if(type == 0) {
		// here value means which team, like ankkurit or lippo
		if((stateInfo.globalGameInfo)->teams[team].value == 1) {
			if(joker == 0) {
				glBindTexture(GL_TEXTURE_2D, team1Texture);
			} else {
				glBindTexture(GL_TEXTURE_2D, team1JokerTexture);
			}
		} else if((stateInfo.globalGameInfo)->teams[team].value == 2) {
			if(joker == 0) {
				glBindTexture(GL_TEXTURE_2D, team2Texture);
			} else {
				glBindTexture(GL_TEXTURE_2D, team2JokerTexture);
			}
		} else if((stateInfo.globalGameInfo)->teams[team].value == 3) {
			if(joker == 0) {
				glBindTexture(GL_TEXTURE_2D, team3Texture);
			} else {
				glBindTexture(GL_TEXTURE_2D, team3JokerTexture);
			}
		} else if((stateInfo.globalGameInfo)->teams[team].value == 4) {
			if(joker == 0) {
				glBindTexture(GL_TEXTURE_2D, team4Texture);
			} else {
				glBindTexture(GL_TEXTURE_2D, team4JokerTexture);
			}
		} else if((stateInfo.globalGameInfo)->teams[team].value == 5) {
			if(joker == 0) {
				glBindTexture(GL_TEXTURE_2D, team5Texture);
			} else {
				glBindTexture(GL_TEXTURE_2D, team5JokerTexture);
			}
		} else if((stateInfo.globalGameInfo)->teams[team].value == 6) {
			if(joker == 0) {
				glBindTexture(GL_TEXTURE_2D, team6Texture);
			} else {
				glBindTexture(GL_TEXTURE_2D, team6JokerTexture);
			}
		} else if((stateInfo.globalGameInfo)->teams[team].value == 7) {
			if(joker == 0) {
				glBindTexture(GL_TEXTURE_2D, team7Texture);
			} else {
				glBindTexture(GL_TEXTURE_2D, team7JokerTexture);
			}
		} else if((stateInfo.globalGameInfo)->teams[team].value == 8) {
			if(joker == 0) {
				glBindTexture(GL_TEXTURE_2D, team8Texture);
			} else {
				glBindTexture(GL_TEXTURE_2D, team8JokerTexture);
			}
		}
	}
	// and here, should we use green, blue or red ball on top of a player.
	// depends on who controls.
	else if(type == 1) {
		if((stateInfo.globalGameInfo)->teams[team].control == 0) {
			glBindTexture(GL_TEXTURE_2D, selection1Texture);
		} else if((stateInfo.globalGameInfo)->teams[team].control == 1) {
			glBindTexture(GL_TEXTURE_2D, selection2Texture);
		} else if((stateInfo.globalGameInfo)->teams[team].control == 2) {
			glBindTexture(GL_TEXTURE_2D, selection3Texture);
		}
	}
}

static void modelSelection(int index)
{
	// and then we must select which mesh we use and call the corresponding display list.
	// for animations we just use the animationStage, that is being updated in game_manipulation, as index
	// in mesh arrays.
	int animIndex = stateInfo.localGameInfo->playerInfo[index].cPI.animationStage / stateInfo.localGameInfo->playerInfo[index].cPI.animationFrequency;
	switch(stateInfo.localGameInfo->playerInfo[index].cPI.model) {
	case 0:
		glCallList(playerGloveWithoutBallStandingDisplayList);
		break;
	case 1:
		glCallList(playerGloveWithBallStandingDisplayList);
		break;
	case 2:
		glCallList(playerGloveWithoutBallWalkingDisplayList[animIndex]);
		break;
	case 3:
		glCallList(playerGloveWithBallWalkingDisplayList[animIndex]);
		break;
	case 4:
		glCallList(playerGloveWithoutBallRunningDisplayList[animIndex]);
		break;
	case 5:
		glCallList(playerGloveWithBallRunningDisplayList[animIndex]);
		break;
	case 6:
		glCallList(pitchDownDisplayList[animIndex]);
		break;
	case 7:
		glCallList(pitchUpDisplayList[animIndex]);
		break;
	case 8:
		glCallList(throwLoadDisplayList[animIndex]);
		break;
	case 9:
		glCallList(throwReleaseDisplayList[animIndex]);
		break;
	case 10:
		glCallList(playerBareHandsStandingDisplayList);
		break;
	case 11:
		glCallList(playerBareHandsWalkingDisplayList[animIndex]);
		break;
	case 12:
		glCallList(playerBareHandsRunningDisplayList[animIndex]);
		break;
	case 13:
		glCallList(swingDisplayList[0]);
		break;
	case 14:
		glCallList(swingDisplayList[animIndex]);
		break;
	case 15:
		glCallList(buntDisplayList[animIndex]);
		break;
	case 16:
		glCallList(battingStopDisplayList[animIndex]);
		break;
	case 17:
		break;
	default:
		break;
	}
}
// cleaning keeps the house tidy
int cleanPlayer()
{
	int i;
	cleanMesh(playerBareHandsStandingMesh);
	cleanMesh(playerGloveWithBallStandingMesh);
	cleanMesh(playerGloveWithoutBallStandingMesh);
	cleanMesh(markerMesh);
	cleanMesh(shadowMesh);
	for(i = 0; i < 16; i++) {
		cleanMesh(playerGloveWithBallWalkingMesh[i]);
		cleanMesh(playerGloveWithoutBallWalkingMesh[i]);
		cleanMesh(playerBareHandsWalkingMesh[i]);
	}
	for(i = 0; i < 20; i++) {
		cleanMesh(playerGloveWithBallRunningMesh[i]);
		cleanMesh(playerGloveWithoutBallRunningMesh[i]);
		cleanMesh(playerBareHandsRunningMesh[i]);
	}
	for(i = 0; i < 9; i++) {
		cleanMesh(pitchDownMesh[i]);
	}
	for(i = 0; i < 13; i++) {
		cleanMesh(pitchUpMesh[i]);
	}
	for(i = 0; i < 11; i++) {
		cleanMesh(throwLoadMesh[i]);
	}
	for(i = 0; i < 21; i++) {
		cleanMesh(throwReleaseMesh[i]);
	}
	for(i = 0; i < 34; i++) {
		cleanMesh(swingMesh[i]);
		cleanMesh(buntMesh[i]);
		cleanMesh(battingStopMesh[i]);
	}
	return 0;
}
