#include "player_renderer.h"
#include "../core/render.h" // For GL-related functions and MeshObject
#include <GL/glew.h>
#include <GL/gl.h>
#include <stdlib.h> // For malloc
#include <math.h>   // For atan2

#define PLAYER_SCALE 0.24f
#define SELECTION_BALL_SCALE 0.2f

// Forward declarations for static helper functions
static void textureSelection(const StateInfo* stateInfo, int team, int joker, int type, ResourceManager* rm);
static void modelSelection(const StateInfo* stateInfo, int index, ResourceManager* rm);


int initPlayerRenderer(ResourceManager* rm)
{
	return 0;
}

void drawPlayerRenderer(const StateInfo* stateInfo, const PlayerInfo *playerInfo, double alpha, ResourceManager* rm)
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
		textureSelection(stateInfo, playerInfo[i].cPI.team, playerInfo[i].bTPI.joker, 0, rm);
		modelSelection(stateInfo, i, rm);
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
		glCallList(resource_manager_get_model(rm, "data/models/shadow.obj"));
		glPopMatrix();
		glDisable(GL_BLEND);
		glEnable(GL_LIGHTING);
		// and then there is the marker at the top of player who is controlled
		if(i == stateInfo->localGameInfo->pII.controlIndex) {
			textureSelection(stateInfo, playerInfo[i].cPI.team, 0, 1, rm);
			glPushMatrix();
			glTranslatef((float)(alpha*playerInfo[i].tPI.location.x + (1-alpha)*playerInfo[i].tPI.lastLocation.x),
			             (float)playerInfo[i].tPI.location.y + 2.0f, (float)(alpha*playerInfo[i].tPI.location.z + (1-alpha)*playerInfo[i].tPI.lastLocation.z));
			glScalef(SELECTION_BALL_SCALE, SELECTION_BALL_SCALE, SELECTION_BALL_SCALE);
			glCallList(resource_manager_get_model(rm, "data/models/pallo.obj"));
			glPopMatrix();
		}
	}
}

static void textureSelection(const StateInfo* stateInfo, int team, int joker, int type, ResourceManager* rm)
{
	// theres two types of texture selections, texture selection for player and texture selection for marker.
	if(type == 0) {
		char path[128];
		int team_val = (stateInfo->globalGameInfo)->teams[team].value;
		if(joker == 0) {
			sprintf(path, "data/textures/team%d.tga", team_val);
		} else {
			sprintf(path, "data/textures/team%d_joker.tga", team_val);
		}
		glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, path));
	}
	// and here, should we use green, blue or red ball on top of a player.
	// depends on who controls.
	else if(type == 1) {
		int control = (stateInfo->globalGameInfo)->teams[team].control;
		if(control == 0) {
			glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, "data/textures/selectionBall1.tga"));
		} else if(control == 1) {
			glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, "data/textures/selectionBall2.tga"));
		} else if(control == 2) {
			glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, "data/textures/selectionBall3.tga"));
		}
	}
}

static void modelSelection(const StateInfo* stateInfo, int index, ResourceManager* rm)
{
	// and then we must select which mesh we use and call the corresponding display list.
	// for animations we just use the animationStage, that is being updated in game_manipulation, as index
	// in mesh arrays.
	int animIndex = stateInfo->localGameInfo->playerInfo[index].cPI.animationStage / stateInfo->localGameInfo->playerInfo[index].cPI.animationFrequency;
	char path[128];
	switch(stateInfo->localGameInfo->playerInfo[index].cPI.model) {
	case PLAYER_ANIM_STAND_NO_BALL:
		glCallList(resource_manager_get_model(rm, "data/models/player_glove_without_ball_standing.obj"));
		break;
	case PLAYER_ANIM_STAND_WITH_BALL:
		glCallList(resource_manager_get_model(rm, "data/models/player_glove_with_ball_standing.obj"));
		break;
	case PLAYER_ANIM_WALK_NO_BALL:
		sprintf(path, "data/models/player_glove_without_ball_walking/player_glove_without_ball_walking_%06d.obj", animIndex + 1);
		glCallList(resource_manager_get_model(rm, path));
		break;
	case PLAYER_ANIM_WALK_WITH_BALL:
		sprintf(path, "data/models/player_glove_with_ball_walking/player_glove_with_ball_walking_%06d.obj", animIndex + 1);
		glCallList(resource_manager_get_model(rm, path));
		break;
	case PLAYER_ANIM_RUN_NO_BALL:
		sprintf(path, "data/models/player_glove_without_ball_running/player_glove_without_ball_running_%06d.obj", animIndex + 1);
		glCallList(resource_manager_get_model(rm, path));
		break;
	case PLAYER_ANIM_RUN_WITH_BALL:
		sprintf(path, "data/models/player_glove_with_ball_running/player_glove_with_ball_running_%06d.obj", animIndex + 1);
		glCallList(resource_manager_get_model(rm, path));
		break;
	case PLAYER_ANIM_PITCH_WINDUP:
		sprintf(path, "data/models/pitch/pitch_down_%06d.obj", animIndex + 1);
		glCallList(resource_manager_get_model(rm, path));
		break;
	case PLAYER_ANIM_PITCH_THROW:
		sprintf(path, "data/models/pitch/pitch_up_%06d.obj", animIndex + 1);
		glCallList(resource_manager_get_model(rm, path));
		break;
	case PLAYER_ANIM_THROW_WINDUP:
		sprintf(path, "data/models/throw/throw_load_%06d.obj", animIndex + 1);
		glCallList(resource_manager_get_model(rm, path));
		break;
	case PLAYER_ANIM_THROW_RELEASE:
		sprintf(path, "data/models/throw/throw_release_%06d.obj", animIndex + 1);
		glCallList(resource_manager_get_model(rm, path));
		break;
	case PLAYER_ANIM_STAND_BARE:
		glCallList(resource_manager_get_model(rm, "data/models/player_bare_hands_standing.obj"));
		break;
	case PLAYER_ANIM_WALK_BARE:
		sprintf(path, "data/models/player_bare_hands_walking/player_bare_hands_walking_%06d.obj", animIndex + 1);
		glCallList(resource_manager_get_model(rm, path));
		break;
	case PLAYER_ANIM_RUN_BARE:
		sprintf(path, "data/models/player_bare_hands_running/player_bare_hands_running_%06d.obj", animIndex + 1);
		glCallList(resource_manager_get_model(rm, path));
		break;
	case PLAYER_ANIM_BATTER_READY:
		glCallList(resource_manager_get_model(rm, "data/models/batting/swing_000001.obj"));
		break;
	case PLAYER_ANIM_BAT_SWING_1:
		sprintf(path, "data/models/batting/swing_%06d.obj", animIndex + 1);
		glCallList(resource_manager_get_model(rm, path));
		break;
	case PLAYER_ANIM_BAT_SWING_2:
		sprintf(path, "data/models/batting/bunt_%06d.obj", animIndex + 1);
		glCallList(resource_manager_get_model(rm, path));
		break;
	case PLAYER_ANIM_BAT_SWING_3:
		sprintf(path, "data/models/batting/batting_stop_%06d.obj", animIndex + 1);
		glCallList(resource_manager_get_model(rm, path));
		break;
	default:
		break;
	}
}

int cleanPlayerRenderer(void)
{
	return 0;
}
