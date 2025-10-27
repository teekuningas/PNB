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



int initMainMenu(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, ResourceManager* rm, RenderState* rs)
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

	// TODO: Draw loading screen here

	resource_manager_load_all_menu_assets(rm);

	// Populate MenuData for legacy code
	menuData->arrowTexture = resource_manager_get_texture(rm, "data/textures/arrow.tga");
	// Load background for new front menu
	menuData->catcherTexture = resource_manager_get_texture(rm, "data/textures/catcher.tga");
	menuData->batterTexture = resource_manager_get_texture(rm, "data/textures/batter.tga");
	menuData->slotTexture = resource_manager_get_texture(rm, "data/textures/cup_tree_slot.tga");
	menuData->trophyTexture = resource_manager_get_texture(rm, "data/textures/menu_trophy.tga");
	menuData->team1Texture = resource_manager_get_texture(rm, "data/textures/team1.tga");
	menuData->team2Texture = resource_manager_get_texture(rm, "data/textures/team2.tga");
	menuData->team3Texture = resource_manager_get_texture(rm, "data/textures/team3.tga");
	menuData->team4Texture = resource_manager_get_texture(rm, "data/textures/team4.tga");
	menuData->team5Texture = resource_manager_get_texture(rm, "data/textures/team5.tga");
	menuData->team6Texture = resource_manager_get_texture(rm, "data/textures/team6.tga");
	menuData->team7Texture = resource_manager_get_texture(rm, "data/textures/team7.tga");
	menuData->team8Texture = resource_manager_get_texture(rm, "data/textures/team8.tga");

	menuData->planeDisplayList = resource_manager_get_model(rm, "data/models/plane.obj");
	menuData->batDisplayList = resource_manager_get_model(rm, "data/models/hutunkeitto_bat.obj");
	menuData->handDisplayList = resource_manager_get_model(rm, "data/models/hutunkeitto_hand.obj");

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
void drawMainMenu(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, double alpha, ResourceManager* rm, RenderState* rs)
{
	if (menuData->stage == MENU_STAGE_FRONT) {
		// New orthographic-only rendering for front menu
		drawFrontMenu(&menuData->front_menu, rs, rm, menuData);
	} else {
		// Legacy rendering path for all other menus (disable lighting)
		begin_3d_render(rs);
		glDisable(GL_LIGHTING);
		gluLookAt(menuData->cam.x, menuData->cam.y, menuData->cam.z, menuData->look.x, menuData->look.y, menuData->look.z, menuData->up.x, menuData->up.y, menuData->up.z);
		switch(menuData->stage) {
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
		default:
			// Should not happen, but good to have a default
			break;
		}
	}
}
// values here are mostly hard-coded, some of them have defines, some dont. do i care ;_;
// most of the repeating stuff has some defines though.

int cleanMainMenu(MenuData* menuData)
{
	// All resources are now cleaned up by the resource manager.
	return 0;
}
