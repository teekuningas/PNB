#include "globals.h"
#include "render.h"
#include "font.h"

#include "main_menu.h"
#include "common_logic.h"
#include "save.h"
#include "menu_types.h"
#include "team_selection_menu.h"
#include "batting_order_menu.h"
#include "hutunkeitto_menu.h"
#include "front_menu.h"
#include "game_over_menu.h"
#include "menu_helpers.h"

#define LOADING_MODELS_HEIGHT -0.15f
#define LOADING_APPRECIATED_HEIGHT 0.0f
#define LOADING_AUTHOR_HEIGHT 0.45f

#define FIGURE_SCALE 0.4f
#define FRONT_ARROW_POS 0.15f

#define HELP_LEFT -0.75f

#define NEW_CUP_TEXT_HEIGHT -0.1f
#define LOAD_CUP_TEXT_HEIGHT 0.0f
#define SELECTION_CUP_ALT_1_HEIGHT -0.15f
#define SELECTION_CUP_LEFT -0.3f
#define SELECTION_CUP_ARROW_LEFT 0.2f
#define SELECTION_CUP_MENU_OFFSET 0.1f

#define SELECTION_ALT_OFFSET 0.06f

#define BAT_DEFAULT_HEIGHT -1.0f
#define LEFT_HAND_DEFAULT_HEIGHT 0.1f
#define RIGHT_HAND_DEFAULT_HEIGHT 0.22f
#define SCALE_FACTOR 0.005f
#define POSITION_SCALE_ADDITION 0.00065f
#define BAT_MOVING_SPEED 0.0025f
#define BAT_DROP_MOVING_SPEED 0.03f
#define SCALE_LIMIT 30
#define HAND_WIDTH 0.12f
#define CAM_HEIGHT 2.3f
#define BAT_HEIGHT_CONSTANT 0.58f
#define MOVING_AWAY_SPEED 0.002f
#define HUTUNKEITTO_TEAM_TEXT_HEIGHT 0.45f
#define HUTUNKEITTO_TEAM_1_TEXT_POSITION 0.2f
#define HUTUNKEITTO_TEAM_2_TEXT_POSITION 0.55f

static MenuData menuData;

static void drawCup();
static void drawHelp();
static void moveToGame();
static void drawLoadingTexts();

typedef struct _TreeCoordinates {
	float x;
	float y;
} TreeCoordinates;

static TreeCoordinates treeCoordinates[SLOT_COUNT];

static int refreshLoadCups(StateInfo* stateInfo);
static void saveCup(StateInfo* stateInfo, int slot);
static void loadCup(StateInfo* stateInfo, int slot);

/*
	loads data from data-file and stores it to saveData-structure.
	return 0 if everything went fine, 1 otherwise
*/
static int refreshLoadCups(StateInfo* stateInfo)
{
	// load save slots into menuData.saveData
	readSaveData(menuData.saveData, 5);

	int i, j;
	int valid = 1;
	// go through the saveData-structure and figure out if its good.
	for(i = 0; i < 5; i++) {
		if(menuData.saveData[i].userTeamIndexInTree != -1) {
			if(menuData.saveData[i].dayCount < 0) valid = 0;
			if(menuData.saveData[i].gameStructure != 0 && menuData.saveData[i].gameStructure != 1) valid = 0;
			if(menuData.saveData[i].inningCount != 2 && menuData.saveData[i].inningCount != 4 && menuData.saveData[i].inningCount != 8) valid = 0;
			if(menuData.saveData[i].winnerIndex >= stateInfo->numTeams) valid = 0;
			if(menuData.saveData[i].userTeamIndexInTree >= 14) valid = 0;
			for(j = 0; j < SLOT_COUNT; j++) {
				if(menuData.saveData[i].slotWins[j] < 0 || menuData.saveData[i].slotWins[j] > 3) valid = 0;
				if(menuData.saveData[i].cupTeamIndexTree[j] > stateInfo->numTeams) valid = 0;
			}
		}
	}
	if(valid == 0) {
		printf("Something wrong with the save file.\n");
		return 1;
	}
	return 0;
}
/*
	stores information in menuData.cupInfo-structure to saveData-structure's specified slot and
	then writes saveData-info to data-file.
*/
static void saveCup(StateInfo* stateInfo, int slot)
{
	// write current cup state into save slot
	writeSaveData(menuData.saveData, &menuData.cupInfo, slot, 5);

	// Refresh
	int result = refreshLoadCups(stateInfo);
	if (result != 0) {
		printf("Something wrong with the save file.\n");
	}
}

/*
	loads data from saveData-structure to menuData.cupInfo-structure
*/
static void loadCup(StateInfo* stateInfo, int slot)
{
	int i;
	menuData.cupInfo.inningCount = menuData.saveData[slot].inningCount;
	menuData.cupInfo.gameStructure = menuData.saveData[slot].gameStructure;
	menuData.cupInfo.userTeamIndexInTree = menuData.saveData[slot].userTeamIndexInTree;
	menuData.cupInfo.dayCount = menuData.saveData[slot].dayCount;

	for(i = 0; i < SLOT_COUNT; i++) {
		menuData.cupInfo.cupTeamIndexTree[i] = menuData.saveData[slot].cupTeamIndexTree[i];
		menuData.cupInfo.slotWins[i] = menuData.saveData[slot].slotWins[i];
	}
	menuData.cupInfo.winnerIndex = -1;
	if(menuData.cupInfo.gameStructure == 1) {
		if(menuData.cupInfo.slotWins[12] == 1) {
			menuData.cupInfo.winnerIndex = menuData.cupInfo.cupTeamIndexTree[12];
		} else if(menuData.cupInfo.slotWins[13] == 1) {
			menuData.cupInfo.winnerIndex = menuData.cupInfo.cupTeamIndexTree[13];
		}
	} else if(menuData.cupInfo.gameStructure == 0) {
		if(menuData.cupInfo.slotWins[12] == 3) {
			menuData.cupInfo.winnerIndex = menuData.cupInfo.cupTeamIndexTree[12];
		} else if(menuData.cupInfo.slotWins[13] == 3) {
			menuData.cupInfo.winnerIndex = menuData.cupInfo.cupTeamIndexTree[13];
		}
	}

	// here we should fill the schedule array

	updateSchedule(&menuData, stateInfo);

}



int initMainMenu(StateInfo* stateInfo, MenuInfo* menuInfo)

{

	menuInfo->mode = MENU_ENTRY_NORMAL;
	menuData.cam.x = 0.0f;
	menuData.cam.y = CAM_HEIGHT;
	menuData.cam.z = 0.0f;
	menuData.up.x = 0.0f;
	menuData.up.y = 0.0f;
	menuData.up.z = -1.0f;
	menuData.look.x = 0.0f;
	menuData.look.y = 0.0f;
	menuData.look.z = 0.0f;
	if(tryLoadingTextureGL(&menuData.arrowTexture, "data/textures/arrow.tga", "arrow") != 0) return -1;
	if(tryLoadingTextureGL(&menuData.catcherTexture, "data/textures/catcher.tga", "catcher") != 0) return -1;
	if(tryLoadingTextureGL(&menuData.batterTexture, "data/textures/batter.tga", "batter") != 0) return -1;
	if(tryLoadingTextureGL(&menuData.slotTexture, "data/textures/cup_tree_slot.tga", "slot") != 0) return -1;
	if(tryLoadingTextureGL(&menuData.trophyTexture, "data/textures/menu_trophy.tga", "trophy") != 0) return -1;
	if(tryLoadingTextureGL(&menuData.team1Texture, "data/textures/team1.tga", "team1") != 0) return -1;
	if(tryLoadingTextureGL(&menuData.team2Texture, "data/textures/team2.tga", "team2") != 0) return -1;
	if(tryLoadingTextureGL(&menuData.team3Texture, "data/textures/team3.tga", "team3") != 0) return -1;
	if(tryLoadingTextureGL(&menuData.team4Texture, "data/textures/team4.tga", "team4") != 0) return -1;
	if(tryLoadingTextureGL(&menuData.team5Texture, "data/textures/team5.tga", "team5") != 0) return -1;
	if(tryLoadingTextureGL(&menuData.team6Texture, "data/textures/team6.tga", "team6") != 0) return -1;
	if(tryLoadingTextureGL(&menuData.team7Texture, "data/textures/team7.tga", "team7") != 0) return -1;
	if(tryLoadingTextureGL(&menuData.team8Texture, "data/textures/team8.tga", "team8") != 0) return -1;
	menuData.planeMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/plane.obj", "Plane", menuData.planeMesh, &menuData.planeDisplayList) != 0) return -1;
	menuData.batMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/hutunkeitto_bat.obj", "Sphere.001", menuData.batMesh, &menuData.batDisplayList) != 0) return -1;
	menuData.handMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/hutunkeitto_hand.obj", "Cube.001", menuData.handMesh, &menuData.handDisplayList) != 0) return -1;

	if(refreshLoadCups(stateInfo) != 0) {
		printf("Something wrong with the save file.\n");
		return 1;
	}

	// set locations for cup tree view.
	treeCoordinates[0].x = -0.65f;
	treeCoordinates[0].y = -0.45f;
	treeCoordinates[1].x = -0.65f;
	treeCoordinates[1].y = -0.15f;
	treeCoordinates[2].x = -0.65f;
	treeCoordinates[2].y =  0.15f;
	treeCoordinates[3].x = -0.65f;
	treeCoordinates[3].y =  0.45f;
	treeCoordinates[4].x =  0.65f;
	treeCoordinates[4].y = -0.45f;
	treeCoordinates[5].x =  0.65f;
	treeCoordinates[5].y = -0.15f;
	treeCoordinates[6].x =  0.65f;
	treeCoordinates[6].y =  0.15f;
	treeCoordinates[7].x =  0.65f;
	treeCoordinates[7].y =  0.45f;
	treeCoordinates[8].x = -0.45f;
	treeCoordinates[8].y = -0.3f;
	treeCoordinates[9].x = -0.45f;
	treeCoordinates[9].y =  0.3f;
	treeCoordinates[10].x =  0.45f;
	treeCoordinates[10].y = -0.3f;
	treeCoordinates[11].x =  0.45f;
	treeCoordinates[11].y =  0.3f;
	treeCoordinates[12].x = -0.25f;
	treeCoordinates[12].y = 0.0f;
	treeCoordinates[13].x =  0.25f;
	treeCoordinates[13].y = 0.0f;
	return 0;
}



void drawLoadingScreen(StateInfo* stateInfo, MenuInfo* menuInfo)
{
	resetMenuForNewGame(&menuData);
	gluLookAt(menuData.cam.x, menuData.cam.y, menuData.cam.z, menuData.look.x, menuData.look.y, menuData.look.z, menuData.up.x, menuData.up.y, menuData.up.z);
	drawFontBackground();
	drawLoadingTexts(stateInfo);
}



void updateMainMenu(StateInfo* stateInfo, MenuInfo* menuInfo, KeyStates* keyStates)
{
	MenuStage nextStage;
	if(stateInfo->changeScreen == 1) {
		stateInfo->changeScreen = 0;
		stateInfo->updated = 1;
		int i;
		switch (menuInfo->mode) {
		case MENU_ENTRY_NORMAL:
			resetMenuForNewGame(&menuData);
			break;
		case MENU_ENTRY_INTER_PERIOD:
			initBattingOrderState(&menuData.batting_order, menuData.team1, menuData.team1_control);
			menuData.stage = MENU_STAGE_BATTING_ORDER_1;
			for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
				menuData.team1_batting_order[i] = i;
				menuData.team2_batting_order[i] = i;
			}
			break;
		case MENU_ENTRY_SUPER_INNING:
			initBattingOrderState(&menuData.batting_order, menuData.team1, menuData.team1_control);
			menuData.stage = MENU_STAGE_BATTING_ORDER_1;
			for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
				menuData.team1_batting_order[i] = i;
				menuData.team2_batting_order[i] = i;
			}
			break;
		case MENU_ENTRY_HOMERUN_CONTEST:
			menuData.stage = MENU_STAGE_HOMERUN_CONTEST_1;
			menuData.pointer = 1;
			menuData.rem = 13;
			for(i = 0; i < 2; i++) {
				for(int j = 0; j < 5; j++) {
					menuData.team_1_choices[i][j] = -1;
					menuData.team_2_choices[i][j] = -1;
				}
			}
			if(stateInfo->globalGameInfo->period == 4) {
				menuData.choiceCount = 10;
			} else menuData.choiceCount = 6;
			menuData.choiceCounter = 0;
			break;
		case MENU_ENTRY_GAME_OVER:
			menuData.stage = MENU_STAGE_GAME_OVER;
			break;
		}
		if(menuInfo->mode != MENU_ENTRY_GAME_OVER) {
			stateInfo->playSoundEffect = SOUND_MENU;
		}
	}
	// main main menu.
	switch(menuData.stage) {
	case MENU_STAGE_FRONT:
		nextStage = updateFrontMenu(&menuData.front_menu, keyStates, stateInfo);
		if (nextStage == MENU_STAGE_CUP) {
			menuData.stage_8_state = 0;
			menuData.rem = 2;
			menuData.pointer = 0;
		} else if (nextStage == MENU_STAGE_TEAM_SELECTION) {
			initTeamSelectionState(&menuData.team_selection);
			menuData.team_selection.rem = stateInfo->numTeams;
			menuData.team_selection.pointer = DEFAULT_TEAM_1;
		}
		menuData.stage = nextStage;
		break;
	case MENU_STAGE_TEAM_SELECTION:
		nextStage = updateTeamSelectionMenu(&menuData.team_selection, stateInfo, keyStates);
		if (nextStage != MENU_STAGE_TEAM_SELECTION) {
			// Explicitly copy the necessary data back to the main state object
			// when the team selection process is finished.
			menuData.team1 = menuData.team_selection.team1;
			menuData.team2 = menuData.team_selection.team2;
			menuData.team1_control = menuData.team_selection.team1_controller;
			menuData.team2_control = menuData.team_selection.team2_controller;
			menuData.inningsInPeriod = menuData.team_selection.innings;
			menuData.cupGame = menuData.team_selection.cupGame;
			// Also, reset the UI state (pointer and rem) for the next menu stage.
			if (nextStage == MENU_STAGE_BATTING_ORDER_1) {
				initBattingOrderState(&menuData.batting_order, menuData.team1, menuData.team1_control);
			} else if (nextStage == MENU_STAGE_FRONT) {
				initFrontMenuState(&menuData.front_menu);
			}
			menuData.stage = nextStage;
		}
		break;
	case MENU_STAGE_BATTING_ORDER_1:
		nextStage = updateBattingOrderMenu(&menuData.batting_order, keyStates, menuData.stage, menuInfo->mode);
		if (nextStage != menuData.stage) {
			memcpy(menuData.team1_batting_order, menuData.batting_order.batting_order, sizeof(menuData.batting_order.batting_order));
			if (nextStage == MENU_STAGE_BATTING_ORDER_2) {
				initBattingOrderState(&menuData.batting_order, menuData.team2, menuData.team2_control);
			} else if (nextStage == MENU_STAGE_HUTUNKEITTO) {
				initHutunkeittoState(&menuData.hutunkeitto);
				menuData.pointer = 0;
				menuData.rem = 2;
			}
			menuData.stage = nextStage;
		}
		break;
	case MENU_STAGE_BATTING_ORDER_2:
		nextStage = updateBattingOrderMenu(&menuData.batting_order, keyStates, menuData.stage, menuInfo->mode);
		if (nextStage != menuData.stage) {
			memcpy(menuData.team2_batting_order, menuData.batting_order.batting_order, sizeof(menuData.batting_order.batting_order));
			if (nextStage == MENU_STAGE_HUTUNKEITTO) {
				initHutunkeittoState(&menuData.hutunkeitto);
				menuData.pointer = 0;
				menuData.rem = 2;
			} else if (nextStage == MENU_STAGE_GO_TO_GAME) {
				moveToGame(stateInfo, menuInfo);
			}
			menuData.stage = nextStage;
		}
		break;
	case MENU_STAGE_HUTUNKEITTO:
		nextStage = updateHutunkeittoMenu(&menuData.hutunkeitto, keyStates, menuData.team1_control, menuData.team2_control);
		if (nextStage != menuData.stage) {
			menuData.playsFirst = menuData.hutunkeitto.playsFirst;
			if (nextStage == MENU_STAGE_GO_TO_GAME) {
				moveToGame(stateInfo, menuInfo);
			}
			menuData.stage = nextStage;
		}
		break;
	case MENU_STAGE_GAME_OVER:
		// Delegate Game-Over logic
		nextStage = updateGameOverMenu(&menuData, stateInfo, keyStates, menuInfo);
		menuData.stage = nextStage;
		break;
	case MENU_STAGE_HOMERUN_CONTEST_1:
		if(menuData.team1_control == 2) {
			int i, j;
			int counter = 0;
			int team = stateInfo->globalGameInfo->teams[0].value - 1;
			int currentIndex = 0;
			for(i = 0; i < 2; i++) {
				for(j = 0; j < menuData.choiceCount/2; j++) {
					menuData.team_1_choices[i][j] = -1;
				}
			}
			// first we select batters for ai. we select ones that have high power
			while(counter < 5) {
				for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
					int power = stateInfo->teamData[team].players[i].power;
					if(currentIndex == menuData.choiceCount/2) break;
					if(power == 5-counter) {
						menuData.team_1_choices[0][currentIndex] = i;
						currentIndex++;
					}
				}
				if(currentIndex == menuData.choiceCount/2) break;
				counter++;
			}
			if(currentIndex != menuData.choiceCount/2) printf("weird stats for players. should exit but we pray.\n");
			counter = 0;
			currentIndex = 0;
			// and from players that are left we select runners. we select ones that have high speed
			while(counter < 5) {
				for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
					int speed = stateInfo->teamData[team].players[i].speed;
					if(currentIndex == menuData.choiceCount/2) break;
					if(speed == 5-counter) {
						int indexValid = 1;
						for(j = 0; j < menuData.choiceCount/2; j++) {
							if(menuData.team_1_choices[0][j] == i) indexValid = 0;
						}
						if(indexValid == 1) {
							menuData.team_1_choices[1][currentIndex] = i;
							currentIndex++;
						}
					}
				}
				if(currentIndex == menuData.choiceCount/2) break;
				counter++;
			}
			if(currentIndex != menuData.choiceCount/2) printf("weird stats for players. should exit but we pray.\n");
			menuData.stage = MENU_STAGE_HOMERUN_CONTEST_2;
		} else {
			if(keyStates->released[menuData.team1_control][KEY_1]) {
				if(menuData.choiceCounter != 0) {
					menuData.team_1_choices[(menuData.choiceCounter-1)/(menuData.choiceCount/2)][(menuData.choiceCounter-1)%(menuData.choiceCount/2)] = -1;
					menuData.choiceCounter--;
				}
			}
			if(keyStates->released[menuData.team1_control][KEY_2]) {
				if(menuData.pointer == 0) {
					if(menuData.choiceCounter >= menuData.choiceCount) {
						menuData.stage = MENU_STAGE_HOMERUN_CONTEST_2;
						menuData.pointer = 1;
						menuData.choiceCounter = 0;
					}
				} else {
					if(menuData.choiceCounter < menuData.choiceCount) {
						int valid = 1;
						int i, j;
						for(i = 0; i < 2; i++) {
							for(j = 0; j < menuData.choiceCount/2; j++) {
								if(menuData.pointer == menuData.team_1_choices[i][j] + 1) {
									valid = 0;
								}
							}
						}
						if(valid == 1) {
							menuData.team_1_choices[menuData.choiceCounter/(menuData.choiceCount/2)][menuData.choiceCounter%(menuData.choiceCount/2)] = menuData.pointer - 1;
							menuData.choiceCounter++;
						}
					}
				}
			}
			if(keyStates->released[menuData.team1_control][KEY_DOWN]) {
				menuData.pointer +=1;
				menuData.pointer = (menuData.pointer+menuData.rem)%menuData.rem;
			}
			if(keyStates->released[menuData.team1_control][KEY_UP]) {
				menuData.pointer -=1;
				menuData.pointer = (menuData.pointer+menuData.rem)%menuData.rem;
			}
		}
		break;
	case MENU_STAGE_HOMERUN_CONTEST_2:
		if(menuData.team2_control == 2) {
			int i, j;
			int counter = 0;
			int team = stateInfo->globalGameInfo->teams[1].value - 1;
			int currentIndex = 0;
			for(i = 0; i < 2; i++) {
				for(j = 0; j < menuData.choiceCount/2; j++) {
					menuData.team_2_choices[i][j] = -1;
				}
			}
			while(counter < 5) {
				for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
					int power = stateInfo->teamData[team].players[i].power;
					if(currentIndex == menuData.choiceCount/2) break;
					if(power == 5-counter) {
						menuData.team_2_choices[0][currentIndex] = i;
						currentIndex++;
					}
				}
				if(currentIndex == menuData.choiceCount/2) break;
				counter++;
			}
			if(currentIndex != menuData.choiceCount/2) printf("weird stats for players. should exit or pray.\n");
			counter = 0;
			currentIndex = 0;
			while(counter < 5) {
				for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
					int speed = stateInfo->teamData[team].players[i].speed;
					if(currentIndex == menuData.choiceCount/2) break;
					if(speed == 5-counter) {
						int indexValid = 1;
						for(j = 0; j < menuData.choiceCount/2; j++) {
							if(menuData.team_2_choices[0][j] == i) indexValid = 0;
						}
						if(indexValid == 1) {
							menuData.team_2_choices[1][currentIndex] = i;
							currentIndex++;
						}
					}
				}
				if(currentIndex == menuData.choiceCount/2) break;
				counter++;
			}
			if(currentIndex != menuData.choiceCount/2) printf("weird stats for players. should exit or pray.\n");
			moveToGame(stateInfo, menuInfo);
		} else {
			if(keyStates->released[menuData.team2_control][KEY_1]) {
				if(menuData.choiceCounter != 0) {
					menuData.team_2_choices[(menuData.choiceCounter-1)/(menuData.choiceCount/2)][(menuData.choiceCounter-1)%(menuData.choiceCount/2)] = -1;
					menuData.choiceCounter--;
				} else {
					menuData.choiceCounter = menuData.choiceCount;
					menuData.stage = MENU_STAGE_HOMERUN_CONTEST_1;
					menuData.pointer = 0;
				}
			}
			if(keyStates->released[menuData.team2_control][KEY_2]) {
				if(menuData.pointer == 0) {
					if(menuData.choiceCounter >= menuData.choiceCount) {
						moveToGame(stateInfo, menuInfo);
					}
				} else {
					if(menuData.choiceCounter < menuData.choiceCount) {
						int valid = 1;
						int i, j;
						for(i = 0; i < 2; i++) {
							for(j = 0; j < menuData.choiceCount/2; j++) {
								if(menuData.pointer == menuData.team_2_choices[i][j] + 1) {
									valid = 0;
								}
							}
						}
						if(valid == 1) {
							menuData.team_2_choices[menuData.choiceCounter/(menuData.choiceCount/2)][menuData.choiceCounter%(menuData.choiceCount/2)] = menuData.pointer - 1;
							menuData.choiceCounter++;
						}
					}
				}
			}
			if(keyStates->released[menuData.team2_control][KEY_DOWN]) {
				menuData.pointer +=1;
				menuData.pointer = (menuData.pointer+menuData.rem)%menuData.rem;
			}
			if(keyStates->released[menuData.team2_control][KEY_UP]) {
				menuData.pointer -=1;
				menuData.pointer = (menuData.pointer+menuData.rem)%menuData.rem;
			}
		}
		break;
	case MENU_STAGE_CUP:
		if(menuData.stage_8_state == 0) {
			if(keyStates->released[0][KEY_DOWN]) {
				menuData.pointer +=1;
				menuData.pointer = (menuData.pointer+menuData.rem)%menuData.rem;
			}
			if(keyStates->released[0][KEY_UP]) {
				menuData.pointer -=1;
				menuData.pointer = (menuData.pointer+menuData.rem)%menuData.rem;
			}
			if(keyStates->released[0][KEY_2]) {
				if(menuData.pointer == 0) {
					menuData.stage_8_state = 1;
					menuData.stage_8_state_1_level = 0;
					menuData.rem = stateInfo->numTeams;
					menuData.pointer = 0;
				} else if(menuData.pointer == 1) {
					menuData.stage_8_state = 5;
					menuData.rem = 5;
					menuData.pointer = 0;
				}
			}
			if(keyStates->released[0][KEY_1]) {
				menuData.stage = MENU_STAGE_FRONT;
				initFrontMenuState(&menuData.front_menu);
				menuData.front_menu.pointer = 1;
			}
		} else if(menuData.stage_8_state == 1) {
			if(menuData.stage_8_state_1_level == 0) {
				if(keyStates->released[0][KEY_DOWN]) {
					menuData.pointer +=1;
					menuData.pointer = (menuData.pointer+menuData.rem)%menuData.rem;
				}
				if(keyStates->released[0][KEY_UP]) {
					menuData.pointer -=1;
					menuData.pointer = (menuData.pointer+menuData.rem)%menuData.rem;
				}
				if(keyStates->released[0][KEY_2]) {
					menuData.stage_8_state_1_level = 1;
					menuData.teamSelection = menuData.pointer;
					menuData.pointer = 0;
					menuData.rem = 2;
				}
				if(keyStates->released[0][KEY_1]) {
					menuData.pointer = 0;
					menuData.rem = 2;
					menuData.stage_8_state = 0;
				}
			} else if(menuData.stage_8_state_1_level == 1) {
				if(keyStates->released[0][KEY_DOWN]) {
					menuData.pointer +=1;
					menuData.pointer = (menuData.pointer+menuData.rem)%menuData.rem;
				}
				if(keyStates->released[0][KEY_UP]) {
					menuData.pointer -=1;
					menuData.pointer = (menuData.pointer+menuData.rem)%menuData.rem;
				}
				if(keyStates->released[0][KEY_2]) {
					menuData.cupInfo.gameStructure = menuData.pointer;
					menuData.stage_8_state_1_level = 2;
					menuData.pointer = 0;
					menuData.rem = 3;
				}
				if(keyStates->released[0][KEY_1]) {
					menuData.pointer = 0;
					menuData.rem = stateInfo->numTeams;
					menuData.stage_8_state_1_level = 0;
				}
			} else if(menuData.stage_8_state_1_level == 2) {
				if(keyStates->released[0][KEY_DOWN]) {
					menuData.pointer +=1;
					menuData.pointer = (menuData.pointer+menuData.rem)%menuData.rem;
				}
				if(keyStates->released[0][KEY_UP]) {
					menuData.pointer -=1;
					menuData.pointer = (menuData.pointer+menuData.rem)%menuData.rem;
				}
				if(keyStates->released[0][KEY_2]) {
					int i;
					// fill the\1menuData.cupInfo-structure
					if(menuData.pointer == 0) menuData.cupInfo.inningCount = 2;
					else if(menuData.pointer == 1) menuData.cupInfo.inningCount = 4;
					else if(menuData.pointer == 2) menuData.cupInfo.inningCount = 8;
					menuData.cupInfo.userTeamIndexInTree = 0;
					menuData.cupInfo.winnerIndex = -1;
					menuData.cupInfo.dayCount = 0;
					for(i = 0; i < SLOT_COUNT; i++) {
						menuData.cupInfo.cupTeamIndexTree[i] = -1;
					}
					i = 0;
					while(i < 8) {
						int random = rand()%8;
						if(menuData.cupInfo.cupTeamIndexTree[random] == -1) {
							menuData.cupInfo.cupTeamIndexTree[random] = i;
							if(i == menuData.teamSelection) {
								menuData.cupInfo.userTeamIndexInTree = random;
							}
							i++;
						}
					}
					for(i = 0; i < SLOT_COUNT; i++) {
						menuData.cupInfo.slotWins[i] = 0;
					}
					for(i = 0; i < 4; i++) {
						menuData.cupInfo.schedule[i][0] = i*2;
						menuData.cupInfo.schedule[i][1] = i*2+1;
					}
					menuData.stage_8_state = 2;
					menuData.pointer = 0;
					menuData.rem = 5;
				}
				if(keyStates->released[0][KEY_1]) {
					menuData.pointer = 0;
					menuData.rem = 2;
					menuData.stage_8_state_1_level = 1;
				}
			}
		} else if(menuData.stage_8_state == 2) {
			if(keyStates->released[0][KEY_DOWN]) {
				menuData.pointer +=1;
				menuData.pointer = (menuData.pointer+menuData.rem)%menuData.rem;
			}
			if(keyStates->released[0][KEY_UP]) {
				menuData.pointer -=1;
				menuData.pointer = (menuData.pointer+menuData.rem)%menuData.rem;
			}
			if(keyStates->released[0][KEY_2]) {
				if(menuData.pointer == 0) {
					int userTeamIndex = -1;
					int userPosition = 0;
					int opponentTeamIndex = -1;
					int i, j;
					// so here we start the game of the human player.
					// first lets find out if there is a game for human player.
					for(i = 0; i < 4; i++) {
						for(j = 0; j < 2; j++) {
							if(menuData.cupInfo.schedule[i][j] == menuData.cupInfo.userTeamIndexInTree) {
								if(j == 0) {
									userTeamIndex = menuData.cupInfo.cupTeamIndexTree[menuData.cupInfo.schedule[i][0]];
									opponentTeamIndex = menuData.cupInfo.cupTeamIndexTree[menuData.cupInfo.schedule[i][1]];
									userPosition = 0;
								} else {
									userTeamIndex = menuData.cupInfo.cupTeamIndexTree[menuData.cupInfo.schedule[i][1]];
									opponentTeamIndex = menuData.cupInfo.cupTeamIndexTree[menuData.cupInfo.schedule[i][0]];
									userPosition = 1;
								}
							}
						}
					}
					menuData.cupInfo.dayCount++;
					// if there is, we proceed to the match and let the match ending update cup trees and schedules.
					if(userTeamIndex != -1) {
						menuData.stage = MENU_STAGE_BATTING_ORDER_1;
						menuData.pointer = 0;
						menuData.rem = 13;
						if(userPosition == 0) {
							menuData.team1 = userTeamIndex;
							menuData.team2 = opponentTeamIndex;
							menuData.team1_control = 0;
							menuData.team2_control = 2;
						} else {
							menuData.team2 = userTeamIndex;
							menuData.team1 = opponentTeamIndex;
							menuData.team2_control = 0;
							menuData.team1_control = 2;
						}
						menuData.inningsInPeriod = menuData.cupInfo.inningCount;
						menuData.cupGame = 1;
					} else {
						// otherwise we update them right away.
						updateCupTreeAfterDay(&menuData, stateInfo, -1, 0);
						updateSchedule(&menuData, stateInfo);
					}
				} else if(menuData.pointer == 1) {
					menuData.stage_8_state = 4;
				} else if(menuData.pointer == 2) {
					menuData.stage_8_state = 3;
				} else if(menuData.pointer == 3) {
					menuData.stage_8_state = 6;
					menuData.pointer = 0;
					menuData.rem = 5;
				} else if(menuData.pointer == 4) {
					menuData.stage = MENU_STAGE_FRONT;
					initFrontMenuState(&menuData.front_menu);
				}
			}
		} else if(menuData.stage_8_state == 3) {
			if(keyStates->released[0][KEY_2]) {
				menuData.stage_8_state = 2;
			}
			if(keyStates->released[0][KEY_1]) {
				menuData.stage_8_state = 2;
			}
		} else if(menuData.stage_8_state == 5) {
			// load
			if(keyStates->released[0][KEY_DOWN]) {
				menuData.pointer +=1;
				menuData.pointer = (menuData.pointer+menuData.rem)%menuData.rem;
			}
			if(keyStates->released[0][KEY_UP]) {
				menuData.pointer -=1;
				menuData.pointer = (menuData.pointer+menuData.rem)%menuData.rem;
			}
			if(keyStates->released[0][KEY_1]) {
				menuData.stage_8_state = 0;
				menuData.pointer = 1;
				menuData.rem = 2;
			}
			if(keyStates->released[0][KEY_2]) {
				if(menuData.saveData[menuData.pointer].userTeamIndexInTree != -1) {
					loadCup(stateInfo, menuData.pointer);
					menuData.stage_8_state = 2;
					menuData.pointer = 0;
					menuData.rem = 5;
				}
			}
		} else if(menuData.stage_8_state == 6) {
			// save
			if(keyStates->released[0][KEY_DOWN]) {
				menuData.pointer +=1;
				menuData.pointer = (menuData.pointer+menuData.rem)%menuData.rem;
			}
			if(keyStates->released[0][KEY_UP]) {
				menuData.pointer -=1;
				menuData.pointer = (menuData.pointer+menuData.rem)%menuData.rem;
			}
			if(keyStates->released[0][KEY_1]) {
				menuData.stage_8_state = 2;
				menuData.pointer = 2;
				menuData.rem = 5;
			}
			if(keyStates->released[0][KEY_2]) {
				saveCup(stateInfo, menuData.pointer);
			}
		} else if(menuData.stage_8_state == 4) {
			if(keyStates->released[0][KEY_2]) {
				menuData.stage_8_state = 2;
			}
			if(keyStates->released[0][KEY_1]) {
				menuData.stage_8_state = 2;
			}
		} else if(menuData.stage_8_state == 7) {
			if(keyStates->released[0][KEY_2]) {
				menuData.stage_8_state = 2;
			}
			if(keyStates->released[0][KEY_1]) {
				menuData.stage_8_state = 2;
			}
		}
		break;
	case MENU_STAGE_HELP:
		if(menuData.stage_9_state == 0) {
			if(keyStates->released[0][KEY_2]) {
				menuData.stage_9_state = 1;
			}
			if(keyStates->released[0][KEY_1]) {
				menuData.stage_9_state = 0;
				menuData.stage = MENU_STAGE_FRONT;
			}
		} else if(menuData.stage_9_state == 1) {
			if(keyStates->released[0][KEY_2]) {
				menuData.stage_9_state = 2;
			}
			if(keyStates->released[0][KEY_1]) {
				menuData.stage_9_state = 0;
				menuData.stage = MENU_STAGE_FRONT;
			}
		} else if(menuData.stage_9_state == 2) {
			if(keyStates->released[0][KEY_2]) {
				menuData.stage_9_state = 3;
			}
			if(keyStates->released[0][KEY_1]) {
				menuData.stage_9_state = 0;
				menuData.stage = MENU_STAGE_FRONT;
			}
		} else if(menuData.stage_9_state == 3) {
			if(keyStates->released[0][KEY_2]) {
				menuData.stage_9_state = 4;
			}
			if(keyStates->released[0][KEY_1]) {
				menuData.stage_9_state = 0;
				menuData.stage = MENU_STAGE_FRONT;
			}
		} else if(menuData.stage_9_state == 4) {
			if(keyStates->released[0][KEY_2]) {
				menuData.stage_9_state = 0;
				menuData.stage = MENU_STAGE_FRONT;
			}
			if(keyStates->released[0][KEY_1]) {
				menuData.stage_9_state = 0;
				menuData.stage = MENU_STAGE_FRONT;
			}
		}
		break;
	case MENU_STAGE_GO_TO_GAME:
		break;
	}
}

// here we draw everything.
// directly we draw ugly stuff like images of players or hand or bat models etc.
// then we call methods to handle text rendering.
void drawMainMenu(StateInfo* stateInfo, MenuInfo* menuInfo, double alpha)
{
	gluLookAt(menuData.cam.x, menuData.cam.y, menuData.cam.z, menuData.look.x, menuData.look.y, menuData.look.z, menuData.up.x, menuData.up.y, menuData.up.z);
	switch(menuData.stage) {
	case MENU_STAGE_FRONT:
		drawFrontMenu(&menuData.front_menu, &menuData);
		break;
	case MENU_STAGE_TEAM_SELECTION:
		drawTeamSelectionMenu(&menuData.team_selection, stateInfo, &menuData);
		break;
	case MENU_STAGE_BATTING_ORDER_1:
	case MENU_STAGE_BATTING_ORDER_2:
		drawBattingOrderMenu(&menuData.batting_order, stateInfo, &menuData);
		break;
	case MENU_STAGE_HUTUNKEITTO:
		drawHutunkeittoMenu(&menuData.hutunkeitto, &menuData);
		break;
	case MENU_STAGE_GAME_OVER:
		drawGameOverMenu(stateInfo);
		break;
	case MENU_STAGE_HOMERUN_CONTEST_1:
		if (menuData.team1_control != 2) {
			int i, j;
			drawFontBackground();
			glBindTexture(GL_TEXTURE_2D, menuData.arrowTexture);
			glPushMatrix();
			if(menuData.pointer == 0) {
				glTranslatef(PLAYER_LIST_ARROW_CONTINUE_POS, 1.0f, PLAYER_LIST_CONTINUE_HEIGHT);
				glScalef(ARROW_SCALE, ARROW_SCALE, ARROW_SCALE);
			} else {
				glTranslatef(PLAYER_LIST_ARROW_POS, 1.0f, PLAYER_LIST_FIRST_PLAYER_HEIGHT + menuData.pointer*PLAYER_LIST_PLAYER_OFFSET - PLAYER_LIST_PLAYER_OFFSET);
				glScalef(ARROW_SMALLER_SCALE, ARROW_SMALLER_SCALE,ARROW_SMALLER_SCALE);
			}

			glCallList(menuData.planeDisplayList);
			glPopMatrix();

			for(i = 0; i < 2; i++) {
				for(j = 0; j < 5; j++) {
					char str[2];
					int choice;
					if(menuData.stage == MENU_STAGE_HOMERUN_CONTEST_1) choice = menuData.team_1_choices[i][j];
					else choice = menuData.team_2_choices[i][j];
					str[0] = (char)(((int)'0')+j+1);
					if(choice != -1) {
						printText(str, 1, 0.45f + i*0.05f, PLAYER_LIST_FIRST_PLAYER_HEIGHT +
						          (choice+1)*PLAYER_LIST_PLAYER_OFFSET - PLAYER_LIST_PLAYER_OFFSET, 2);
					}
				}
			}
		}
		break;
	case MENU_STAGE_HOMERUN_CONTEST_2:
		if(menuData.team2_control != 2) {
			int i, j;
			drawFontBackground();
			glBindTexture(GL_TEXTURE_2D, menuData.arrowTexture);
			glPushMatrix();
			if(menuData.pointer == 0) {
				glTranslatef(PLAYER_LIST_ARROW_CONTINUE_POS, 1.0f, PLAYER_LIST_CONTINUE_HEIGHT);
				glScalef(ARROW_SCALE, ARROW_SCALE, ARROW_SCALE);
			} else {
				glTranslatef(PLAYER_LIST_ARROW_POS, 1.0f, PLAYER_LIST_FIRST_PLAYER_HEIGHT + menuData.pointer*PLAYER_LIST_PLAYER_OFFSET - PLAYER_LIST_PLAYER_OFFSET);
				glScalef(ARROW_SMALLER_SCALE, ARROW_SMALLER_SCALE,ARROW_SMALLER_SCALE);
			}

			glCallList(menuData.planeDisplayList);
			glPopMatrix();

			for(i = 0; i < 2; i++) {
				for(j = 0; j < 5; j++) {
					char str[2];
					int choice;
					if(menuData.stage == MENU_STAGE_HOMERUN_CONTEST_1) choice = menuData.team_1_choices[i][j];
					else choice = menuData.team_2_choices[i][j];
					str[0] = (char)(((int)'0')+j+1);
					if(choice != -1) {
						printText(str, 1, 0.45f + i*0.05f, PLAYER_LIST_FIRST_PLAYER_HEIGHT +
						          (choice+1)*PLAYER_LIST_PLAYER_OFFSET - PLAYER_LIST_PLAYER_OFFSET, 2);
					}
				}
			}
		}
		break;
	case MENU_STAGE_CUP:
		drawFontBackground();
		if(menuData.stage_8_state == 0) {
			// arrow
			glBindTexture(GL_TEXTURE_2D, menuData.arrowTexture);
			glPushMatrix();
			if(menuData.pointer == 0) glTranslatef(FRONT_ARROW_POS + 0.05f, 1.0f, NEW_CUP_TEXT_HEIGHT);
			else if(menuData.pointer == 1) glTranslatef(FRONT_ARROW_POS + 0.05f, 1.0f, LOAD_CUP_TEXT_HEIGHT);
			glScalef(ARROW_SCALE, ARROW_SCALE, ARROW_SCALE);
			glCallList(menuData.planeDisplayList);
			glPopMatrix();
			// catcher
			glBindTexture(GL_TEXTURE_2D, menuData.catcherTexture);
			glPushMatrix();
			glTranslatef(0.7f, 1.0f, 0.0f);
			glScalef(FIGURE_SCALE, FIGURE_SCALE, FIGURE_SCALE);
			glCallList(menuData.planeDisplayList);
			glPopMatrix();
			// batter
			glBindTexture(GL_TEXTURE_2D, menuData.batterTexture);
			glPushMatrix();
			glTranslatef(-0.6f, 1.0f, 0.0f);
			glScalef(FIGURE_SCALE/2, FIGURE_SCALE, FIGURE_SCALE);
			glCallList(menuData.planeDisplayList);
			glPopMatrix();
		} else if(menuData.stage_8_state == 1) {
			glBindTexture(GL_TEXTURE_2D, menuData.arrowTexture);
			glPushMatrix();
			glTranslatef(SELECTION_CUP_ARROW_LEFT, 1.0f, SELECTION_CUP_ALT_1_HEIGHT + SELECTION_ALT_OFFSET*menuData.pointer);
			glScalef(ARROW_SCALE, ARROW_SCALE, ARROW_SCALE);
			glCallList(menuData.planeDisplayList);
			glPopMatrix();
		} else if(menuData.stage_8_state == 2) {
			glBindTexture(GL_TEXTURE_2D, menuData.arrowTexture);
			glPushMatrix();
			glTranslatef(SELECTION_CUP_ARROW_LEFT, 1.0f, SELECTION_CUP_ALT_1_HEIGHT + SELECTION_CUP_MENU_OFFSET*menuData.pointer);
			glScalef(ARROW_SCALE, ARROW_SCALE, ARROW_SCALE);
			glCallList(menuData.planeDisplayList);
			glPopMatrix();
		} else if(menuData.stage_8_state == 3) {
			int i;
			for(i = 0; i < SLOT_COUNT; i++) {
				glBindTexture(GL_TEXTURE_2D, menuData.slotTexture);
				glPushMatrix();
				glTranslatef(treeCoordinates[i].x, 1.0f, treeCoordinates[i].y);
				glScalef(0.2f, 0.15f, 0.10f);
				glCallList(menuData.planeDisplayList);
				glPopMatrix();
			}
		} else if(menuData.stage_8_state == 5 || menuData.stage_8_state == 6) {
			glBindTexture(GL_TEXTURE_2D, menuData.arrowTexture);
			glPushMatrix();
			glTranslatef(SELECTION_CUP_ARROW_LEFT, 1.0f, SELECTION_CUP_ALT_1_HEIGHT + SELECTION_CUP_MENU_OFFSET*menuData.pointer);
			glScalef(ARROW_SCALE, ARROW_SCALE, ARROW_SCALE);
			glCallList(menuData.planeDisplayList);
			glPopMatrix();
		} else if(menuData.stage_8_state == 7) {
			glBindTexture(GL_TEXTURE_2D, menuData.trophyTexture);
			glPushMatrix();
			glTranslatef(0.0f, 1.0f, -0.25f);
			glScalef(0.3f, 0.3f, 0.3f);
			glCallList(menuData.planeDisplayList);
			glPopMatrix();
		}
		drawCup(stateInfo);
		break;
	case MENU_STAGE_HELP:
		drawFontBackground();
		drawHelp(stateInfo);
		break;
	case MENU_STAGE_GO_TO_GAME:
		break;
	}
}
// values here are mostly hard-coded, some of them have defines, some dont. do i care ;_;
// most of the repeating stuff has some defines though.
static void drawLoadingTexts(StateInfo* stateInfo)
{
	printText("Loading resources", 17, -0.21f, LOADING_MODELS_HEIGHT, 2);
	printText("Your patience is appreciated", 28, -0.48f, LOADING_APPRECIATED_HEIGHT, 3);
	printText("Erkka Heinila 2013", 18, -0.22f, LOADING_AUTHOR_HEIGHT, 2);
}

static void drawCup(StateInfo* stateInfo)
{
	if(menuData.stage_8_state == 0) {
		printText("P N B", 5, -0.23f, -0.4f, 8);
		printText("New cup", 7, -0.16f, NEW_CUP_TEXT_HEIGHT, 3);
		printText("Load cup", 8, -0.18f, LOAD_CUP_TEXT_HEIGHT, 3);
	} else if(menuData.stage_8_state == 1) {
		if(menuData.stage_8_state_1_level == 0) {
			int i;
			printText("Select team", 11, -0.35f, -0.4f, 5);
			for(i = 0; i < stateInfo->numTeams; i++) {
				char* str = stateInfo->teamData[i].name;
				printText(str, strlen(str), SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT + i*SELECTION_ALT_OFFSET, 2);
			}
		} else if(menuData.stage_8_state_1_level == 1) {
			printText("How many wins to move forward", 29, -0.5f, -0.4f, 3);
			printText("Normal", 6, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT, 2);
			printText("One", 3, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT + SELECTION_ALT_OFFSET, 2);
		} else if(menuData.stage_8_state_1_level == 2) {
			printText("How many innings in period", 26, -0.5f, -0.4f, 3);
			printText("1", 1, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT, 2);
			printText("2", 1, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT + SELECTION_ALT_OFFSET, 2);
			printText("4", 1, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT + 2*SELECTION_ALT_OFFSET, 2);
		}
	} else if(menuData.stage_8_state == 2) {
		printText("Cup menu", 8, -0.22f, -0.4f, 5);
		printText("Next day", 8, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT, 2);
		printText("Schedule", 8, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT + SELECTION_CUP_MENU_OFFSET, 2);
		printText("Cup tree", 8, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT + 2*SELECTION_CUP_MENU_OFFSET, 2);
		printText("Save", 4, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT + 3*SELECTION_CUP_MENU_OFFSET, 2);
		printText("Quit", 4, SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT + 4*SELECTION_CUP_MENU_OFFSET, 2);
		if(menuData.cupInfo.winnerIndex != -1) {
			char* str = stateInfo->teamData[menuData.cupInfo.winnerIndex].name;
			printText(str, strlen(str), -0.45f, SELECTION_CUP_ALT_1_HEIGHT + 6*SELECTION_CUP_MENU_OFFSET, 3);
			printText("has won the cup", 15, -0.45f + strlen(str)*0.04f, SELECTION_CUP_ALT_1_HEIGHT + 6*SELECTION_CUP_MENU_OFFSET, 3);
		}
	} else if(menuData.stage_8_state == 3) {
		int i;
		for(i = 0; i < SLOT_COUNT; i++) {
			int index = menuData.cupInfo.cupTeamIndexTree[i];
			if(index != -1) {
				char* str = stateInfo->teamData[index].name;
				char wins[2] = " ";
				wins[0] = (char)('0' + menuData.cupInfo.slotWins[i]);
				printText(str, strlen(str), treeCoordinates[i].x - 0.15f, treeCoordinates[i].y, 2);
				printText(wins, 1, treeCoordinates[i].x + 0.25f, treeCoordinates[i].y, 3);
			}
		}
	} else if(menuData.stage_8_state == 4) {
		int i;
		int counter = 0;
		int index1, index2;
		printText("schedule", 8, -0.15f, -0.35f, 3);
		for(i = 0; i < 4; i++) {
			index1 = -1;
			index2 = -1;
			if(menuData.cupInfo.schedule[i][0] != -1)
				index1 = menuData.cupInfo.cupTeamIndexTree[menuData.cupInfo.schedule[i][0]];
			if(menuData.cupInfo.schedule[i][1] != -1)
				index2 = menuData.cupInfo.cupTeamIndexTree[menuData.cupInfo.schedule[i][1]];
			if(index1 != -1 && index2 != -1) {
				char* str = stateInfo->teamData[index1].name;
				char* str2 = stateInfo->teamData[index2].name;
				printText(str, strlen(str), -0.4f, -0.15f + counter*0.1f, 2);
				printText("-", 1, -0.02f, -0.15f + counter*0.1f, 2);
				printText(str2, strlen(str2), 0.1f, -0.15f + counter*0.1f, 2);
				counter++;
			}
		}
	} else if(menuData.stage_8_state == 7) {
		printText("there you go champ", 18, -0.4f, -0.45f, 4);
		printText("so you beat the game huh", 24, -0.28f, 0.1f, 2);
		printText("the mighty conqueror", 20, -0.23f, 0.15f, 2);
		printText("anyway thank you for playing", 28, -0.3f, 0.2f, 2);
		printText("made me happy", 13, -0.14f, 0.25f, 2);
		printText("special thanks to", 17, -0.21f, 0.33f, 2);
		printText("petri anttila juuso heinila matti pitkanen", 42, -0.5f, 0.38f, 2);
		printText("ville viljanmaa petri mikola pekka heinila", 43, -0.5f, 0.43f, 2);
		printText("for supporting the development in various ways", 46, -0.53f, 0.48f, 2);

	} else if(menuData.stage_8_state == 5 || menuData.stage_8_state == 6) {
		int i;
		if(menuData.stage_8_state == 5) {
			printText("Load cup", 8, -0.22f, -0.4f, 5);
		} else {
			printText("Save cup", 8, -0.22f, -0.4f, 5);
		}
		for(i = 0; i < 5; i++) {
			char* text;
			char* empty = "Empty slot";
			int index = menuData.saveData[i].userTeamIndexInTree;
			if(index != -1) {
				text = stateInfo->teamData[menuData.saveData[i].cupTeamIndexTree[index]].name;
			} else {
				text = empty;
			}
			printText(text, strlen(text), SELECTION_CUP_LEFT, SELECTION_CUP_ALT_1_HEIGHT + i*SELECTION_CUP_MENU_OFFSET, 2);
		}
	}
}

static void drawHelp(StateInfo* stateInfo)
{
	if(menuData.stage_9_state == 0) {
		printText("Controls", 8, HELP_LEFT, -0.45f, 4);
		printText("Pad 1", 5, HELP_LEFT, -0.3f, 3);
		printText("Direction keys - Arrow keys", 27, HELP_LEFT, -0.2f, 2);
		printText("Action key - Return", 19, HELP_LEFT, -0.15f, 2);
		printText("Second action key - Right shift", 31, HELP_LEFT, -0.10f, 2);
		printText("Left strafe - Right alt", 23, HELP_LEFT, -0.05f, 2);
		printText("Right strafe - Right ctrl", 25, HELP_LEFT, 0.0f, 2);
		printText("Pad 2", 5, HELP_LEFT, 0.1f, 3);
		printText("Direction keys - G F H T", 24, HELP_LEFT, 0.2f, 2);
		printText("Action key - A", 14, HELP_LEFT, 0.25f, 2);
		printText("Second action key - Z", 21, HELP_LEFT, 0.3f, 2);
		printText("Left strafe - Left ctrl", 23, HELP_LEFT, 0.35f, 2);
		printText("Right strafe - Left alt", 23, HELP_LEFT, 0.4f, 2);
	} else if(menuData.stage_9_state == 1) {
		char* str;
		printText("Batting", 7, HELP_LEFT, -0.45f, 3);
		str = "Batting is done by the action key. Movement of the batter starts";
		printText(str, strlen(str), HELP_LEFT, -0.35f, 2);
		str = "automatically when pitch is in air. If you dont press the key";
		printText(str, strlen(str), HELP_LEFT, -0.3f, 2);
		str = "before the marker reaches the end of the meter, batter will not bat.";
		printText(str, strlen(str), HELP_LEFT, -0.25f, 2);
		str = "If you want to bat, select power by pressing the key down when marker";
		printText(str, strlen(str), HELP_LEFT, -0.2f, 2);
		str = "reaches wanted height and then releasing it after marker has come down";
		printText(str, strlen(str), HELP_LEFT, -0.15f, 2);
		str = "to the yellow area. ";
		printText(str, strlen(str), HELP_LEFT, -0.10f, 2);
		str = "Balls vertical takeoff direction depends on the actual spot of the";
		printText(str, strlen(str), HELP_LEFT, 0.0f, 2);
		str = "marker in the yellow area. You can adjust the horizontal angle of ";
		printText(str, strlen(str), HELP_LEFT, 0.05f, 2);
		str = "balls takeoff direction by strafing with batter. If there is no";
		printText(str, strlen(str), HELP_LEFT, 0.1f, 2);
		str = "batter you can browse through available batters by pressing the";
		printText(str, strlen(str), HELP_LEFT, 0.15f, 2);
		str = "second action key and select one by pressing the action key";
		printText(str, strlen(str), HELP_LEFT, 0.2f, 2);

	} else if(menuData.stage_9_state == 2) {
		char* str;
		printText("Running", 7, HELP_LEFT, -0.45f, 3);
		str = "Running is controlled by the direction keys. Every direction key";
		printText(str, strlen(str), HELP_LEFT, -0.35f, 2);
		str = "represents a base. Down is the home, left is the first base, right";
		printText(str, strlen(str), HELP_LEFT, -0.3f, 2);
		str = "is the second base and up is the third base. If you just suddenly";
		printText(str, strlen(str), HELP_LEFT, -0.25f, 2);
		str = "feel the urge to run, you can double click one of these keys to";
		printText(str, strlen(str), HELP_LEFT, -0.2f, 2);
		str = "make runner at that base to run. Normally you probably should ";
		printText(str, strlen(str), HELP_LEFT, -0.15f, 2);
		str = "synchronize running with pitching and batting though. This is done";
		printText(str, strlen(str), HELP_LEFT, -0.10f, 2);
		str = "by pressing the key once. That will make baserunners start running";
		printText(str, strlen(str), HELP_LEFT, -0.05f, 2);
		str = "immediately after pitch leaves pitchers hand. Batter will start";
		printText(str, strlen(str), HELP_LEFT, 0.0f, 2);
		str = "running after he has finished his swing or bunt. If you ended up";
		printText(str, strlen(str), HELP_LEFT, 0.05f, 2);
		str = "running but arent sure why and would like to return to the previous";
		printText(str, strlen(str), HELP_LEFT, 0.1f, 2);
		str = "base, you can press the corresponding direction key once and the";
		printText(str, strlen(str), HELP_LEFT, 0.15f, 2);
		str = "runner will return if he still is safe on that base.";
		printText(str, strlen(str), HELP_LEFT, 0.2f, 2);
		str = "If pitcher pitches enough invalid pitches you will be prompted if ";
		printText(str, strlen(str), HELP_LEFT, 0.3f, 2);
		str = "you want to take a free walk or not. Action key accepts and second ";
		printText(str, strlen(str), HELP_LEFT, 0.35f, 2);
		str = "action key rejects.";
		printText(str, strlen(str), HELP_LEFT, 0.4f, 2);
	} else if(menuData.stage_9_state == 3) {
		char* str;
		printText("Pitching", 8, HELP_LEFT, -0.45f, 3);
		str = "Pitching is also done with the action key. To pitch you first hold";
		printText(str, strlen(str), HELP_LEFT, -0.35f, 2);
		str = "the action key down and the marker will rise. You select the power";
		printText(str, strlen(str), HELP_LEFT, -0.3f, 2);
		str = "by releasing action key and after that you must click the action";
		printText(str, strlen(str), HELP_LEFT, -0.25f, 2);
		str = "key when marker reaches the yellow area again. The final position";
		printText(str, strlen(str), HELP_LEFT, -0.2f, 2);
		str = "of marker in yellow area will affect the takeoff direction of ball.";
		printText(str, strlen(str), HELP_LEFT, -0.15f, 2);
	} else if(menuData.stage_9_state == 4) {
		char* str;
		printText("Fielding", 8, HELP_LEFT, -0.45f, 3);
		str = "You move with a fielder by pressing direction keys. If you dont";
		printText(str, strlen(str), HELP_LEFT, -0.35f, 2);
		str = "have the ball, the action key will change the player. If you have";
		printText(str, strlen(str), HELP_LEFT, -0.3f, 2);
		str = "the ball, pressing action key alone will make player drop the ball.";
		printText(str, strlen(str), HELP_LEFT, -0.25f, 2);
		str = "If you hold one direction key when holding down the action key, ";
		printText(str, strlen(str), HELP_LEFT, -0.2f, 2);
		str = "the player will load some throwing power. Releasing the key will";
		printText(str, strlen(str), HELP_LEFT, -0.15f, 2);
		str = "make player to throw the ball towards a base. The base corresponds";
		printText(str, strlen(str), HELP_LEFT, -0.10f, 2);
		str = "to direction key that was pressed. That is, down will be the home";
		printText(str, strlen(str), HELP_LEFT, -0.05f, 2);
		str = "base, left will be the first base, right will be the second base";
		printText(str, strlen(str), HELP_LEFT, 0.0f, 2);
		str = "and up will be the third base.";
		printText(str, strlen(str), HELP_LEFT, 0.05f, 2);
	}
}

int cleanMainMenu(StateInfo* stateInfo)
{
	cleanMesh(menuData.planeMesh);
	cleanMesh(menuData.handMesh);
	cleanMesh(menuData.batMesh);
	return 0;
}


// and we initialize the game.
static void moveToGame(StateInfo* stateInfo, MenuInfo* menuInfo)
{
	stateInfo->playSoundEffect = 0;
	stateInfo->screen = 1;
	stateInfo->changeScreen = 1;
	stateInfo->updated = 0;
	// when first starting the game, we se teams and inning and period settings.
	if(menuInfo->mode == MENU_ENTRY_NORMAL) {
		int i;
		stateInfo->globalGameInfo->inning = 0;
		stateInfo->globalGameInfo->inningsInPeriod = menuData.inningsInPeriod;
		stateInfo->globalGameInfo->period = 0;
		stateInfo->globalGameInfo->winner = -1;
		stateInfo->globalGameInfo->playsFirst = menuData.playsFirst;
		stateInfo->globalGameInfo->teams[0].value = menuData.team1 + 1;
		stateInfo->globalGameInfo->teams[1].value = menuData.team2 + 1;
		stateInfo->globalGameInfo->teams[0].control = menuData.team1_control;
		stateInfo->globalGameInfo->teams[1].control = menuData.team2_control;
		for(i = 0; i < 2; i++) {
			stateInfo->globalGameInfo->teams[i].runs = 0;
			stateInfo->globalGameInfo->teams[i].period0Runs = 0;
			stateInfo->globalGameInfo->teams[i].period1Runs = 0;
			stateInfo->globalGameInfo->teams[i].period2Runs = 0;
			stateInfo->globalGameInfo->teams[i].period3Runs = 0;
		}
	}
	// in the beginning and after second period we have had hutunkeitto.
	if(menuInfo->mode == MENU_ENTRY_NORMAL || menuInfo->mode == MENU_ENTRY_SUPER_INNING) {
		stateInfo->globalGameInfo->playsFirst = menuData.playsFirst;
	}
	// after super period we have to do different kind of initialization.
	if(menuInfo->mode == MENU_ENTRY_HOMERUN_CONTEST) {
		int i, j;
		for(i = 0; i < 2; i++) {
			for(j = 0; j < menuData.choiceCount/2; j++) {
				stateInfo->globalGameInfo->teams[0].batterRunnerIndices[i][j] = menuData.team_1_choices[i][j];
				stateInfo->globalGameInfo->teams[1].batterRunnerIndices[i][j] = menuData.team_2_choices[i][j];
			}
			if(stateInfo->globalGameInfo->period > 4) {
				for(j = menuData.choiceCount/2; j < 5; j++) {
					stateInfo->globalGameInfo->teams[0].batterRunnerIndices[i][j] = -1;
					stateInfo->globalGameInfo->teams[1].batterRunnerIndices[i][j] = -1;
				}
			}
		}
		stateInfo->globalGameInfo->pairCount = menuData.choiceCount / 2;
		stateInfo->localGameInfo->gAI.runnerBatterPairCounter = 0;
	} else {
		// if homerun batting contest is not coming, we just set batterOrder settings normally.
		stateInfo->globalGameInfo->teams[0].batterOrderIndex = 0;
		stateInfo->globalGameInfo->teams[1].batterOrderIndex = 0;
		memcpy(stateInfo->globalGameInfo->teams[0].batterOrder, menuData.team1_batting_order, sizeof(menuData.team1_batting_order));
		memcpy(stateInfo->globalGameInfo->teams[1].batterOrder, menuData.team2_batting_order, sizeof(menuData.team2_batting_order));
	}

	menuInfo->mode = MENU_ENTRY_NORMAL;
	loadMutableWorldSettings(stateInfo);
}
