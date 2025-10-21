#include "batting_order_menu.h"
#include "render.h"
#include "font.h"
#include "main_menu.h"
#include "globals.h"

#define ARROW_SCALE 0.05f
#define ARROW_SMALLER_SCALE 0.03f
#define PLAYER_LIST_ARROW_CONTINUE_POS 0.35f
#define PLAYER_LIST_ARROW_POS 0.42f

#define PLAYER_LIST_TEAM_TEXT_HEIGHT -0.4f
#define PLAYER_LIST_TEAM_TEXT_POS -0.1f
#define PLAYER_LIST_INFO_HEIGHT -0.3f
#define PLAYER_LIST_INFO_OFFSET 0.22f
#define PLAYER_LIST_INFO_NAME_OFFSET 0.1f
#define PLAYER_LIST_INFO_FIRST -0.4f
#define PLAYER_LIST_FIRST_PLAYER_HEIGHT -0.2f
#define PLAYER_LIST_PLAYER_OFFSET 0.05f
#define PLAYER_LIST_NUMBER_OFFSET 0.1f
#define PLAYER_LIST_CONTINUE_HEIGHT 0.45f

static void drawPlayerListForBattingOrder(const BattingOrderState *state, const StateInfo *stateInfo)
{
	const TeamData* teamData = stateInfo->teamData;
	int i;
	int team = state->team_index;

	printText("name", 4, PLAYER_LIST_INFO_FIRST, PLAYER_LIST_INFO_HEIGHT, 2);
	printText("speed", 5, PLAYER_LIST_INFO_FIRST + PLAYER_LIST_INFO_NAME_OFFSET + PLAYER_LIST_INFO_OFFSET, PLAYER_LIST_INFO_HEIGHT, 2);
	printText("power", 5, PLAYER_LIST_INFO_FIRST + PLAYER_LIST_INFO_NAME_OFFSET + 2*PLAYER_LIST_INFO_OFFSET, PLAYER_LIST_INFO_HEIGHT, 2);

	printText("change batting order", 20, -0.35f, -0.5f, 3);

	// This is a bit of a hack, we should probably pass the team number in.
	if(team == 0) printText("team 1", 6, PLAYER_LIST_TEAM_TEXT_POS, PLAYER_LIST_TEAM_TEXT_HEIGHT, 2);
	else printText("team 2", 6, PLAYER_LIST_TEAM_TEXT_POS, PLAYER_LIST_TEAM_TEXT_HEIGHT, 2);

	printText("continue", 8, -0.05f, PLAYER_LIST_CONTINUE_HEIGHT, 3);

	for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		char str[2];
		const char* str2;
		int index;

		if(i < PLAYERS_IN_TEAM) {
			str[0] = (char)(((int)'0')+i+1);
		} else {
			str[0] = 'J';
		}
		index = state->batting_order[i];

		printText(str, 1, PLAYER_LIST_INFO_FIRST - PLAYER_LIST_NUMBER_OFFSET,
		          PLAYER_LIST_FIRST_PLAYER_HEIGHT + i*PLAYER_LIST_PLAYER_OFFSET, 2);

		str2  = teamData[team].players[index].name;
		printText(str2, strlen(str2), PLAYER_LIST_INFO_FIRST,
		          PLAYER_LIST_FIRST_PLAYER_HEIGHT + i*PLAYER_LIST_PLAYER_OFFSET, 2);

		str[0] = (char)(((int)'0')+teamData[team].players[index].speed);
		printText(str, 1, PLAYER_LIST_INFO_FIRST + PLAYER_LIST_INFO_OFFSET + PLAYER_LIST_INFO_NAME_OFFSET,
		          PLAYER_LIST_FIRST_PLAYER_HEIGHT + i*PLAYER_LIST_PLAYER_OFFSET, 2);

		str[0] = (char)(((int)'0')+teamData[team].players[index].power);
		printText(str, 1, PLAYER_LIST_INFO_FIRST + 2*PLAYER_LIST_INFO_OFFSET + PLAYER_LIST_INFO_NAME_OFFSET,
		          PLAYER_LIST_FIRST_PLAYER_HEIGHT + i*PLAYER_LIST_PLAYER_OFFSET, 2);
	}
}

void initBattingOrderState(BattingOrderState *state, int team_index, int player_control)
{
	state->pointer = 0;
	state->rem = 13; // 12 players + "Continue"
	state->mark = 0;
	state->team_index = team_index;
	state->player_control = player_control;
	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		state->batting_order[i] = i;
	}
}

MenuStage updateBattingOrderMenu(BattingOrderState *state, const KeyStates *keyStates, MenuStage currentStage, MenuMode menuMode)
{
	if (state->player_control == 2) { // AI control
		if (currentStage == MENU_STAGE_BATTING_ORDER_1) {
			return MENU_STAGE_BATTING_ORDER_2;
		} else { // Batting order 2
			if (menuMode == MENU_ENTRY_NORMAL || menuMode == MENU_ENTRY_SUPER_INNING) {
				return MENU_STAGE_HUTUNKEITTO;
			} else { // MENU_ENTRY_INTER_PERIOD
				return MENU_STAGE_GO_TO_GAME;
			}
		}
	}

	if (keyStates->released[state->player_control][KEY_2]) {
		if (state->pointer == 0) { // "Continue"
			if (currentStage == MENU_STAGE_BATTING_ORDER_1) {
				return MENU_STAGE_BATTING_ORDER_2;
			} else { // Batting order 2
				if (menuMode == MENU_ENTRY_NORMAL || menuMode == MENU_ENTRY_SUPER_INNING) {
					return MENU_STAGE_HUTUNKEITTO;
				} else { // MENU_ENTRY_INTER_PERIOD
					return MENU_STAGE_GO_TO_GAME;
				}
			}
		} else {
			if (state->mark == 0) {
				state->mark = state->pointer;
			} else {
				int temp = state->batting_order[state->pointer - 1];
				state->batting_order[state->pointer - 1] = state->batting_order[state->mark - 1];
				state->batting_order[state->mark - 1] = temp;
				state->mark = 0;
			}
		}
	}

	if (keyStates->released[state->player_control][KEY_DOWN]) {
		state->pointer = (state->pointer + 1) % state->rem;
	}
	if (keyStates->released[state->player_control][KEY_UP]) {
		state->pointer = (state->pointer + state->rem - 1) % state->rem;
	}

	return currentStage; // Stay in the current stage by default
}

void drawBattingOrderMenu(const BattingOrderState *state, const StateInfo *stateInfo, const struct MenuData *menuData)
{
	if (state->player_control == 2) { // AI, so nothing is drawn
		return;
	}

	drawFontBackground();
	glBindTexture(GL_TEXTURE_2D, menuData->arrowTexture);
	glPushMatrix();
	if(state->pointer == 0) {
		glTranslatef(PLAYER_LIST_ARROW_CONTINUE_POS, 1.0f, PLAYER_LIST_CONTINUE_HEIGHT);
		glScalef(ARROW_SCALE, ARROW_SCALE, ARROW_SCALE);
	} else {
		glTranslatef(PLAYER_LIST_ARROW_POS, 1.0f, PLAYER_LIST_FIRST_PLAYER_HEIGHT + state->pointer*PLAYER_LIST_PLAYER_OFFSET - PLAYER_LIST_PLAYER_OFFSET);
		glScalef(ARROW_SMALLER_SCALE, ARROW_SMALLER_SCALE,ARROW_SMALLER_SCALE);
	}
	glCallList(menuData->planeDisplayList);
	glPopMatrix();

	if(state->mark != 0) {
		glBindTexture(GL_TEXTURE_2D, menuData->arrowTexture);
		glPushMatrix();
		glTranslatef(PLAYER_LIST_ARROW_POS, 1.0f, PLAYER_LIST_FIRST_PLAYER_HEIGHT + state->mark*PLAYER_LIST_PLAYER_OFFSET - PLAYER_LIST_PLAYER_OFFSET);
		glScalef(ARROW_SMALLER_SCALE, ARROW_SMALLER_SCALE, ARROW_SMALLER_SCALE);
		glCallList(menuData->planeDisplayList);
		glPopMatrix();
	}
	drawPlayerListForBattingOrder(state, stateInfo);
}
