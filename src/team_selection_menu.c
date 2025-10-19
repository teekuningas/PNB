#include "team_selection_menu.h"
#include "render.h"
#include "font.h"

#define ARROW_SCALE 0.05f
#define SELECTION_ARROW_LEFT -0.05f
#define SELECTION_ARROW_RIGHT 0.4f

#define SELECTION_TEXT_LEFT -0.35f
#define SELECTION_TEXT_RIGHT 0.1f
#define SELECTION_ALT_1_HEIGHT -0.1f
#define SELECTION_ALT_OFFSET 0.06f
#define SELECTION_TEAM_TEXT_HEIGHT -0.2f

#define DEFAULT_CONTROLLED_1 0
#define DEFAULT_CONTROLLED_2 2
#define DEFAULT_TEAM_1 0
#define DEFAULT_TEAM_2 1



static void drawSelection(MenuData* menuData, StateInfo* stateInfo)
{
	printText("Game setup", 10, SELECTION_TEXT_LEFT + 0.1f, -0.4f, 5);

	if(menuData->stage_1_state == TEAM_SELECTION_STAGE_TEAM_1) {
		int i;
		printText("Team 1", 6, SELECTION_TEXT_LEFT, SELECTION_TEAM_TEXT_HEIGHT, 4);
		for(i = 0; i < stateInfo->numTeams; i++) {
			char* str = stateInfo->teamData[i].name;
			printText(str, strlen(str), SELECTION_TEXT_LEFT, SELECTION_ALT_1_HEIGHT + i*SELECTION_ALT_OFFSET, 2);
		}
	} else if(menuData->stage_1_state == TEAM_SELECTION_STAGE_CONTROL_1) {
		printText("Controlled by", 13, -0.5f, SELECTION_TEAM_TEXT_HEIGHT, 3);
		printText("Pad 1", 5, SELECTION_TEXT_LEFT, SELECTION_ALT_1_HEIGHT, 2);
		printText("Pad 2", 5, SELECTION_TEXT_LEFT, SELECTION_ALT_1_HEIGHT + SELECTION_ALT_OFFSET, 2);
		printText("AI", 2, SELECTION_TEXT_LEFT, SELECTION_ALT_1_HEIGHT + 2*SELECTION_ALT_OFFSET, 2);
	}

	else if(menuData->stage_1_state == TEAM_SELECTION_STAGE_TEAM_2) {
		int i;
		printText("OK", 2, SELECTION_TEXT_LEFT, SELECTION_ALT_1_HEIGHT + SELECTION_ALT_OFFSET, 4);
		printText("Team 2", 6, SELECTION_TEXT_RIGHT, SELECTION_TEAM_TEXT_HEIGHT, 4);
		for(i = 0; i < stateInfo->numTeams; i++) {
			char* str = stateInfo->teamData[i].name;
			printText(str, strlen(str), SELECTION_TEXT_RIGHT, SELECTION_ALT_1_HEIGHT + i*SELECTION_ALT_OFFSET, 2);
		}
	} else if(menuData->stage_1_state == TEAM_SELECTION_STAGE_CONTROL_2) {
		printText("OK", 2, SELECTION_TEXT_LEFT, SELECTION_ALT_1_HEIGHT + SELECTION_ALT_OFFSET, 4);
		printText("Controlled by", 13, -0.05f, SELECTION_TEAM_TEXT_HEIGHT, 3);
		printText("Pad 1", 5, SELECTION_TEXT_RIGHT, SELECTION_ALT_1_HEIGHT, 2);
		printText("Pad 2", 5, SELECTION_TEXT_RIGHT, SELECTION_ALT_1_HEIGHT + SELECTION_ALT_OFFSET, 2);
		printText("AI", 2, SELECTION_TEXT_RIGHT, SELECTION_ALT_1_HEIGHT + 2*SELECTION_ALT_OFFSET, 2);
	} else if(menuData->stage_1_state == TEAM_SELECTION_STAGE_INNINGS) {
		printText("select number of innings", 24, -0.45f, -0.25f, 3);
		printText("1", 1, -0.05f, SELECTION_ALT_1_HEIGHT, 3);
		printText("2", 1, -0.05f, SELECTION_ALT_1_HEIGHT + SELECTION_ALT_OFFSET, 3);
		printText("4", 1, -0.05f, SELECTION_ALT_1_HEIGHT + 2*SELECTION_ALT_OFFSET, 3);
	}
}


void updateTeamSelectionMenu(MenuData* menuData, StateInfo* stateInfo, MenuInfo* menuInfo, KeyStates* keyStates, GlobalGameInfo* globalGameInfo)
{
	switch(menuData->stage_1_state) {
	case TEAM_SELECTION_STAGE_TEAM_1:
		if(keyStates->released[0][KEY_1]) {
			menuData->stage = MENU_STAGE_FRONT;
			menuData->rem = 4;
			menuData->pointer = 0;

		}
		if(keyStates->released[0][KEY_2]) {
			menuData->stage_1_state = TEAM_SELECTION_STAGE_CONTROL_1;
			menuData->team1 = menuData->pointer;
			menuData->pointer = DEFAULT_CONTROLLED_1;
			menuData->rem = 3;
		}
		if(keyStates->released[0][KEY_DOWN]) {
			menuData->pointer +=1;
			menuData->pointer = (menuData->pointer+menuData->rem)%menuData->rem;
		}
		if(keyStates->released[0][KEY_UP]) {
			menuData->pointer -=1;
			menuData->pointer = (menuData->pointer+menuData->rem)%menuData->rem;
		}
		break;
	case TEAM_SELECTION_STAGE_CONTROL_1:
		if(keyStates->released[0][KEY_1]) {
			menuData->stage_1_state = TEAM_SELECTION_STAGE_TEAM_1;
			menuData->pointer = 0;
			menuData->rem = stateInfo->numTeams;

		}
		if(keyStates->released[0][KEY_2]) {
			menuData->stage_1_state = TEAM_SELECTION_STAGE_TEAM_2;
			menuData->team1_control = menuData->pointer;
			menuData->pointer = DEFAULT_TEAM_2;
			menuData->rem = stateInfo->numTeams;
		}
		if(keyStates->released[0][KEY_DOWN]) {
			menuData->pointer +=1;
			menuData->pointer = (menuData->pointer+menuData->rem)%menuData->rem;
		}
		if(keyStates->released[0][KEY_UP]) {
			menuData->pointer -=1;
			menuData->pointer = (menuData->pointer+menuData->rem)%menuData->rem;
		}
		break;
	case TEAM_SELECTION_STAGE_TEAM_2:
		if(keyStates->released[0][KEY_1]) {
			menuData->stage_1_state = TEAM_SELECTION_STAGE_CONTROL_1;
			menuData->rem = 3;
			menuData->pointer = 0;

		}
		if(keyStates->released[0][KEY_2]) {
			menuData->stage_1_state = TEAM_SELECTION_STAGE_CONTROL_2;
			menuData->team2 = menuData->pointer;
			menuData->rem = 3;
			menuData->pointer = DEFAULT_CONTROLLED_2;
			if(menuData->pointer == menuData->team1_control) {
				menuData->pointer++;
				menuData->pointer = (menuData->pointer+menuData->rem)%menuData->rem;
			}
		}
		if(keyStates->released[0][KEY_DOWN]) {
			menuData->pointer +=1;
			menuData->pointer = (menuData->pointer+menuData->rem)%menuData->rem;
		}
		if(keyStates->released[0][KEY_UP]) {
			menuData->pointer -=1;
			menuData->pointer = (menuData->pointer+menuData->rem)%menuData->rem;
		}
		break;
	case TEAM_SELECTION_STAGE_CONTROL_2:
		if(keyStates->released[0][KEY_1]) {
			menuData->stage_1_state = TEAM_SELECTION_STAGE_TEAM_2;
			menuData->rem = stateInfo->numTeams;
			menuData->pointer = 0;

		}
		if(keyStates->released[0][KEY_2]) {
			menuData->stage_1_state = TEAM_SELECTION_STAGE_INNINGS;
			menuData->team2_control = menuData->pointer;
			menuData->pointer = 1;
			menuData->rem = 3;
		}
		if(keyStates->released[0][KEY_DOWN]) {
			menuData->pointer +=1;
			menuData->pointer = (menuData->pointer+menuData->rem)%menuData->rem;
			if(menuData->pointer == menuData->team1_control) {
				menuData->pointer++;
				menuData->pointer = (menuData->pointer+menuData->rem)%menuData->rem;
			}
		}
		if(keyStates->released[0][KEY_UP]) {
			menuData->pointer -=1;
			menuData->pointer = (menuData->pointer+menuData->rem)%menuData->rem;
			if(menuData->pointer == menuData->team1_control) {
				menuData->pointer--;
				menuData->pointer = (menuData->pointer+menuData->rem)%menuData->rem;
			}
		}
		break;
	case TEAM_SELECTION_STAGE_INNINGS:
		if(keyStates->released[0][KEY_1]) {
			menuData->stage_1_state = TEAM_SELECTION_STAGE_CONTROL_2;
			menuData->rem = 3;
			menuData->pointer = 0;
			if(menuData->pointer == menuData->team1_control) {
				menuData->pointer++;
				menuData->pointer = (menuData->pointer+menuData->rem)%menuData->rem;
			}
		}
		if(keyStates->released[0][KEY_2]) {
			menuData->stage = MENU_STAGE_BATTING_ORDER_1;
			// cupGame = 0; // This was in the original function, but cupGame is static to main_menu.c.
			// We'll need to handle this dependency later. For now, we comment it out.
			if(menuData->pointer == 0) menuData->inningsInPeriod = 2;
			else if(menuData->pointer == 1) menuData->inningsInPeriod = 4;
			else if(menuData->pointer == 2) menuData->inningsInPeriod = 8;

			menuData->pointer = 0;
			menuData->rem = 13;
		}
		if(keyStates->released[0][KEY_DOWN]) {
			menuData->pointer +=1;
			menuData->pointer = (menuData->pointer+menuData->rem)%menuData->rem;
		}
		if(keyStates->released[0][KEY_UP]) {
			menuData->pointer -=1;
			menuData->pointer = (menuData->pointer+menuData->rem)%menuData->rem;
		}
		break;
	}
}

void drawTeamSelectionMenu(MenuData* menuData, StateInfo* stateInfo, MenuInfo* menuInfo, double alpha)
{
	drawFontBackground();

	glBindTexture(GL_TEXTURE_2D, menuData->arrowTexture);
	glPushMatrix();
	if(menuData->stage_1_state == TEAM_SELECTION_STAGE_TEAM_1 || menuData->stage_1_state == TEAM_SELECTION_STAGE_CONTROL_1) {
		glTranslatef(SELECTION_ARROW_LEFT, 1.0f, SELECTION_ALT_1_HEIGHT + SELECTION_ALT_OFFSET*menuData->pointer);
	} else if(menuData->stage_1_state == TEAM_SELECTION_STAGE_TEAM_2 || menuData->stage_1_state == TEAM_SELECTION_STAGE_CONTROL_2) {
		glTranslatef(SELECTION_ARROW_RIGHT, 1.0f, SELECTION_ALT_1_HEIGHT + SELECTION_ALT_OFFSET*menuData->pointer);
	} else if(menuData->stage_1_state == TEAM_SELECTION_STAGE_INNINGS) {
		glTranslatef(SELECTION_ARROW_RIGHT, 1.0f, SELECTION_ALT_1_HEIGHT + SELECTION_ALT_OFFSET*menuData->pointer);
	}
	glScalef(ARROW_SCALE, ARROW_SCALE, ARROW_SCALE);
	glCallList(menuData->planeDisplayList);
	glPopMatrix();
	drawSelection(menuData, stateInfo);
}
