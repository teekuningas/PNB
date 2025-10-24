#include "cup_menu.h"
#include "font.h"
#include "render.h"
#include "menu_helpers.h"
#include "save.h"

#define SELECTION_CUP_ALT_1_HEIGHT -0.15f
#define SELECTION_CUP_LEFT -0.3f
#define SELECTION_CUP_ARROW_LEFT 0.2f
#define SELECTION_CUP_MENU_OFFSET 0.1f
#define SELECTION_ALT_OFFSET 0.06f


static int refreshLoadCups(StateInfo* stateInfo);
static void saveCup(StateInfo* stateInfo, int slot);
static void loadCup(StateInfo* stateInfo, int slot);

static int refreshLoadCups(StateInfo* stateInfo)
{
	// load save slots into menuData->saveData
	readSaveData(stateInfo->tournamentState->saveData, 5);

	int i, j;
	int valid = 1;
	// go through the saveData-structure and figure out if its good.
	for(i = 0; i < 5; i++) {
		if(stateInfo->tournamentState->saveData[i].userTeamIndexInTree != -1) {
			if(stateInfo->tournamentState->saveData[i].dayCount < 0) valid = 0;
			if(stateInfo->tournamentState->saveData[i].gameStructure != 0 && stateInfo->tournamentState->saveData[i].gameStructure != 1) valid = 0;
			if(stateInfo->tournamentState->saveData[i].inningCount != 2 && stateInfo->tournamentState->saveData[i].inningCount != 4 && stateInfo->tournamentState->saveData[i].inningCount != 8) valid = 0;
			if(stateInfo->tournamentState->saveData[i].winnerIndex >= stateInfo->numTeams) valid = 0;
			if(stateInfo->tournamentState->saveData[i].userTeamIndexInTree >= 14) valid = 0;
			for(j = 0; j < SLOT_COUNT; j++) {
				if(stateInfo->tournamentState->saveData[i].slotWins[j] < 0 || stateInfo->tournamentState->saveData[i].slotWins[j] > 3) valid = 0;
				if(stateInfo->tournamentState->saveData[i].cupTeamIndexTree[j] > stateInfo->numTeams) valid = 0;
			}
		}
	}
	if(valid == 0) {
		printf("Something wrong with the save file.\n");
		return 1;
	}
	return 0;
}

static void saveCup(StateInfo* stateInfo, int slot)
{
	// write current cup state into save slot
	writeSaveData(stateInfo->tournamentState->saveData, &stateInfo->tournamentState->cupInfo, slot, 5);

	// Refresh
	int result = refreshLoadCups(stateInfo);
	if (result != 0) {
		printf("Something wrong with the save file.\n");
	}
}

static void loadCup(StateInfo* stateInfo, int slot)
{
	int i;
	stateInfo->tournamentState->cupInfo.inningCount = stateInfo->tournamentState->saveData[slot].inningCount;
	stateInfo->tournamentState->cupInfo.gameStructure = stateInfo->tournamentState->saveData[slot].gameStructure;
	stateInfo->tournamentState->cupInfo.userTeamIndexInTree = stateInfo->tournamentState->saveData[slot].userTeamIndexInTree;
	stateInfo->tournamentState->cupInfo.dayCount = stateInfo->tournamentState->saveData[slot].dayCount;

	for(i = 0; i < SLOT_COUNT; i++) {
		stateInfo->tournamentState->cupInfo.cupTeamIndexTree[i] = stateInfo->tournamentState->saveData[slot].cupTeamIndexTree[i];
		stateInfo->tournamentState->cupInfo.slotWins[i] = stateInfo->tournamentState->saveData[slot].slotWins[i];
	}
	stateInfo->tournamentState->cupInfo.winnerIndex = -1;
	if(stateInfo->tournamentState->cupInfo.gameStructure == 1) {
		if(stateInfo->tournamentState->cupInfo.slotWins[12] == 1) {
			stateInfo->tournamentState->cupInfo.winnerIndex = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[12];
		} else if(stateInfo->tournamentState->cupInfo.slotWins[13] == 1) {
			stateInfo->tournamentState->cupInfo.winnerIndex = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[13];
		}
	} else if(stateInfo->tournamentState->cupInfo.gameStructure == 0) {
		if(stateInfo->tournamentState->cupInfo.slotWins[12] == 3) {
			stateInfo->tournamentState->cupInfo.winnerIndex = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[12];
		} else if(stateInfo->tournamentState->cupInfo.slotWins[13] == 3) {
			stateInfo->tournamentState->cupInfo.winnerIndex = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[13];
		}
	}

	// here we should fill the schedule array

	updateSchedule(stateInfo->tournamentState, stateInfo);

}

void initCupMenu(CupMenuState* cupMenuState, StateInfo* stateInfo)
{
	cupMenuState->screen = CUP_MENU_SCREEN_INITIAL;
	cupMenuState->pointer = 0;
	cupMenuState->rem = 2;
	cupMenuState->new_cup_stage = NEW_CUP_STAGE_TEAM_SELECTION;
	cupMenuState->team_selection = 0;

	// set locations for cup tree view.
	cupMenuState->treeCoordinates[0].x = -0.65f;
	cupMenuState->treeCoordinates[0].y = -0.45f;
	cupMenuState->treeCoordinates[1].x = -0.65f;
	cupMenuState->treeCoordinates[1].y = -0.15f;
	cupMenuState->treeCoordinates[2].x = -0.65f;
	cupMenuState->treeCoordinates[2].y =  0.15f;
	cupMenuState->treeCoordinates[3].x = -0.65f;
	cupMenuState->treeCoordinates[3].y =  0.45f;
	cupMenuState->treeCoordinates[4].x =  0.65f;
	cupMenuState->treeCoordinates[4].y = -0.45f;
	cupMenuState->treeCoordinates[5].x =  0.65f;
	cupMenuState->treeCoordinates[5].y = -0.15f;
	cupMenuState->treeCoordinates[6].x =  0.65f;
	cupMenuState->treeCoordinates[6].y =  0.15f;
	cupMenuState->treeCoordinates[7].x =  0.65f;
	cupMenuState->treeCoordinates[7].y =  0.45f;
	cupMenuState->treeCoordinates[8].x = -0.45f;
	cupMenuState->treeCoordinates[8].y = -0.3f;
	cupMenuState->treeCoordinates[9].x = -0.45f;
	cupMenuState->treeCoordinates[9].y =  0.3f;
	cupMenuState->treeCoordinates[10].x =  0.45f;
	cupMenuState->treeCoordinates[10].y = -0.3f;
	cupMenuState->treeCoordinates[11].x =  0.45f;
	cupMenuState->treeCoordinates[11].y =  0.3f;
	cupMenuState->treeCoordinates[12].x = -0.25f;
	cupMenuState->treeCoordinates[12].y = 0.0f;
	cupMenuState->treeCoordinates[13].x =  0.25f;
	cupMenuState->treeCoordinates[13].y = 0.0f;

	if(refreshLoadCups(stateInfo) != 0) {
		printf("Something wrong with the save file.\n");
	}
}
MenuStage updateCupMenu(CupMenuState* cupMenuState, StateInfo* stateInfo, struct MenuData* menuData, KeyStates* keyStates)
{
	if (cupMenuState->screen == CUP_MENU_SCREEN_INITIAL) {
		if(keyStates->released[0][KEY_DOWN]) {
			cupMenuState->pointer +=1;
			cupMenuState->pointer = (cupMenuState->pointer+cupMenuState->rem)%cupMenuState->rem;
		}
		if(keyStates->released[0][KEY_UP]) {
			cupMenuState->pointer -=1;
			cupMenuState->pointer = (cupMenuState->pointer+cupMenuState->rem)%cupMenuState->rem;
		}
		if(keyStates->released[0][KEY_2]) {
			if(cupMenuState->pointer == 0) {
				cupMenuState->screen = CUP_MENU_SCREEN_NEW_CUP;
				cupMenuState->new_cup_stage = NEW_CUP_STAGE_TEAM_SELECTION;
				cupMenuState->rem = stateInfo->numTeams;
				cupMenuState->pointer = 0;
			} else if(cupMenuState->pointer == 1) {
				cupMenuState->screen = CUP_MENU_SCREEN_LOAD_CUP;
				cupMenuState->rem = 5;
				cupMenuState->pointer = 0;
			}
		}
		if(keyStates->released[0][KEY_1]) {
			return MENU_STAGE_FRONT;
		}
	} else if (cupMenuState->screen == CUP_MENU_SCREEN_ONGOING) {
		if(keyStates->released[0][KEY_DOWN]) {
			cupMenuState->pointer +=1;
			cupMenuState->pointer = (cupMenuState->pointer+cupMenuState->rem)%cupMenuState->rem;
		}
		if(keyStates->released[0][KEY_UP]) {
			cupMenuState->pointer -=1;
			cupMenuState->pointer = (cupMenuState->pointer+cupMenuState->rem)%cupMenuState->rem;
		}
		if(keyStates->released[0][KEY_2]) {
			if(cupMenuState->pointer == 0) {
				int userTeamIndex = -1;
				int userPosition = 0;
				int opponentTeamIndex = -1;
				int i, j;
				// so here we start the game of the human player.
				// first lets find out if there is a game for human player.
				for(i = 0; i < 4; i++) {
					for(j = 0; j < 2; j++) {
						if(stateInfo->tournamentState->cupInfo.schedule[i][j] == stateInfo->tournamentState->cupInfo.userTeamIndexInTree) {
							if(j == 0) {
								userTeamIndex = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[stateInfo->tournamentState->cupInfo.schedule[i][0]];
								opponentTeamIndex = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[stateInfo->tournamentState->cupInfo.schedule[i][1]];
								userPosition = 0;
							} else {
								userTeamIndex = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[stateInfo->tournamentState->cupInfo.schedule[i][1]];
								opponentTeamIndex = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[stateInfo->tournamentState->cupInfo.schedule[i][0]];
								userPosition = 1;
							}
						}
					}
				}
				stateInfo->tournamentState->cupInfo.dayCount++;
				// if there is, we proceed to the match and let the match ending update cup trees and schedules.
				if(userTeamIndex != -1) {
					menuData->stage = MENU_STAGE_BATTING_ORDER_1;
					menuData->pointer = 0;
					menuData->rem = 13;
					if(userPosition == 0) {
						menuData->team1 = userTeamIndex;
						menuData->team2 = opponentTeamIndex;
						menuData->team1_control = 0;
						menuData->team2_control = 2;
					} else {
						menuData->team2 = userTeamIndex;
						menuData->team1 = opponentTeamIndex;
						menuData->team2_control = 0;
						menuData->team1_control = 2;
					}
					menuData->inningsInPeriod = stateInfo->tournamentState->cupInfo.inningCount;
					stateInfo->globalGameInfo->isCupGame = 1;
					initBattingOrderState(&menuData->batting_order, menuData->team1, menuData->team1_control);
					return MENU_STAGE_BATTING_ORDER_1;
				} else {
					// otherwise we update them right away.
					updateCupTreeAfterDay(stateInfo->tournamentState, stateInfo, -1, 0);
					updateSchedule(stateInfo->tournamentState, stateInfo);
				}
			} else if(cupMenuState->pointer == 1) {
				cupMenuState->screen = CUP_MENU_SCREEN_VIEW_SCHEDULE;
			} else if(cupMenuState->pointer == 2) {
				cupMenuState->screen = CUP_MENU_SCREEN_VIEW_TREE;
			} else if(cupMenuState->pointer == 3) {
				cupMenuState->screen = CUP_MENU_SCREEN_SAVE_CUP;
				cupMenuState->pointer = 0;
				cupMenuState->rem = 5;
			} else if(cupMenuState->pointer == 4) {
				return MENU_STAGE_FRONT;
			}
		}
	} else if (cupMenuState->screen == CUP_MENU_SCREEN_NEW_CUP) {
		if (cupMenuState->new_cup_stage == NEW_CUP_STAGE_TEAM_SELECTION) {
			if(keyStates->released[0][KEY_DOWN]) {
				cupMenuState->pointer +=1;
				cupMenuState->pointer = (cupMenuState->pointer+cupMenuState->rem)%cupMenuState->rem;
			}
			if(keyStates->released[0][KEY_UP]) {
				cupMenuState->pointer -=1;
				cupMenuState->pointer = (cupMenuState->pointer+cupMenuState->rem)%cupMenuState->rem;
			}
			if(keyStates->released[0][KEY_2]) {
				cupMenuState->new_cup_stage = NEW_CUP_STAGE_WINS_TO_ADVANCE;
				cupMenuState->team_selection = cupMenuState->pointer;
				cupMenuState->pointer = 0;
				cupMenuState->rem = 2;
			}
			if(keyStates->released[0][KEY_1]) {
				cupMenuState->screen = CUP_MENU_SCREEN_INITIAL;
				cupMenuState->pointer = 0;
				cupMenuState->rem = 2;
			}
		} else if (cupMenuState->new_cup_stage == NEW_CUP_STAGE_WINS_TO_ADVANCE) {
			if(keyStates->released[0][KEY_DOWN]) {
				cupMenuState->pointer +=1;
				cupMenuState->pointer = (cupMenuState->pointer+cupMenuState->rem)%cupMenuState->rem;
			}
			if(keyStates->released[0][KEY_UP]) {
				cupMenuState->pointer -=1;
				cupMenuState->pointer = (cupMenuState->pointer+cupMenuState->rem)%cupMenuState->rem;
			}
			if(keyStates->released[0][KEY_2]) {
				stateInfo->tournamentState->cupInfo.gameStructure = cupMenuState->pointer;
				cupMenuState->new_cup_stage = NEW_CUP_STAGE_INNINGS;
				cupMenuState->pointer = 0;
				cupMenuState->rem = 3;
			}
			if(keyStates->released[0][KEY_1]) {
				cupMenuState->new_cup_stage = NEW_CUP_STAGE_TEAM_SELECTION;
				cupMenuState->pointer = 0;
				cupMenuState->rem = stateInfo->numTeams;
			}
		} else if (cupMenuState->new_cup_stage == NEW_CUP_STAGE_INNINGS) {
			if(keyStates->released[0][KEY_DOWN]) {
				cupMenuState->pointer +=1;
				cupMenuState->pointer = (cupMenuState->pointer+cupMenuState->rem)%cupMenuState->rem;
			}
			if(keyStates->released[0][KEY_UP]) {
				cupMenuState->pointer -=1;
				cupMenuState->pointer = (cupMenuState->pointer+cupMenuState->rem)%cupMenuState->rem;
			}
			if(keyStates->released[0][KEY_2]) {
				int i;
				if(cupMenuState->pointer == 0) stateInfo->tournamentState->cupInfo.inningCount = 2;
				else if(cupMenuState->pointer == 1) stateInfo->tournamentState->cupInfo.inningCount = 4;
				else if(cupMenuState->pointer == 2) stateInfo->tournamentState->cupInfo.inningCount = 8;
				stateInfo->tournamentState->cupInfo.userTeamIndexInTree = 0;
				stateInfo->tournamentState->cupInfo.winnerIndex = -1;
				stateInfo->tournamentState->cupInfo.dayCount = 0;
				for(i = 0; i < SLOT_COUNT; i++) {
					stateInfo->tournamentState->cupInfo.cupTeamIndexTree[i] = -1;
				}
				i = 0;
				while(i < 8) {
					int random = rand()%8;
					if(stateInfo->tournamentState->cupInfo.cupTeamIndexTree[random] == -1) {
						stateInfo->tournamentState->cupInfo.cupTeamIndexTree[random] = i;
						if(i == cupMenuState->team_selection) {
							stateInfo->tournamentState->cupInfo.userTeamIndexInTree = random;
						}
						i++;
					}
				}
				for(i = 0; i < SLOT_COUNT; i++) {
					stateInfo->tournamentState->cupInfo.slotWins[i] = 0;
				}
				for(i = 0; i < 4; i++) {
					stateInfo->tournamentState->cupInfo.schedule[i][0] = i*2;
					stateInfo->tournamentState->cupInfo.schedule[i][1] = i*2+1;
				}
				cupMenuState->screen = CUP_MENU_SCREEN_ONGOING;
				cupMenuState->pointer = 0;
				cupMenuState->rem = 5;
			}
			if(keyStates->released[0][KEY_1]) {
				cupMenuState->new_cup_stage = NEW_CUP_STAGE_WINS_TO_ADVANCE;
				cupMenuState->pointer = 0;
				cupMenuState->rem = 2;
			}
		}
	} else if (cupMenuState->screen == CUP_MENU_SCREEN_LOAD_CUP) {
		if(keyStates->released[0][KEY_DOWN]) {
			cupMenuState->pointer +=1;
			cupMenuState->pointer = (cupMenuState->pointer+cupMenuState->rem)%cupMenuState->rem;
		}
		if(keyStates->released[0][KEY_UP]) {
			cupMenuState->pointer -=1;
			cupMenuState->pointer = (cupMenuState->pointer+cupMenuState->rem)%cupMenuState->rem;
		}
		if(keyStates->released[0][KEY_1]) {
			cupMenuState->screen = CUP_MENU_SCREEN_INITIAL;
			cupMenuState->pointer = 1;
			cupMenuState->rem = 2;
		}
		if (keyStates->released[0][KEY_2]) {
			if (stateInfo->tournamentState->saveData[cupMenuState->pointer].userTeamIndexInTree != -1) {
				loadCup(stateInfo, cupMenuState->pointer);
				cupMenuState->screen = CUP_MENU_SCREEN_ONGOING;
				cupMenuState->pointer = 0;
				cupMenuState->rem = 5;
			}
		}
	} else if (cupMenuState->screen == CUP_MENU_SCREEN_SAVE_CUP) {
		if(keyStates->released[0][KEY_DOWN]) {
			cupMenuState->pointer +=1;
			cupMenuState->pointer = (cupMenuState->pointer+cupMenuState->rem)%cupMenuState->rem;
		}
		if(keyStates->released[0][KEY_UP]) {
			cupMenuState->pointer -=1;
			cupMenuState->pointer = (cupMenuState->pointer+cupMenuState->rem)%cupMenuState->rem;
		}
		if(keyStates->released[0][KEY_1]) {
			cupMenuState->screen = CUP_MENU_SCREEN_ONGOING;
			cupMenuState->pointer = 2;
			cupMenuState->rem = 5;
		}
		if (keyStates->released[0][KEY_2]) {
			saveCup(stateInfo, cupMenuState->pointer);
		}
	} else if (cupMenuState->screen == CUP_MENU_SCREEN_VIEW_SCHEDULE) {
		if(keyStates->released[0][KEY_2]) {
			cupMenuState->screen = CUP_MENU_SCREEN_ONGOING;
		}
		if(keyStates->released[0][KEY_1]) {
			cupMenuState->screen = CUP_MENU_SCREEN_ONGOING;
		}
	} else if (cupMenuState->screen == CUP_MENU_SCREEN_VIEW_TREE) {
		if(keyStates->released[0][KEY_2]) {
			cupMenuState->screen = CUP_MENU_SCREEN_ONGOING;
		}
		if(keyStates->released[0][KEY_1]) {
			cupMenuState->screen = CUP_MENU_SCREEN_ONGOING;
		}
	} else if (cupMenuState->screen == CUP_MENU_SCREEN_END_CREDITS) {
		if(keyStates->released[0][KEY_2]) {
			cupMenuState->screen = CUP_MENU_SCREEN_ONGOING;
		}
		if(keyStates->released[0][KEY_1]) {
			cupMenuState->screen = CUP_MENU_SCREEN_ONGOING;
		}
	}
	return MENU_STAGE_CUP;
}

static void drawCupMenuInternal(const CupMenuState* cupMenuState, const StateInfo* stateInfo, const struct MenuData* menuData);

void drawCupMenu(const CupMenuState* cupMenuState, const StateInfo* stateInfo, const struct MenuData* menuData)
{
	drawFontBackground();
	if (cupMenuState->screen == CUP_MENU_SCREEN_ONGOING) {
		glBindTexture(GL_TEXTURE_2D, menuData->arrowTexture);
		glPushMatrix();
		glTranslatef(SELECTION_CUP_ARROW_LEFT, 1.0f, SELECTION_CUP_ALT_1_HEIGHT + SELECTION_CUP_MENU_OFFSET*cupMenuState->pointer);
		glScalef(0.05f, 0.05f, 0.05f);
		glCallList(menuData->planeDisplayList);
		glPopMatrix();
	} else if (cupMenuState->screen == CUP_MENU_SCREEN_INITIAL) {
		// arrow
		glBindTexture(GL_TEXTURE_2D, menuData->arrowTexture);
		glPushMatrix();
		if (cupMenuState->pointer == 0) glTranslatef(0.15f + 0.05f, 1.0f, -0.1f);
		else if (cupMenuState->pointer == 1) glTranslatef(0.15f + 0.05f, 1.0f, 0.0f);
		glScalef(0.05f, 0.05f, 0.05f);
		glCallList(menuData->planeDisplayList);
		glPopMatrix();
		// catcher
		glBindTexture(GL_TEXTURE_2D, menuData->catcherTexture);
		glPushMatrix();
		glTranslatef(0.7f, 1.0f, 0.0f);
		glScalef(0.4f, 0.4f, 0.4f);
		glCallList(menuData->planeDisplayList);
		glPopMatrix();
		// batter
		glBindTexture(GL_TEXTURE_2D, menuData->batterTexture);
		glPushMatrix();
		glTranslatef(-0.6f, 1.0f, 0.0f);
		glScalef(0.4f/2, 0.4f, 0.4f);
		glCallList(menuData->planeDisplayList);
		glPopMatrix();
	} else if (cupMenuState->screen == CUP_MENU_SCREEN_NEW_CUP) {
		glBindTexture(GL_TEXTURE_2D, menuData->arrowTexture);
		glPushMatrix();
		glTranslatef(SELECTION_CUP_ARROW_LEFT, 1.0f, SELECTION_CUP_ALT_1_HEIGHT + SELECTION_ALT_OFFSET*cupMenuState->pointer);
		glScalef(0.05f, 0.05f, 0.05f);
		glCallList(menuData->planeDisplayList);
		glPopMatrix();
	} else if (cupMenuState->screen == CUP_MENU_SCREEN_VIEW_TREE) {
		int i;
		for(i = 0; i < SLOT_COUNT; i++) {
			glBindTexture(GL_TEXTURE_2D, menuData->slotTexture);
			glPushMatrix();
			glTranslatef(cupMenuState->treeCoordinates[i].x, 1.0f, cupMenuState->treeCoordinates[i].y);
			glScalef(0.2f, 0.15f, 0.10f);
			glCallList(menuData->planeDisplayList);
			glPopMatrix();
		}
	} else if (cupMenuState->screen == CUP_MENU_SCREEN_LOAD_CUP || cupMenuState->screen == CUP_MENU_SCREEN_SAVE_CUP) {
		glBindTexture(GL_TEXTURE_2D, menuData->arrowTexture);
		glPushMatrix();
		glTranslatef(SELECTION_CUP_ARROW_LEFT, 1.0f, SELECTION_CUP_ALT_1_HEIGHT + SELECTION_CUP_MENU_OFFSET*cupMenuState->pointer);
		glScalef(0.05f, 0.05f, 0.05f);
		glCallList(menuData->planeDisplayList);
		glPopMatrix();
	} else if (cupMenuState->screen == CUP_MENU_SCREEN_END_CREDITS) {
		glBindTexture(GL_TEXTURE_2D, menuData->trophyTexture);
		glPushMatrix();
		glTranslatef(0.0f, 1.0f, -0.25f);
		glScalef(0.3f, 0.3f, 0.3f);
		glCallList(menuData->planeDisplayList);
		glPopMatrix();
	}
	drawCupMenuInternal(cupMenuState, stateInfo, menuData);
}

static void drawCupMenuInternal(const CupMenuState* cupMenuState, const StateInfo* stateInfo, const struct MenuData* menuData)
{
	if (cupMenuState->screen == CUP_MENU_SCREEN_ONGOING) {
		printText("Cup menu", 8, -0.22f, -0.4f, 5);
		printText("Next day", 8, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT, 2);
		printText("Schedule", 8, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT + SELECTION_CUP_MENU_OFFSET, 2);
		printText("Cup tree", 8, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT + 2*SELECTION_CUP_MENU_OFFSET, 2);
		printText("Save", 4, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT + 3*SELECTION_CUP_MENU_OFFSET, 2);
		printText("Quit", 4, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT + 4*SELECTION_CUP_MENU_OFFSET, 2);
		if(stateInfo->tournamentState->cupInfo.winnerIndex != -1) {
			char* str = stateInfo->teamData[stateInfo->tournamentState->cupInfo.winnerIndex].name;
			printText(str, strlen(str), -0.45f, SELECTION_CUP_ALT_1_HEIGHT + 6*SELECTION_CUP_MENU_OFFSET, 3);
			printText("has won the cup", 15, -0.45f + strlen(str)*0.04f, SELECTION_CUP_ALT_1_HEIGHT + 6*SELECTION_CUP_MENU_OFFSET, 3);
		}
	} else if (cupMenuState->screen == CUP_MENU_SCREEN_INITIAL) {
		printText("P N B", 5, -0.23f, -0.4f, 8);
		printText("New cup", 7, -0.16f, -0.1f, 3);
		printText("Load cup", 8, -0.18f, 0.0f, 3);
	} else if (cupMenuState->screen == CUP_MENU_SCREEN_NEW_CUP) {
		if (cupMenuState->new_cup_stage == NEW_CUP_STAGE_TEAM_SELECTION) {
			int i;
			printText("Select team", 11, -0.35f, -0.4f, 5);
			for(i = 0; i < stateInfo->numTeams; i++) {
				char* str = stateInfo->teamData[i].name;
				printText(str, strlen(str), SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT + i*SELECTION_ALT_OFFSET, 2);
			}
		} else if (cupMenuState->new_cup_stage == NEW_CUP_STAGE_WINS_TO_ADVANCE) {
			printText("How many wins to move forward", 29, -0.5f, -0.4f, 3);
			printText("Normal", 6, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT, 2);
			printText("One", 3, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT + SELECTION_ALT_OFFSET, 2);
		} else if (cupMenuState->new_cup_stage == NEW_CUP_STAGE_INNINGS) {
			printText("How many innings in period", 26, -0.5f, -0.4f, 3);
			printText("1", 1, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT, 2);
			printText("2", 1, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT + SELECTION_ALT_OFFSET, 2);
			printText("4", 1, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT + 2*SELECTION_ALT_OFFSET, 2);
		}
	} else if (cupMenuState->screen == CUP_MENU_SCREEN_VIEW_TREE) {
		int i;
		for(i = 0; i < SLOT_COUNT; i++) {
			int index = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[i];
			if(index != -1) {
				char* str = stateInfo->teamData[index].name;
				char wins[2] = " ";
				wins[0] = (char)('0' + stateInfo->tournamentState->cupInfo.slotWins[i]);
				printText(str, strlen(str), cupMenuState->treeCoordinates[i].x - 0.15f, cupMenuState->treeCoordinates[i].y, 2);
				printText(wins, 1, cupMenuState->treeCoordinates[i].x + 0.25f, cupMenuState->treeCoordinates[i].y, 3);
			}
		}
	} else if (cupMenuState->screen == CUP_MENU_SCREEN_VIEW_SCHEDULE) {
		int i;
		int counter = 0;
		int index1, index2;
		printText("schedule", 8, -0.15f, -0.35f, 3);
		for(i = 0; i < 4; i++) {
			index1 = -1;
			index2 = -1;
			if(stateInfo->tournamentState->cupInfo.schedule[i][0] != -1)
				index1 = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[stateInfo->tournamentState->cupInfo.schedule[i][0]];
			if(stateInfo->tournamentState->cupInfo.schedule[i][1] != -1)
				index2 = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[stateInfo->tournamentState->cupInfo.schedule[i][1]];
			if(index1 != -1 && index2 != -1) {
				char* str = stateInfo->teamData[index1].name;
				char* str2 = stateInfo->teamData[index2].name;
				printText(str, strlen(str), -0.4f, -0.15f + counter*0.1f, 2);
				printText("-", 1, -0.02f, -0.15f + counter*0.1f, 2);
				printText(str2, strlen(str2), 0.1f, -0.15f + counter*0.1f, 2);
				counter++;
			}
		}
	} else if (cupMenuState->screen == CUP_MENU_SCREEN_END_CREDITS) {
		printText("there you go champ", 18, -0.4f, -0.45f, 4);
		printText("so you beat the game huh", 24, -0.28f, 0.1f, 2);
		printText("the mighty conqueror", 20, -0.23f, 0.15f, 2);
		printText("anyway thank you for playing", 28, -0.3f, 0.2f, 2);
		printText("made me happy", 13, -0.14f, 0.25f, 2);
		printText("special thanks to", 17, -0.21f, 0.33f, 2);
		printText("petri anttila juuso heinila matti pitkanen", 42, -0.5f, 0.38f, 2);
		printText("ville viljanmaa petri mikola pekka heinila", 43, -0.5f, 0.43f, 2);
		printText("for supporting the development in various ways", 46, -0.53f, 0.48f, 2);

	} else if (cupMenuState->screen == CUP_MENU_SCREEN_LOAD_CUP || cupMenuState->screen == CUP_MENU_SCREEN_SAVE_CUP) {
		int i;
		if(cupMenuState->screen == CUP_MENU_SCREEN_LOAD_CUP) {
			printText("Load cup", 8, -0.22f, -0.4f, 5);
		} else {
			printText("Save cup", 8, -0.22f, -0.4f, 5);
		}
		for(i = 0; i < 5; i++) {
			char* text;
			char* empty = "Empty slot";
			int index = stateInfo->tournamentState->saveData[i].userTeamIndexInTree;
			if(index != -1) {
				text = stateInfo->teamData[stateInfo->tournamentState->saveData[i].cupTeamIndexTree[index]].name;
			} else {
				text = empty;
			}
			printText(text, strlen(text), SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT + i*SELECTION_CUP_MENU_OFFSET, 2);
		}
	}
}
