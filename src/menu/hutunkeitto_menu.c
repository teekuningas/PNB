#include "hutunkeitto_menu.h"
#include "render.h"
#include "font.h"
#include "main_menu.h"
#include "globals.h"
#include "menu_types.h"

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

static void drawHutunkeittoTexts(const HutunkeittoState *state)
{
	printText("Who bats first", 14, HUTUNKEITTO_TEAM_1_TEXT_POSITION, HUTUNKEITTO_TEAM_TEXT_HEIGHT - 0.1f, 3);
	printText("Team 1", 6, HUTUNKEITTO_TEAM_1_TEXT_POSITION, HUTUNKEITTO_TEAM_TEXT_HEIGHT, 3);
	printText("Team 2", 6, HUTUNKEITTO_TEAM_2_TEXT_POSITION, HUTUNKEITTO_TEAM_TEXT_HEIGHT, 3);
}

void initHutunkeittoState(HutunkeittoState *state)
{
	state->batTimer = 0;
	state->batTimerLimit = 0;
	state->batTimerCount = 0;
	state->state = 0;
	state->updatingCanStart = 0;
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

MenuStage updateHutunkeittoMenu(HutunkeittoState *state, const KeyStates *keyStates, int team1_control, int team2_control)
{
	if(state->state == 0 && state->updatingCanStart == 1) {
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
	}
	// finally all that messy stuff is over and we just decide the winner
	else if(state->state == 6) {
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
			return MENU_STAGE_GO_TO_GAME;
		}
	}
	return MENU_STAGE_HUTUNKEITTO;
}

void drawHutunkeittoMenu(const HutunkeittoState *state, const struct MenuData *menuData)
{
	drawFontBackground();
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	float lightPos[4];
	lightPos[0] = LIGHT_SOURCE_POSITION_X;
	lightPos[1] = LIGHT_SOURCE_POSITION_Y;
	lightPos[2] = LIGHT_SOURCE_POSITION_Z;
	lightPos[3] = 1.0f;
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
	// bat
	glBindTexture(GL_TEXTURE_2D, menuData->team1Texture);
	glPushMatrix();
	glTranslatef(state->batPosition, state->handsZ, state->batHeight);
	glScalef(0.6f, 0.5f, 0.45f);
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glCallList(menuData->batDisplayList);
	glPopMatrix();
	// right hand
	if(menuData->team2 == 0) glBindTexture(GL_TEXTURE_2D, menuData->team1Texture);
	else if(menuData->team2 == 1) glBindTexture(GL_TEXTURE_2D, menuData->team2Texture);
	else if(menuData->team2 == 2) glBindTexture(GL_TEXTURE_2D, menuData->team3Texture);
	else if(menuData->team2 == 3) glBindTexture(GL_TEXTURE_2D, menuData->team4Texture);
	else if(menuData->team2 == 4) glBindTexture(GL_TEXTURE_2D, menuData->team5Texture);
	else if(menuData->team2 == 5) glBindTexture(GL_TEXTURE_2D, menuData->team6Texture);
	else if(menuData->team2 == 6) glBindTexture(GL_TEXTURE_2D, menuData->team7Texture);
	else if(menuData->team2 == 7) glBindTexture(GL_TEXTURE_2D, menuData->team8Texture);
	glPushMatrix();
	glTranslatef(state->rightHandPosition, state->handsZ, state->rightHandHeight);
	glScalef(0.5f, 0.5f, 0.5f*(1.0f+state->rightScaleCount*SCALE_FACTOR));
	glTranslatef(0.0f, 0.0f, -0.35f);
	glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glCallList(menuData->handDisplayList);
	glPopMatrix();
	// left hand
	if(menuData->team1 == 0) glBindTexture(GL_TEXTURE_2D, menuData->team1Texture);
	else if(menuData->team1 == 1) glBindTexture(GL_TEXTURE_2D, menuData->team2Texture);
	else if(menuData->team1 == 2) glBindTexture(GL_TEXTURE_2D, menuData->team3Texture);
	else if(menuData->team1 == 3) glBindTexture(GL_TEXTURE_2D, menuData->team4Texture);
	else if(menuData->team1 == 4) glBindTexture(GL_TEXTURE_2D, menuData->team5Texture);
	else if(menuData->team1 == 5) glBindTexture(GL_TEXTURE_2D, menuData->team6Texture);
	else if(menuData->team1 == 6) glBindTexture(GL_TEXTURE_2D, menuData->team7Texture);
	else if(menuData->team1 == 7) glBindTexture(GL_TEXTURE_2D, menuData->team8Texture);
	glPushMatrix();
	glTranslatef(state->leftHandPosition, state->handsZ, state->leftHandHeight);
	glScalef(0.5f, 0.5f, 0.5f*(1.0f+state->leftScaleCount*SCALE_FACTOR));
	glTranslatef(0.0f, 0.0f, -0.35f);
	glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glCallList(menuData->handDisplayList);
	glPopMatrix();
	// referee hand
	glBindTexture(GL_TEXTURE_2D, menuData->team2Texture);
	glPushMatrix();
	glTranslatef(0.0f, 1.0f, state->refereeHandHeight);
	glScalef(0.5f, 0.5f, 0.5f);
	glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glCallList(menuData->handDisplayList);
	glPopMatrix();

	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	if(state->state == 6) {
		drawHutunkeittoTexts(state);
		// arrow
		glBindTexture(GL_TEXTURE_2D, menuData->arrowTexture);
		glPushMatrix();
		if(state->pointer == 0) glTranslatef(HUTUNKEITTO_TEAM_1_TEXT_POSITION + 0.25f, 1.0f, HUTUNKEITTO_TEAM_TEXT_HEIGHT);
		else glTranslatef(HUTUNKEITTO_TEAM_2_TEXT_POSITION + 0.25f, 1.0f, HUTUNKEITTO_TEAM_TEXT_HEIGHT);
		glScalef(ARROW_SCALE, ARROW_SCALE, ARROW_SCALE);
		glCallList(menuData->planeDisplayList);
		glPopMatrix();
	}
}
