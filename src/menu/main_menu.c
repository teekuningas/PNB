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
#include "homerun_contest_menu.h"
#include "front_menu.h"
#include "game_over_menu.h"
#include "help_menu.h"
#include "menu_helpers.h"
#include "loading_screen_menu.h"
#include "cup_menu.h"

#define FIGURE_SCALE 0.4f
#define FRONT_ARROW_POS 0.15f
#define CAM_HEIGHT 2.3f



int initMainMenu(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo)
{
	menuInfo->mode = MENU_ENTRY_NORMAL;
	menuData->cam.x = 0.0f;
	menuData->cam.y = CAM_HEIGHT;
	menuData->cam.z = 0.0f;
	menuData->up.x = 0.0f;
	menuData->up.y = 0.0f;
	menuData->up.z = -1.0f;
	menuData->look.x = 0.0f;
	menuData->look.y = 0.0f;
	menuData->look.z = 0.0f;
	if(tryLoadingTextureGL(&menuData->arrowTexture, "data/textures/arrow.tga", "arrow") != 0) return -1;
	if(tryLoadingTextureGL(&menuData->catcherTexture, "data/textures/catcher.tga", "catcher") != 0) return -1;
	if(tryLoadingTextureGL(&menuData->batterTexture, "data/textures/batter.tga", "batter") != 0) return -1;
	if(tryLoadingTextureGL(&menuData->slotTexture, "data/textures/cup_tree_slot.tga", "slot") != 0) return -1;
	if(tryLoadingTextureGL(&menuData->trophyTexture, "data/textures/menu_trophy.tga", "trophy") != 0) return -1;
	if(tryLoadingTextureGL(&menuData->team1Texture, "data/textures/team1.tga", "team1") != 0) return -1;
	if(tryLoadingTextureGL(&menuData->team2Texture, "data/textures/team2.tga", "team2") != 0) return -1;
	if(tryLoadingTextureGL(&menuData->team3Texture, "data/textures/team3.tga", "team3") != 0) return -1;
	if(tryLoadingTextureGL(&menuData->team4Texture, "data/textures/team4.tga", "team4") != 0) return -1;
	if(tryLoadingTextureGL(&menuData->team5Texture, "data/textures/team5.tga", "team5") != 0) return -1;
	if(tryLoadingTextureGL(&menuData->team6Texture, "data/textures/team6.tga", "team6") != 0) return -1;
	if(tryLoadingTextureGL(&menuData->team7Texture, "data/textures/team7.tga", "team7") != 0) return -1;
	if(tryLoadingTextureGL(&menuData->team8Texture, "data/textures/team8.tga", "team8") != 0) return -1;
	menuData->planeMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/plane.obj", "Plane", menuData->planeMesh, &menuData->planeDisplayList) != 0) return -1;
	menuData->batMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/hutunkeitto_bat.obj", "Sphere.001", menuData->batMesh, &menuData->batDisplayList) != 0) return -1;
	menuData->handMesh = (MeshObject *)malloc ( sizeof(MeshObject));
	if(tryPreparingMeshGL("data/models/hutunkeitto_hand.obj", "Cube.001", menuData->handMesh, &menuData->handDisplayList) != 0) return -1;

	return 0;
}

void updateMainMenu(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, KeyStates* keyStates)
{
	MenuStage nextStage;
	if(stateInfo->changeScreen == 1) {
		stateInfo->changeScreen = 0;
		stateInfo->updated = 1;
		int i;
		switch (menuInfo->mode) {
		case MENU_ENTRY_NORMAL:
			resetMenuForNewGame(menuData, stateInfo);
			break;
		case MENU_ENTRY_INTER_PERIOD:
			initBattingOrderState(&menuData->batting_order, menuData->team1, menuData->team1_control);
			menuData->stage = MENU_STAGE_BATTING_ORDER_1;
			for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
				menuData->team1_batting_order[i] = i;
				menuData->team2_batting_order[i] = i;
			}
			break;
		case MENU_ENTRY_SUPER_INNING:
			initBattingOrderState(&menuData->batting_order, menuData->team1, menuData->team1_control);
			menuData->stage = MENU_STAGE_BATTING_ORDER_1;
			for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
				menuData->team1_batting_order[i] = i;
				menuData->team2_batting_order[i] = i;
			}
			break;
		case MENU_ENTRY_HOMERUN_CONTEST: {
			// initialize both teams' home-run contest states
			int totalPicks = (stateInfo->globalGameInfo->period == 4) ? 10 : 6;
			int team1Index = stateInfo->globalGameInfo->teams[0].value - 1;
			int team2Index = stateInfo->globalGameInfo->teams[1].value - 1;
			initHomerunContestState(&menuData->homerun1,
			                        team1Index,
			                        menuData->team1_control,
			                        totalPicks);
			initHomerunContestState(&menuData->homerun2,
			                        team2Index,
			                        menuData->team2_control,
			                        totalPicks);
			menuData->stage = MENU_STAGE_HOMERUN_CONTEST_1;
		}
		break;
		case MENU_ENTRY_GAME_OVER:
			menuData->stage = MENU_STAGE_GAME_OVER;
			break;
		}
		if(menuInfo->mode != MENU_ENTRY_GAME_OVER) {
			stateInfo->playSoundEffect = SOUND_MENU;
		}
	}
	// main main menu.
	switch (menuData->stage) {
	case MENU_STAGE_FRONT: {
		nextStage = updateFrontMenu(&menuData->front_menu, keyStates, stateInfo);
		if (nextStage == MENU_STAGE_CUP) {
			initCupMenu(&menuData->cup_menu, stateInfo);
		} else if (nextStage == MENU_STAGE_TEAM_SELECTION) {
			stateInfo->globalGameInfo->isCupGame = 0;
			initTeamSelectionState(&menuData->team_selection);
			menuData->team_selection.rem = stateInfo->numTeams;
			menuData->team_selection.pointer = DEFAULT_TEAM_1;
		} else if (nextStage == MENU_STAGE_HELP) {
			initHelpMenu(&menuData->help_menu);
		} else if (nextStage == MENU_STAGE_QUIT) {
			stateInfo->screen = -1;
		}
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_TEAM_SELECTION: {
		nextStage = updateTeamSelectionMenu(&menuData->team_selection, stateInfo, keyStates);
		if (nextStage != menuData->stage) {
			// Copy chosen teams back to main state
			menuData->team1 = menuData->team_selection.team1;
			menuData->team2 = menuData->team_selection.team2;
			menuData->team1_control = menuData->team_selection.team1_controller;
			menuData->team2_control = menuData->team_selection.team2_controller;
			menuData->inningsInPeriod = menuData->team_selection.innings;
			// Prepare next screen
			if (nextStage == MENU_STAGE_BATTING_ORDER_1) {
				initBattingOrderState(&menuData->batting_order, menuData->team1, menuData->team1_control);
			} else if (nextStage == MENU_STAGE_FRONT) {
				initFrontMenuState(&menuData->front_menu);
			}
		}
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_BATTING_ORDER_1: {
		nextStage = updateBattingOrderMenu(&menuData->batting_order, keyStates, menuData->stage, menuInfo->mode);
		if (nextStage != menuData->stage) {
			// Save team1's batting order
			memcpy(menuData->team1_batting_order, menuData->batting_order.batting_order,
			       sizeof(menuData->batting_order.batting_order));
			if (nextStage == MENU_STAGE_BATTING_ORDER_2) {
				// Setup for team2 ordering
				initBattingOrderState(&menuData->batting_order, menuData->team2, menuData->team2_control);
			} else if (nextStage == MENU_STAGE_HUTUNKEITTO) {
				initHutunkeittoState(&menuData->hutunkeitto);
				menuData->pointer = 0;
				menuData->rem = 2;
			}
		}
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_BATTING_ORDER_2: {
		nextStage = updateBattingOrderMenu(&menuData->batting_order, keyStates, menuData->stage, menuInfo->mode);
		if (nextStage != menuData->stage) {
			// Save team2's batting order
			memcpy(menuData->team2_batting_order, menuData->batting_order.batting_order,
			       sizeof(menuData->batting_order.batting_order));
			if (nextStage == MENU_STAGE_HUTUNKEITTO) {
				initHutunkeittoState(&menuData->hutunkeitto);
				menuData->pointer = 0;
				menuData->rem = 2;
			}
			if (nextStage == MENU_STAGE_GO_TO_GAME) {
				GameSetup gameSetup;
				createGameSetup(&gameSetup, menuData, menuInfo);
				initializeGameFromMenu(stateInfo, &gameSetup);
				menuInfo->mode = MENU_ENTRY_NORMAL;
			}
		}
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_HUTUNKEITTO: {
		nextStage = updateHutunkeittoMenu(&menuData->hutunkeitto, keyStates,
		                                  menuData->team1_control, menuData->team2_control);
		if (nextStage != menuData->stage) {
			// Record who bats first
			menuData->playsFirst = menuData->hutunkeitto.playsFirst;
			if (nextStage == MENU_STAGE_GO_TO_GAME) {
				GameSetup gameSetup;
				createGameSetup(&gameSetup, menuData, menuInfo);
				initializeGameFromMenu(stateInfo, &gameSetup);
				menuInfo->mode = MENU_ENTRY_NORMAL;
			}
		}
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_GAME_OVER: {
		// Delegate Game-Over logic
		nextStage = updateGameOverMenu(menuData, stateInfo, keyStates, menuInfo);
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_HOMERUN_CONTEST_1: {
		// Delegate to home-run contest logic for team 1
		nextStage = updateHomerunContestMenu(&menuData->homerun1,
		                                     keyStates,
		                                     stateInfo,
		                                     MENU_STAGE_HOMERUN_CONTEST_1);
		if (nextStage != menuData->stage) {
			menuData->stage = nextStage;
		}
		break;
	}
	case MENU_STAGE_HOMERUN_CONTEST_2: {
		// Delegate to home-run contest logic for team 2
		nextStage = updateHomerunContestMenu(&menuData->homerun2,
		                                     keyStates,
		                                     stateInfo,
		                                     MENU_STAGE_HOMERUN_CONTEST_2);
		if (nextStage != menuData->stage) {
			if (nextStage == MENU_STAGE_GO_TO_GAME) {
				GameSetup gameSetup;
				createGameSetup(&gameSetup, menuData, menuInfo);
				initializeGameFromMenu(stateInfo, &gameSetup);
				menuInfo->mode = MENU_ENTRY_NORMAL;
			}
			menuData->stage = nextStage;
		}
		break;
	}
	case MENU_STAGE_CUP: {
		nextStage = updateCupMenu(&menuData->cup_menu, stateInfo, menuData, keyStates);
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_HELP: {
		nextStage = updateHelpMenu(&menuData->help_menu, keyStates);
		if (nextStage != menuData->stage) {
			if (nextStage == MENU_STAGE_FRONT) {
				initFrontMenuState(&menuData->front_menu);
			}
		}
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_GO_TO_GAME:
		break;
	case MENU_STAGE_QUIT:
		// Quit handled in FRONT case, no update here
		break;
	}
}

// here we draw everything.
// directly we draw ugly stuff like images of players or hand or bat models etc.
// then we call methods to handle text rendering.
void drawMainMenu(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, double alpha)
{
	gluLookAt(menuData->cam.x, menuData->cam.y, menuData->cam.z, menuData->look.x, menuData->look.y, menuData->look.z, menuData->up.x, menuData->up.y, menuData->up.z);
	switch(menuData->stage) {
	case MENU_STAGE_FRONT:
		drawFrontMenu(&menuData->front_menu, menuData);
		break;
	case MENU_STAGE_TEAM_SELECTION:
		drawTeamSelectionMenu(&menuData->team_selection, stateInfo, menuData);
		break;
	case MENU_STAGE_BATTING_ORDER_1:
	case MENU_STAGE_BATTING_ORDER_2:
		drawBattingOrderMenu(&menuData->batting_order, stateInfo, menuData);
		break;
	case MENU_STAGE_HUTUNKEITTO:
		drawHutunkeittoMenu(&menuData->hutunkeitto, menuData);
		break;
	case MENU_STAGE_GAME_OVER:
		drawGameOverMenu(stateInfo);
		break;
	case MENU_STAGE_HOMERUN_CONTEST_1:
		drawHomerunContestMenu(&menuData->homerun1, stateInfo, menuData);
		break;
	case MENU_STAGE_HOMERUN_CONTEST_2:
		drawHomerunContestMenu(&menuData->homerun2, stateInfo, menuData);
		break;
	case MENU_STAGE_CUP:
		drawCupMenu(&menuData->cup_menu, stateInfo, menuData);
		break;
	case MENU_STAGE_HELP:
		drawHelpMenu(&menuData->help_menu);
		break;
	case MENU_STAGE_GO_TO_GAME:
		break;
	case MENU_STAGE_QUIT:
		// Nothing to draw when quitting
		break;
	}
}
// values here are mostly hard-coded, some of them have defines, some dont. do i care ;_;
// most of the repeating stuff has some defines though.

int cleanMainMenu(MenuData* menuData)
{
	cleanMesh(menuData->planeMesh);
	cleanMesh(menuData->handMesh);
	cleanMesh(menuData->batMesh);
	return 0;
}


