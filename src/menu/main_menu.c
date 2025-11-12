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

	resource_manager_load_all_menu_assets(rm);

	// Populate MenuData for legacy code
	menuData->arrowTexture = resource_manager_get_texture(rm, "data/textures/arrow.tga");
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
			initBattingOrderState(&menuData->batting_order, menuData->team1, menuData->team1_control, stateInfo);
			menuData->stage = MENU_STAGE_BATTING_ORDER_1;
			for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
				menuData->team1_batting_order[i] = i;
				menuData->team2_batting_order[i] = i;
			}
			break;
		case MENU_ENTRY_SUPER_INNING:
			initBattingOrderState(&menuData->batting_order, menuData->team1, menuData->team1_control, stateInfo);
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
		// No sound on game over screen, it's played on exit
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
			initTeamSelectionState(&menuData->team_selection, stateInfo->numTeams);
		} else if (nextStage == MENU_STAGE_HELP) {
			initHelpMenu(&menuData->help_menu);
		} else if (nextStage == MENU_STAGE_QUIT) {
			stateInfo->screen = LOADING_SCREEN;
		}
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_TEAM_SELECTION: {
		TeamSelectionMenuOutput team_selection_output;
		nextStage = updateTeamSelectionMenu(&menuData->team_selection, keyStates, &team_selection_output);
		if (nextStage != menuData->stage) {
			// Copy chosen teams back to main state
			menuData->team1 = team_selection_output.team1;
			menuData->team2 = team_selection_output.team2;
			menuData->team1_control = team_selection_output.team1_controller;
			menuData->team2_control = team_selection_output.team2_controller;
			menuData->halfInningsInPeriod = team_selection_output.innings;
			// Prepare next screen
			if (nextStage == MENU_STAGE_BATTING_ORDER_1) {
				initBattingOrderState(&menuData->batting_order, menuData->team1, menuData->team1_control, stateInfo);
			} else if (nextStage == MENU_STAGE_FRONT) {
				initFrontMenuState(&menuData->front_menu);
			}
		}
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_BATTING_ORDER_1: {
		BattingOrderMenuOutput output;
		nextStage = updateBattingOrderMenu(&menuData->batting_order, keyStates, menuData->stage, menuInfo->mode, &output);
		if (nextStage != menuData->stage) {
			// Save team1's batting order
			memcpy(menuData->team1_batting_order, output.batting_order,
			       sizeof(output.batting_order));
			if (nextStage == MENU_STAGE_BATTING_ORDER_2) {
				// Setup for team2 ordering
				initBattingOrderState(&menuData->batting_order, menuData->team2, menuData->team2_control, stateInfo);
			} else if (nextStage == MENU_STAGE_HUTUNKEITTO) {
				initHutunkeittoState(&menuData->hutunkeitto);
			}
		}
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_BATTING_ORDER_2: {
		BattingOrderMenuOutput output;
		nextStage = updateBattingOrderMenu(&menuData->batting_order, keyStates, menuData->stage, menuInfo->mode, &output);
		if (nextStage != menuData->stage) {
			// Save team2's batting order
			memcpy(menuData->team2_batting_order, output.batting_order,
			       sizeof(output.batting_order));
			if (nextStage == MENU_STAGE_HUTUNKEITTO) {
				initHutunkeittoState(&menuData->hutunkeitto);
			}
			if (nextStage == MENU_STAGE_GO_TO_GAME) {
				if (menuInfo->mode == MENU_ENTRY_INTER_PERIOD || menuInfo->mode == MENU_ENTRY_SUPER_INNING) {
					returnToGame(stateInfo);
				} else {
					GameSetup gameSetup;
					createGameSetup(&gameSetup, menuData, menuInfo);
					initializeGameFromMenu(stateInfo, &gameSetup);
				}
			}
		}
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_HUTUNKEITTO: {
		HutunkeittoMenuOutput hutunkeitto_output;
		nextStage = updateHutunkeittoMenu(&menuData->hutunkeitto, keyStates,
		                                  menuData->team1_control, menuData->team2_control, &hutunkeitto_output);
		if (nextStage != menuData->stage) {
			// Record who bats first
			menuData->playsFirst = hutunkeitto_output.playsFirst;
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
		nextStage = updateGameOverMenu(stateInfo->gameConclusion, keyStates, menuData->team1_control, menuData->team2_control);
		if (nextStage != menuData->stage) {
			stateInfo->playSoundEffect = SOUND_MENU;
			if (nextStage == MENU_STAGE_CUP) {
				// Enrich the thin gameConclusion with tournament metadata before processing
				stateInfo->gameConclusion->userTeamIndexInTree = stateInfo->tournamentState->cupInfo.userTeamIndexInTree;
				stateInfo->gameConclusion->gameStructure = stateInfo->tournamentState->cupInfo.gameStructure;
				stateInfo->gameConclusion->dayCount = stateInfo->tournamentState->cupInfo.dayCount;
				for (int i = 0; i < SLOT_COUNT; i++) {
					stateInfo->gameConclusion->slotWins[i] = stateInfo->tournamentState->cupInfo.slotWins[i];
				}

				int i, j;
				int scheduleSlot = -1;
				int playerWon = 0;
				for (i = 0; i < 4; i++) {
					for (j = 0; j < 2; j++) {
						if (stateInfo->tournamentState->cupInfo.schedule[i][j] == stateInfo->gameConclusion->userTeamIndexInTree) {
							scheduleSlot = i;
							if (j == stateInfo->gameConclusion->winner) playerWon = 1;
						}
					}
				}
				if (playerWon == 1) {
					int advance = 0;
					if (stateInfo->gameConclusion->gameStructure == 0) {
						if (stateInfo->gameConclusion->slotWins[stateInfo->gameConclusion->userTeamIndexInTree] == 2) {
							if (stateInfo->gameConclusion->dayCount >= 11) advance = 1;
						}
					} else {
						if (stateInfo->gameConclusion->slotWins[stateInfo->gameConclusion->userTeamIndexInTree] == 0) {
							if (stateInfo->gameConclusion->dayCount >= 3) advance = 1;
						}
					}
					if (advance == 1) menuData->cup_menu.screen = CUP_MENU_SCREEN_END_CREDITS;
				}
				updateCupTreeAfterDay(stateInfo->tournamentState, stateInfo, scheduleSlot, stateInfo->gameConclusion->winner);
				updateSchedule(stateInfo->tournamentState, stateInfo);
				initCupMenu(&menuData->cup_menu, stateInfo);
			} else {
				resetMenuForNewGame(menuData, stateInfo);
			}
		}
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_HOMERUN_CONTEST_1: {
		HomerunContestMenuOutput output;
		nextStage = updateHomerunContestMenu(&menuData->homerun1,
		                                     keyStates,
		                                     menuData->stage,
		                                     &output,
		                                     (const TeamData*)stateInfo->teamData);
		if (nextStage != menuData->stage) {
			memcpy(menuData->homerun_choices1, output.choices, sizeof(output.choices));
			menuData->stage = nextStage;
		}
		break;
	}
	case MENU_STAGE_HOMERUN_CONTEST_2: {
		HomerunContestMenuOutput output;
		nextStage = updateHomerunContestMenu(&menuData->homerun2,
		                                     keyStates,
		                                     menuData->stage,
		                                     &output,
		                                     (const TeamData*)stateInfo->teamData);
		if (nextStage != menuData->stage) {
			if (nextStage == MENU_STAGE_GO_TO_GAME) {
				memcpy(menuData->homerun_choices2, output.choices, sizeof(output.choices));

				// Update batter/runner selections from menu
				int pairCount = menuData->homerun1.choiceCount / 2;
				for (int i = 0; i < 2; i++) {
					for (int j = 0; j < pairCount; j++) {
						stateInfo->globalGameInfo->teams[0].batterRunnerIndices[i][j] = menuData->homerun_choices1[i][j];
						stateInfo->globalGameInfo->teams[1].batterRunnerIndices[i][j] = menuData->homerun_choices2[i][j];
					}
				}
				stateInfo->globalGameInfo->pairCount = pairCount;
				stateInfo->localGameInfo->gAI.runnerBatterPairCounter = 0;

				returnToGame(stateInfo);
			}
			menuData->stage = nextStage;
		}
		break;
	}
	case MENU_STAGE_CUP: {
		CupMenuOutput cup_output;
		nextStage = updateCupMenu(&menuData->cup_menu, stateInfo, keyStates, &cup_output);
		if (nextStage == MENU_STAGE_BATTING_ORDER_1) {
			// A game is starting, transfer data from cup output to menuData
			menuData->team1 = cup_output.team1;
			menuData->team2 = cup_output.team2;
			menuData->team1_control = cup_output.team1_control;
			menuData->team2_control = cup_output.team2_control;
			menuData->halfInningsInPeriod = cup_output.innings;
			stateInfo->globalGameInfo->isCupGame = 1;
			initBattingOrderState(&menuData->batting_order, menuData->team1, menuData->team1_control, stateInfo);
		}
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
void drawMainMenu(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, double alpha, ResourceManager* rm, RenderState* rs)
{
	switch(menuData->stage) {
	case MENU_STAGE_FRONT:
		drawFrontMenu(&menuData->front_menu, rs, rm, menuData);
		break;
	case MENU_STAGE_HELP:
		drawHelpMenu(&menuData->help_menu, rs, rm);
		break;
	case MENU_STAGE_BATTING_ORDER_1:
	case MENU_STAGE_BATTING_ORDER_2:
		drawBattingOrderMenu(&menuData->batting_order, menuData->stage, rs, rm);
		break;
	case MENU_STAGE_TEAM_SELECTION:
		drawTeamSelectionMenu(&menuData->team_selection, (const TeamData*)stateInfo->teamData, rs, rm);
		break;
	case MENU_STAGE_HUTUNKEITTO:
		drawHutunkeittoMenu(&menuData->hutunkeitto, rs, rm, menuData->team1, menuData->team2);
		break;

	case MENU_STAGE_GAME_OVER:
		drawGameOverMenu(stateInfo->gameConclusion, (const TeamData*)stateInfo->teamData, rs, rm);
		break;
	case MENU_STAGE_HOMERUN_CONTEST_1:
		drawHomerunContestMenu(&menuData->homerun1, (const RenderState*)rs, rm, (const TeamData*)stateInfo->teamData);
		break;
	case MENU_STAGE_HOMERUN_CONTEST_2:
		drawHomerunContestMenu(&menuData->homerun2, (const RenderState*)rs, rm, (const TeamData*)stateInfo->teamData);
		break;
	case MENU_STAGE_CUP:
		begin_3d_render(rs);
		glDisable(GL_LIGHTING);
		gluLookAt(menuData->cam.x, menuData->cam.y, menuData->cam.z, menuData->look.x, menuData->look.y, menuData->look.z, menuData->up.x, menuData->up.y, menuData->up.z);
		drawCupMenu(&menuData->cup_menu, stateInfo, menuData);
		break;

	case MENU_STAGE_GO_TO_GAME:
	case MENU_STAGE_QUIT:
		// Nothing to draw
		break;

	}
}

int cleanMainMenu(MenuData* menuData)
{
	// All resources are now cleaned up by the resource manager.
	return 0;
}
