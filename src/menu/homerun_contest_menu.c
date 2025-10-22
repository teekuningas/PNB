// Home-run contest menu implementation
#include "homerun_contest_menu.h"
#include "globals.h"    // for PLAYERS_IN_TEAM, JOKER_COUNT
#include "render.h"
#include "font.h"
#include <string.h>

// Initialize or reset the home-run contest state
void initHomerunContestState(HomerunContestState *state,
                             int team_index,
                             int player_control,
                             int choiceCount)
{
	state->team_index = team_index;
	state->player_control = player_control;
	state->pointer = 1; // start at first player slot
	state->rem = PLAYERS_IN_TEAM + JOKER_COUNT + 1; // slots + Continue
	state->choiceCount = choiceCount;
	state->choiceCounter = 0;
	// clear previous choices
	int half = choiceCount / 2;
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < half; j++) {
			state->choices[i][j] = -1;
		}
	}
}

// Update function for a home-run contest stage
MenuStage updateHomerunContestMenu(HomerunContestState *state,
                                   const KeyStates *keyStates,
                                   const StateInfo *stateInfo,
                                   MenuStage currentStage)
{
	int half = state->choiceCount / 2;
	// AI auto-selection
	if (state->player_control == 2) {
		// first select batters by descending power
		int counter = 0, currentIndex = 0;
		int team = stateInfo->globalGameInfo->teams
		           [(currentStage == MENU_STAGE_HOMERUN_CONTEST_1 ? 0 : 1)].value - 1;
		// clear slots
		for (int i = 0; i < 2; i++)
			for (int j = 0; j < half; j++)
				state->choices[i][j] = -1;
		// batters
		while (counter < half) {
			for (int p = 0; p < PLAYERS_IN_TEAM + JOKER_COUNT; p++) {
				if (currentIndex == half) break;
				if (stateInfo->teamData[team].players[p].power == 5 - counter) {
					state->choices[0][currentIndex++] = p;
				}
			}
			if (currentIndex == half) break;
			counter++;
		}
		// runners by descending speed
		counter = 0;
		currentIndex = 0;
		while (counter < half) {
			for (int p = 0; p < PLAYERS_IN_TEAM + JOKER_COUNT; p++) {
				if (currentIndex == half) break;
				if (stateInfo->teamData[team].players[p].speed == 5 - counter) {
					int valid = 1;
					for (int k = 0; k < half; k++) {
						if (state->choices[0][k] == p) {
							valid = 0;
							break;
						}
					}
					if (valid) state->choices[1][currentIndex++] = p;
				}
			}
			if (currentIndex == half) break;
			counter++;
		}
		// advance to next stage or game
		return (currentStage == MENU_STAGE_HOMERUN_CONTEST_1)
		       ? MENU_STAGE_HOMERUN_CONTEST_2
		       : MENU_STAGE_GO_TO_GAME;
	}
	// Human selection
	// undo last pick
	if (keyStates->released[state->player_control][KEY_1]) {
		if (state->choiceCounter > 0) {
			int idx = state->choiceCounter - 1;
			int row = idx / half;
			int col = idx % half;
			state->choices[row][col] = -1;
			state->choiceCounter--;
		}
	}
	// select or continue
	if (keyStates->released[state->player_control][KEY_2]) {
		if (state->pointer == 0) {
			if (state->choiceCounter >= state->choiceCount) {
				state->choiceCounter = 0;
				return (currentStage == MENU_STAGE_HOMERUN_CONTEST_1)
				       ? MENU_STAGE_HOMERUN_CONTEST_2
				       : MENU_STAGE_GO_TO_GAME;
			}
		} else if (state->choiceCounter < state->choiceCount) {
			// validate no duplicate
			int valid = 1;
			for (int r = 0; r < 2; r++) {
				for (int c = 0; c < half; c++) {
					if (state->choices[r][c] + 1 == state->pointer) {
						valid = 0;
						break;
					}
				}
				if (!valid) break;
			}
			if (valid) {
				int idx = state->choiceCounter;
				int row = idx / half;
				int col = idx % half;
				state->choices[row][col] = state->pointer - 1;
				state->choiceCounter++;
			}
		}
	}
	// navigate cursor
	if (keyStates->released[state->player_control][KEY_DOWN]) {
		state->pointer = (state->pointer + 1) % state->rem;
	}
	if (keyStates->released[state->player_control][KEY_UP]) {
		state->pointer = (state->pointer + state->rem - 1) % state->rem;
	}
	// stay in current stage
	return currentStage;
}

// Draw function for a home-run contest stage
void drawHomerunContestMenu(const HomerunContestState *state,
                            const StateInfo *stateInfo,
                            const struct MenuData *menuData)
{
	if (state->player_control == 2) return; // nothing to draw for AI
	drawFontBackground();
	glBindTexture(GL_TEXTURE_2D, menuData->arrowTexture);
	glPushMatrix();
	if (state->pointer == 0) {
		glTranslatef(PLAYER_LIST_ARROW_CONTINUE_POS, 1.0f, PLAYER_LIST_CONTINUE_HEIGHT);
		glScalef(ARROW_SCALE, ARROW_SCALE, ARROW_SCALE);
	} else {
		glTranslatef(PLAYER_LIST_ARROW_POS, 1.0f,
		             PLAYER_LIST_FIRST_PLAYER_HEIGHT + state->pointer * PLAYER_LIST_PLAYER_OFFSET
		             - PLAYER_LIST_PLAYER_OFFSET);
		glScalef(ARROW_SMALLER_SCALE, ARROW_SMALLER_SCALE, ARROW_SMALLER_SCALE);
	}
	glCallList(menuData->planeDisplayList);
	glPopMatrix();

	// print selected numbers
	for (int r = 0; r < 2; r++) {
		for (int c = 0; c < state->choiceCount / 2; c++) {
			int choice = state->choices[r][c];
			if (choice >= 0) {
				char str[2] = { (char)('0' + c + 1), '\0' };
				printText(str, 1,
				          0.45f + r * 0.05f,
				          PLAYER_LIST_FIRST_PLAYER_HEIGHT + (choice + 1) * PLAYER_LIST_PLAYER_OFFSET
				          - PLAYER_LIST_PLAYER_OFFSET,
				          2);
			}
		}
	}
	// now draw full player list with names and stats
	// headers
	printText("name", 4,
	          PLAYER_LIST_INFO_FIRST,
	          PLAYER_LIST_INFO_HEIGHT,
	          2);
	printText("speed", 5,
	          PLAYER_LIST_INFO_FIRST + PLAYER_LIST_INFO_NAME_OFFSET + PLAYER_LIST_INFO_OFFSET,
	          PLAYER_LIST_INFO_HEIGHT,
	          2);
	printText("power", 5,
	          PLAYER_LIST_INFO_FIRST + PLAYER_LIST_INFO_NAME_OFFSET + 2*PLAYER_LIST_INFO_OFFSET,
	          PLAYER_LIST_INFO_HEIGHT,
	          2);
	// instruction label
	printText("choose batters and runners", 26,
	          -0.42f, -0.5f, 3);
	// team label
	const char* teamLabel = stateInfo->teamData[state->team_index].name;
	printText(teamLabel, strlen(teamLabel),
	          PLAYER_LIST_TEAM_TEXT_POS,
	          PLAYER_LIST_TEAM_TEXT_HEIGHT,
	          2);
	// continue label
	printText("continue", 8,
	          -0.05f,
	          PLAYER_LIST_CONTINUE_HEIGHT,
	          3);
	// list all players
	int total = PLAYERS_IN_TEAM + JOKER_COUNT;
	for (int p = 0; p < total; p++) {
		// number or joker
		char num[2];
		num[1] = '\0';
		if (p < PLAYERS_IN_TEAM) num[0] = (char)('0' + p + 1);
		else num[0] = 'J';
		printText(num, 1,
		          PLAYER_LIST_INFO_FIRST - PLAYER_LIST_NUMBER_OFFSET,
		          PLAYER_LIST_FIRST_PLAYER_HEIGHT + p * PLAYER_LIST_PLAYER_OFFSET,
		          2);
		// name
		const char* nm = stateInfo->teamData[state->team_index].players[p].name;
		printText(nm, strlen(nm),
		          PLAYER_LIST_INFO_FIRST,
		          PLAYER_LIST_FIRST_PLAYER_HEIGHT + p * PLAYER_LIST_PLAYER_OFFSET,
		          2);
		// speed
		char sp[2] = { (char)('0' + stateInfo->teamData[state->team_index].players[p].speed), '\0' };
		printText(sp, 1,
		          PLAYER_LIST_INFO_FIRST + PLAYER_LIST_INFO_OFFSET + PLAYER_LIST_INFO_NAME_OFFSET,
		          PLAYER_LIST_FIRST_PLAYER_HEIGHT + p * PLAYER_LIST_PLAYER_OFFSET,
		          2);
		// power
		char pw[2] = { (char)('0' + stateInfo->teamData[state->team_index].players[p].power), '\0' };
		printText(pw, 1,
		          PLAYER_LIST_INFO_FIRST + 2*PLAYER_LIST_INFO_OFFSET + PLAYER_LIST_INFO_NAME_OFFSET,
		          PLAYER_LIST_FIRST_PLAYER_HEIGHT + p * PLAYER_LIST_PLAYER_OFFSET,
		          2);
	}
}
