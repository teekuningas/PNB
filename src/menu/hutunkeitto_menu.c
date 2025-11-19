#include "hutunkeitto_menu.h"
#include "render.h"
#include "font.h"
#include "globals.h"
#include "menu_types.h"
#include "menu_helpers.h"

#define BAT_DEFAULT_HEIGHT -1.0f
#define LEFT_HAND_DEFAULT_HEIGHT 0.1f
#define RIGHT_HAND_DEFAULT_HEIGHT 0.22f
#define SCALE_FACTOR 0.005f
#define POSITION_SCALE_ADDITION 0.00065f
#define BAT_MOVING_SPEED 0.0025f
#define BAT_DROP_MOVING_SPEED 0.03f
#define SCALE_LIMIT 30
#define HAND_WIDTH 0.12f
#define BAT_HEIGHT_CONSTANT 0.58f
#define MOVING_AWAY_SPEED 0.002f
#define HUTUNKEITTO_TEAM_TEXT_HEIGHT 0.45f
#define HUTUNKEITTO_TEAM_1_TEXT_POSITION 0.2f
#define HUTUNKEITTO_TEAM_2_TEXT_POSITION 0.55f

void initHutunkeittoState(HutunkeittoState *state)
{
	state->batTimer = 0;
	state->batTimerLimit = 0;
	state->batTimerCount = 0;
	state->state = 0;
	state->batHeight = BAT_DEFAULT_HEIGHT;
	state->batPosition = 0.0f;
	state->leftReady = 0;
	state->rightReady = 0;
	state->turnCount = 0;
	state->handsZ = 1.0f;
	state->leftHandHeight = 0.15f;
	state->leftHandPosition = -0.075f;
	state->rightHandHeight = 0.25f;
	state->rightHandPosition = 0.075f;
	state->refereeHandHeight = 0.15f;
	state->tempLeftHeight = 0.0f;
	state->leftScaleCount = 0;
	state->rightScaleCount = 0;
	state->playsFirst = 0;
	state->pointer = 0;
	state->rem = 2;
}

MenuStage updateHutunkeittoMenu(HutunkeittoState *state, const KeyStates *keyStates, int team1_control, int team2_control, GameSetup *gameSetup)
{
	if(state->state == 0) {
		state->batTimerLimit = 30 + rand()%15;
		state->batTimer = 0;
		state->state = 1;
	} else if(state->state == 1) {
		state->batTimer+=1;
		state->batHeight = BAT_DEFAULT_HEIGHT + state->batTimer*BAT_DROP_MOVING_SPEED;
		if(state->batTimer > state->batTimerLimit) {
			state->state = 2;
			state->batTimerCount = state->batTimer;
			state->batTimer = 0;
		}
	} else if(state->state == 2) {
		state->rightHandPosition = 0.0f;
		state->rightHandHeight = RIGHT_HAND_DEFAULT_HEIGHT;
		state->leftHandPosition = 0.0f;
		state->leftHandHeight = LEFT_HAND_DEFAULT_HEIGHT;
		state->state = 3;

	} else if(state->state == 3) {
		if(team1_control != 2) {
			if(keyStates->down[team1_control][KEY_UP]) {
				if(state->leftScaleCount < SCALE_LIMIT) {
					state->leftScaleCount += 1;
				}
			} else if(keyStates->down[team1_control][KEY_DOWN]) {
				if(state->leftScaleCount > -SCALE_LIMIT) {
					state->leftScaleCount -= 1;
				}
			}
			if(keyStates->released[team1_control][KEY_2]) {
				state->leftReady = 1;
			}
		} else {
			state->leftReady = 1;
		}
		if(team2_control != 2) {
			if(keyStates->down[team2_control][KEY_UP]) {
				if(state->rightScaleCount < SCALE_LIMIT) {
					state->rightScaleCount += 1;
					state->leftHandHeight = LEFT_HAND_DEFAULT_HEIGHT - state->rightScaleCount*POSITION_SCALE_ADDITION;
				}
			} else if(keyStates->down[team2_control][KEY_DOWN]) {
				if(state->rightScaleCount > -SCALE_LIMIT) {
					state->rightScaleCount -= 1;
					state->leftHandHeight = LEFT_HAND_DEFAULT_HEIGHT - state->rightScaleCount*POSITION_SCALE_ADDITION;
				}
			}
			if(keyStates->released[team2_control][KEY_2]) {
				state->rightReady = 1;
			}
		} else {
			state->rightReady = 1;
		}
		if(state->leftReady == 1 && state->rightReady == 1) {
			state->state = 4;
		}
	} else if(state->state == 4) {
		float turnHeight = (HAND_WIDTH*(1.0f+SCALE_FACTOR*state->leftScaleCount) +HAND_WIDTH*(1.0f+SCALE_FACTOR*state->rightScaleCount)) / 2;
		state->batTimer += 1;
		state->batHeight = BAT_DEFAULT_HEIGHT + state->batTimerCount*BAT_DROP_MOVING_SPEED + state->batTimer*BAT_MOVING_SPEED;
		if(state->turnCount%2 == 0) {
			if(state->rightHandHeight < RIGHT_HAND_DEFAULT_HEIGHT - turnHeight) {
				if(state->batHeight - BAT_HEIGHT_CONSTANT > RIGHT_HAND_DEFAULT_HEIGHT - turnHeight) {
					state->state = 5;
					state->batTimer = 0;
				} else {
					state->turnCount += 1;
					state->tempLeftHeight = state->leftHandHeight;
				}
			} else {
				state->leftHandHeight += BAT_MOVING_SPEED;
				state->rightHandHeight -= BAT_MOVING_SPEED;
			}
		} else if(state->turnCount%2 == 1) {
			if(state->leftHandHeight < state->tempLeftHeight - turnHeight) {
				if(state->batHeight - BAT_HEIGHT_CONSTANT > state->tempLeftHeight - turnHeight) {
					state->state = 5;
					state->batTimer = 0;
				} else {
					state->turnCount += 1;
				}
			} else {
				state->leftHandHeight -= BAT_MOVING_SPEED;
				state->rightHandHeight += BAT_MOVING_SPEED;
			}
		}
	} else if(state->state == 5) {
		if(state->batTimer < 50) {
			state->batTimer += 1;
		} else {
			state->state = 6;
		}
		if(state->turnCount%2 == 0) {
			state->batPosition = -state->batTimer*MOVING_AWAY_SPEED;
			state->leftHandPosition = -state->batTimer*MOVING_AWAY_SPEED;
		} else if(state->turnCount%2 == 1) {
			state->batPosition = state->batTimer*MOVING_AWAY_SPEED;
			state->rightHandPosition = state->batTimer*MOVING_AWAY_SPEED;
		}
		state->handsZ = 1.0f - state->batTimer*MOVING_AWAY_SPEED/2;
	} else if(state->state == 6) {
		int control;
		if(state->turnCount%2 == 0) {
			control = team1_control;
		} else control = team2_control;
		if(control != 2) {
			if(keyStates->released[control][KEY_2]) {
				if(state->pointer == 0) {
					state->playsFirst = 0;
				} else {
					state->playsFirst = 1;
				}
				gameSetup->playsFirst = state->playsFirst;
				return MENU_STAGE_GO_TO_GAME;
			}
			if(keyStates->released[control][KEY_RIGHT]) {
				state->pointer +=1;
				state->pointer = (state->pointer+state->rem)%state->rem;
			}
			if(keyStates->released[control][KEY_LEFT]) {
				state->pointer -=1;
				state->pointer = (state->pointer+state->rem)%state->rem;
			}
		} else {
			// ai always selects to field first
			if(state->turnCount%2 == 0) state->playsFirst = 1;
			else state->playsFirst = 0;
			gameSetup->playsFirst = state->playsFirst;
			return MENU_STAGE_GO_TO_GAME;
		}
	}
	return MENU_STAGE_HUTUNKEITTO;
}

void drawHutunkeittoMenu(const HutunkeittoState *state, const RenderState* rs, ResourceManager* rm, int team1_idx, int team2_idx)
{
	// --- 2D Background ---
	begin_2d_render(rs);
	drawMenuLayout2D(rm, rs);

	// --- 3D Drawing ---
	glClear(GL_DEPTH_BUFFER_BIT); // Clear depth buffer to draw 3D models on top of the 2D background
	begin_3d_render(rs);
	gluLookAt(0.0f, 2.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f);

	float lightPos[4] = {LIGHT_SOURCE_POSITION_X, LIGHT_SOURCE_POSITION_Y, LIGHT_SOURCE_POSITION_Z, 1.0f};
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

	GLuint bat_model = resource_manager_get_model(rm, "data/models/hutunkeitto_bat.obj");
	GLuint hand_model = resource_manager_get_model(rm, "data/models/hutunkeitto_hand.obj");

	char team1_texture_path[64];
	sprintf(team1_texture_path, "data/textures/team%d.tga", team1_idx + 1);
	char team2_texture_path[64];
	sprintf(team2_texture_path, "data/textures/team%d.tga", team2_idx + 1);

	// bat
	glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, team1_texture_path));
	glPushMatrix();
	glTranslatef(state->batPosition, state->handsZ, state->batHeight);
	glScalef(0.6f, 0.5f, 0.45f);
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glCallList(bat_model);
	glPopMatrix();

	// right hand (team 2)
	glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, team2_texture_path));
	glPushMatrix();
	glTranslatef(state->rightHandPosition, state->handsZ, state->rightHandHeight);
	glScalef(0.5f, 0.5f, 0.5f*(1.0f+state->rightScaleCount*SCALE_FACTOR));
	glTranslatef(0.0f, 0.0f, -0.35f);
	glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glCallList(hand_model);
	glPopMatrix();

	// left hand (team 1)
	glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, team1_texture_path));
	glPushMatrix();
	glTranslatef(state->leftHandPosition, state->handsZ, state->leftHandHeight);
	glScalef(0.5f, 0.5f, 0.5f*(1.0f+state->leftScaleCount*SCALE_FACTOR));
	glTranslatef(0.0f, 0.0f, -0.35f);
	glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glCallList(hand_model);
	glPopMatrix();

	// referee hand
	glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, "data/textures/team2.tga")); // Referee has a default texture
	glPushMatrix();
	glTranslatef(0.0f, 1.0f, state->refereeHandHeight);
	glScalef(0.5f, 0.5f, 0.5f);
	glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glCallList(hand_model);
	glPopMatrix();

	// --- 2D UI ---
	begin_2d_render(rs);

	if(state->state == 6) {
		const float center_x = VIRTUAL_WIDTH / 2.0f;
		const float title_y = VIRTUAL_HEIGHT * 0.2f;
		const float team_y = VIRTUAL_HEIGHT * 0.3f;
		const float team1_x = VIRTUAL_WIDTH * 0.25f;
		const float team2_x = VIRTUAL_WIDTH * 0.75f;
		const float title_size = 48.0f;
		const float team_size = 36.0f;
		const float arrow_size = 60.0f;

		draw_text_2d("Who bats first?", center_x, title_y, title_size, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("Team 1", team1_x, team_y, team_size, TEXT_ALIGN_CENTER, rs);
		draw_text_2d("Team 2", team2_x, team_y, team_size, TEXT_ALIGN_CENTER, rs);

		// arrow
		float arrow_x = (state->pointer == 0) ? team1_x + 100.0f : team2_x + 100.0f;
		float arrow_y = team_y - (arrow_size - team_size) / 2.0f;
		draw_texture_2d(resource_manager_get_texture(rm, "data/textures/arrow.tga"), arrow_x, arrow_y, arrow_size, arrow_size);
	}
}
