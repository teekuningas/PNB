#include "team_selection_menu.h"
#include "render.h"
#include "font.h"
#include "main_menu.h"

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

void initTeamSelectionState(TeamSelectionState *state)
{
	state->stage_1_state = TEAM_SELECTION_STAGE_TEAM_1;
	state->pointer = DEFAULT_TEAM_1;
	state->team1 = DEFAULT_TEAM_1;
	state->team2 = DEFAULT_TEAM_2;
	state->team1_controller = DEFAULT_CONTROLLED_1;
	state->team2_controller = DEFAULT_CONTROLLED_2;
	state->innings = 2; // Default innings
	state->rem = 0; // Will be set properly when initialized
	state->cupGame = 0; // A standard game is never a cup game.
}
static void drawSelection(const TeamSelectionState* state, const StateInfo* stateInfo)
{
	printText("Game setup", 10, SELECTION_TEXT_LEFT + 0.1f, -0.4f, 5);

	if(state->stage_1_state == TEAM_SELECTION_STAGE_TEAM_1) {
		int i;
		printText("Team 1", 6, SELECTION_TEXT_LEFT, SELECTION_TEAM_TEXT_HEIGHT, 4);
		for(i = 0; i < stateInfo->numTeams; i++) {
			char* str = stateInfo->teamData[i].name;
			printText(str, strlen(str), SELECTION_TEXT_LEFT, SELECTION_ALT_1_HEIGHT + i*SELECTION_ALT_OFFSET, 2);
		}
	} else if(state->stage_1_state == TEAM_SELECTION_STAGE_CONTROL_1) {
		printText("Controlled by", 13, -0.5f, SELECTION_TEAM_TEXT_HEIGHT, 3);
		printText("Pad 1", 5, SELECTION_TEXT_LEFT, SELECTION_ALT_1_HEIGHT, 2);
		printText("Pad 2", 5, SELECTION_TEXT_LEFT, SELECTION_ALT_1_HEIGHT + SELECTION_ALT_OFFSET, 2);
		printText("AI", 2, SELECTION_TEXT_LEFT, SELECTION_ALT_1_HEIGHT + 2*SELECTION_ALT_OFFSET, 2);
	}

	else if(state->stage_1_state == TEAM_SELECTION_STAGE_TEAM_2) {
		int i;
		printText("OK", 2, SELECTION_TEXT_LEFT, SELECTION_ALT_1_HEIGHT + SELECTION_ALT_OFFSET, 4);
		printText("Team 2", 6, SELECTION_TEXT_RIGHT, SELECTION_TEAM_TEXT_HEIGHT, 4);
		for(i = 0; i < stateInfo->numTeams; i++) {
			char* str = stateInfo->teamData[i].name;
			printText(str, strlen(str), SELECTION_TEXT_RIGHT, SELECTION_ALT_1_HEIGHT + i*SELECTION_ALT_OFFSET, 2);
		}
	} else if(state->stage_1_state == TEAM_SELECTION_STAGE_CONTROL_2) {
		printText("OK", 2, SELECTION_TEXT_LEFT, SELECTION_ALT_1_HEIGHT + SELECTION_ALT_OFFSET, 4);
		printText("Controlled by", 13, -0.05f, SELECTION_TEAM_TEXT_HEIGHT, 3);
		printText("Pad 1", 5, SELECTION_TEXT_RIGHT, SELECTION_ALT_1_HEIGHT, 2);
		printText("Pad 2", 5, SELECTION_TEXT_RIGHT, SELECTION_ALT_1_HEIGHT + SELECTION_ALT_OFFSET, 2);
		printText("AI", 2, SELECTION_TEXT_RIGHT, SELECTION_ALT_1_HEIGHT + 2*SELECTION_ALT_OFFSET, 2);
	} else if(state->stage_1_state == TEAM_SELECTION_STAGE_INNINGS) {
		printText("select number of innings", 24, -0.45f, -0.25f, 3);
		printText("1", 1, -0.05f, SELECTION_ALT_1_HEIGHT, 3);
		printText("2", 1, -0.05f, SELECTION_ALT_1_HEIGHT + SELECTION_ALT_OFFSET, 3);
		printText("4", 1, -0.05f, SELECTION_ALT_1_HEIGHT + 2*SELECTION_ALT_OFFSET, 3);
	}
}


MenuStage updateTeamSelectionMenu(TeamSelectionState *state, const StateInfo *stateInfo, const KeyStates *keyStates)
{
	switch(state->stage_1_state) {
	case TEAM_SELECTION_STAGE_TEAM_1:
		if(keyStates->released[0][KEY_1]) {
			return MENU_STAGE_FRONT;
		}
		if(keyStates->released[0][KEY_2]) {
			state->stage_1_state = TEAM_SELECTION_STAGE_CONTROL_1;
			state->team1 = state->pointer;
			state->pointer = DEFAULT_CONTROLLED_1;
			state->rem = 3;
		}
		if(keyStates->released[0][KEY_DOWN]) {
			state->pointer +=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
		}
		if(keyStates->released[0][KEY_UP]) {
			state->pointer -=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
		}
		break;
	case TEAM_SELECTION_STAGE_CONTROL_1:
		if(keyStates->released[0][KEY_1]) {
			state->stage_1_state = TEAM_SELECTION_STAGE_TEAM_1;
			state->pointer = state->team1;
			state->rem = stateInfo->numTeams;

		}
		if(keyStates->released[0][KEY_2]) {
			state->stage_1_state = TEAM_SELECTION_STAGE_TEAM_2;
			state->team1_controller = state->pointer;
			state->pointer = DEFAULT_TEAM_2;
			state->rem = stateInfo->numTeams;
		}
		if(keyStates->released[0][KEY_DOWN]) {
			state->pointer +=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
		}
		if(keyStates->released[0][KEY_UP]) {
			state->pointer -=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
		}
		break;
	case TEAM_SELECTION_STAGE_TEAM_2:
		if(keyStates->released[0][KEY_1]) {
			state->stage_1_state = TEAM_SELECTION_STAGE_CONTROL_1;
			state->rem = 3;
			state->pointer = state->team1_controller;

		}
		if(keyStates->released[0][KEY_2]) {
			state->stage_1_state = TEAM_SELECTION_STAGE_CONTROL_2;
			state->team2 = state->pointer;
			state->rem = 3;
			state->pointer = DEFAULT_CONTROLLED_2;
			if(state->pointer == state->team1_controller) {
				state->pointer++;
				state->pointer = (state->pointer+state->rem)%state->rem;
			}
		}
		if(keyStates->released[0][KEY_DOWN]) {
			state->pointer +=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
		}
		if(keyStates->released[0][KEY_UP]) {
			state->pointer -=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
		}
		break;
	case TEAM_SELECTION_STAGE_CONTROL_2:
		if(keyStates->released[0][KEY_1]) {
			state->stage_1_state = TEAM_SELECTION_STAGE_TEAM_2;
			state->rem = stateInfo->numTeams;
			state->pointer = state->team2;

		}
		if(keyStates->released[0][KEY_2]) {
			state->stage_1_state = TEAM_SELECTION_STAGE_INNINGS;
			state->team2_controller = state->pointer;
			state->pointer = 1;
			state->rem = 3;
		}
		if(keyStates->released[0][KEY_DOWN]) {
			state->pointer +=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
			if(state->pointer == state->team1_controller) {
				state->pointer++;
				state->pointer = (state->pointer+state->rem)%state->rem;
			}
		}
		if(keyStates->released[0][KEY_UP]) {
			state->pointer -=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
			if(state->pointer == state->team1_controller) {
				state->pointer--;
				state->pointer = (state->pointer+state->rem)%state->rem;
			}
		}
		break;
	case TEAM_SELECTION_STAGE_INNINGS:
		if(keyStates->released[0][KEY_1]) {
			state->stage_1_state = TEAM_SELECTION_STAGE_CONTROL_2;
			state->rem = 3;
			state->pointer = state->team2_controller;
			if(state->pointer == state->team1_controller) {
				state->pointer++;
				state->pointer = (state->pointer+state->rem)%state->rem;
			}
		}
		if(keyStates->released[0][KEY_2]) {
			if(state->pointer == 0) state->innings = 2;
			else if(state->pointer == 1) state->innings = 4;
			else if(state->pointer == 2) state->innings = 8;

			return MENU_STAGE_BATTING_ORDER_1;
		}
		if(keyStates->released[0][KEY_DOWN]) {
			state->pointer +=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
		}
		if(keyStates->released[0][KEY_UP]) {
			state->pointer -=1;
			state->pointer = (state->pointer+state->rem)%state->rem;
		}
		break;
	}
	return MENU_STAGE_TEAM_SELECTION; // Stay in this stage by default
}

void drawTeamSelectionMenu(const TeamSelectionState *state, const StateInfo *stateInfo, const struct MenuData *menuData)
{
	drawFontBackground();

	glBindTexture(GL_TEXTURE_2D, menuData->arrowTexture);
	glPushMatrix();
	if(state->stage_1_state == TEAM_SELECTION_STAGE_TEAM_1 || state->stage_1_state == TEAM_SELECTION_STAGE_CONTROL_1) {
		glTranslatef(SELECTION_ARROW_LEFT, 1.0f, SELECTION_ALT_1_HEIGHT + SELECTION_ALT_OFFSET*state->pointer);
	} else if(state->stage_1_state == TEAM_SELECTION_STAGE_TEAM_2 || state->stage_1_state == TEAM_SELECTION_STAGE_CONTROL_2) {
		glTranslatef(SELECTION_ARROW_RIGHT, 1.0f, SELECTION_ALT_1_HEIGHT + SELECTION_ALT_OFFSET*state->pointer);
	} else if(state->stage_1_state == TEAM_SELECTION_STAGE_INNINGS) {
		glTranslatef(SELECTION_ARROW_RIGHT, 1.0f, SELECTION_ALT_1_HEIGHT + SELECTION_ALT_OFFSET*state->pointer);
	}
	glScalef(ARROW_SCALE, ARROW_SCALE, ARROW_SCALE);
	glCallList(menuData->planeDisplayList);
	glPopMatrix();
	drawSelection(state, stateInfo);
}
