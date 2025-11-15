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
#include "input.h"

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
			// When returning to a game, team info is in globalGameInfo
			initBattingOrderState(&menuData->batting_order, stateInfo->globalGameInfo->teams[0].value - 1, stateInfo->globalGameInfo->teams[0].control, stateInfo);
			menuData->stage = MENU_STAGE_BATTING_ORDER_1;
			for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
				menuData->pendingGameSetup.team1_batting_order[i] = i;
				menuData->pendingGameSetup.team2_batting_order[i] = i;
			}
			break;
		case MENU_ENTRY_SUPER_INNING:
			// When returning to a game, team info is in globalGameInfo
			initBattingOrderState(&menuData->batting_order, stateInfo->globalGameInfo->teams[0].value - 1, stateInfo->globalGameInfo->teams[0].control, stateInfo);
			menuData->stage = MENU_STAGE_BATTING_ORDER_1;
			for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
				menuData->pendingGameSetup.team1_batting_order[i] = i;
				menuData->pendingGameSetup.team2_batting_order[i] = i;
			}
			break;
		case MENU_ENTRY_HOMERUN_CONTEST: {
			// initialize both teams' home-run contest states
			int totalPicks = (stateInfo->globalGameInfo->period == 4) ? 10 : 6;
			int team1Index = stateInfo->globalGameInfo->teams[0].value - 1;
			int team2Index = stateInfo->globalGameInfo->teams[1].value - 1;
			initHomerunContestState(&menuData->homerun1,
			                        team1Index,
			                        stateInfo->globalGameInfo->teams[0].control,
			                        totalPicks);
			initHomerunContestState(&menuData->homerun2,
			                        team2Index,
			                        stateInfo->globalGameInfo->teams[1].control,
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
		nextStage = updateTeamSelectionMenu(&menuData->team_selection, keyStates, &menuData->pendingGameSetup);
		if (nextStage != menuData->stage) {
			// Prepare next screen
			if (nextStage == MENU_STAGE_BATTING_ORDER_1) {
				initBattingOrderState(&menuData->batting_order, menuData->pendingGameSetup.team1, menuData->pendingGameSetup.team1_control, stateInfo);
			} else if (nextStage == MENU_STAGE_FRONT) {
				initFrontMenuState(&menuData->front_menu);
			}
		}
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_BATTING_ORDER_1: {
		nextStage = updateBattingOrderMenu(&menuData->batting_order, keyStates, menuData->stage, menuInfo->mode, &menuData->pendingGameSetup);
		if (nextStage != menuData->stage) {
			if (nextStage == MENU_STAGE_BATTING_ORDER_2) {
				// Setup for team2 ordering
				initBattingOrderState(&menuData->batting_order, menuData->pendingGameSetup.team2, menuData->pendingGameSetup.team2_control, stateInfo);
			} else if (nextStage == MENU_STAGE_HUTUNKEITTO) {
				initHutunkeittoState(&menuData->hutunkeitto);
			}
		}
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_BATTING_ORDER_2: {
		nextStage = updateBattingOrderMenu(&menuData->batting_order, keyStates, menuData->stage, menuInfo->mode, &menuData->pendingGameSetup);
		if (nextStage != menuData->stage) {
			if (nextStage == MENU_STAGE_HUTUNKEITTO) {
				initHutunkeittoState(&menuData->hutunkeitto);
			}
			if (nextStage == MENU_STAGE_GO_TO_GAME) {
				if (menuInfo->mode == MENU_ENTRY_INTER_PERIOD || menuInfo->mode == MENU_ENTRY_SUPER_INNING) {
					menuData->pendingGameSetup.launchType = GAME_LAUNCH_RETURN_INTER_PERIOD;
					launchGameFromMenu(stateInfo, &menuData->pendingGameSetup);
				} else {
					menuData->pendingGameSetup.gameMode = GAME_MODE_NORMAL;
					menuData->pendingGameSetup.launchType = GAME_LAUNCH_NEW;
					launchGameFromMenu(stateInfo, &menuData->pendingGameSetup);
				}
			}
		}
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_HUTUNKEITTO: {
		nextStage = updateHutunkeittoMenu(&menuData->hutunkeitto, keyStates,
		                                  menuData->pendingGameSetup.team1_control, menuData->pendingGameSetup.team2_control, &menuData->pendingGameSetup);
		if (nextStage != menuData->stage) {
			if (nextStage == MENU_STAGE_GO_TO_GAME) {
				menuData->pendingGameSetup.gameMode = GAME_MODE_NORMAL;
				menuData->pendingGameSetup.launchType = GAME_LAUNCH_NEW;
				launchGameFromMenu(stateInfo, &menuData->pendingGameSetup);
				menuInfo->mode = MENU_ENTRY_NORMAL;
			}
		}
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_GAME_OVER: {
		nextStage = updateGameOverMenu(stateInfo->gameConclusion, keyStates, stateInfo->globalGameInfo->teams[0].control, stateInfo->globalGameInfo->teams[1].control);
		if (nextStage != menuData->stage) {
			stateInfo->playSoundEffect = SOUND_MENU;
			if (nextStage == MENU_STAGE_CUP) {
				// 1. Find the schedule slot that was just played using the correct data source.
				int scheduleSlot = -1;
				int i, j;
				for (i = 0; i < 4; i++) {
					for (j = 0; j < 2; j++) {
						if (stateInfo->tournamentState->cupInfo.schedule[i][j] == stateInfo->tournamentState->cupInfo.userTeamIndexInTree) {
							scheduleSlot = i;
						}
					}
				}

				// If a match was found, process the result.
				if (scheduleSlot != -1) {
					// 2. Determine which team in the schedule (slot 0 or 1) won the match.
					// Get the actual team ID of the winner from the (still valid) globalGameInfo.
					int winningTeamId = stateInfo->globalGameInfo->teams[stateInfo->gameConclusion->winner].value - 1;

					// Get the team ID for the first slot of the match that was played.
					int teamIdInScheduleSlot0 = stateInfo->tournamentState->cupInfo.cupTeamIndexTree[stateInfo->tournamentState->cupInfo.schedule[scheduleSlot][0]];

					int winningSlotInSchedule;
					if (winningTeamId == teamIdInScheduleSlot0) {
						winningSlotInSchedule = 0;
					} else {
						winningSlotInSchedule = 1;
					}

					// 3. Call the update function with the correct parameters.
					updateCupTreeAfterDay(stateInfo->tournamentState, stateInfo, scheduleSlot, winningSlotInSchedule);
					updateSchedule(stateInfo->tournamentState, stateInfo);

					// 4. Re-initialize the cup menu to reflect the updated state.
					initCupMenu(&menuData->cup_menu, stateInfo);

					// 5. Check if the user won the entire cup and set the screen to the trophy/credits screen.
					if (stateInfo->tournamentState->cupInfo.winnerIndex != -1) {
						int winnerControl = stateInfo->globalGameInfo->teams[stateInfo->gameConclusion->winner].control;
						if (winnerControl == 0 || winnerControl == 1) { // Human player
							menuData->cup_menu.screen = CUP_MENU_SCREEN_END_CREDITS;
						}
					}
				}
			} else {
				resetMenuForNewGame(menuData, stateInfo);
			}
		}
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_HOMERUN_CONTEST_1: {
		nextStage = updateHomerunContestMenu(&menuData->homerun1,
		                                     keyStates,
		                                     menuData->stage,
		                                     &menuData->pendingGameSetup,
		                                     (const TeamData*)stateInfo->teamData);
		if (nextStage != menuData->stage) {
			menuData->stage = nextStage;
		}
		break;
	}
	case MENU_STAGE_HOMERUN_CONTEST_2: {
		nextStage = updateHomerunContestMenu(&menuData->homerun2,
		                                     keyStates,
		                                     menuData->stage,
		                                     &menuData->pendingGameSetup,
		                                     (const TeamData*)stateInfo->teamData);
		if (nextStage != menuData->stage) {
			if (nextStage == MENU_STAGE_GO_TO_GAME) {
				menuData->pendingGameSetup.launchType = GAME_LAUNCH_RETURN_HOMERUN_CONTEST;
				launchGameFromMenu(stateInfo, &menuData->pendingGameSetup);
			}
			menuData->stage = nextStage;
		}
		break;
	}
	case MENU_STAGE_CUP: {
		CupMenuOutput cup_output;
		nextStage = updateCupMenu(&menuData->cup_menu, stateInfo, keyStates, &cup_output);
		if (nextStage == MENU_STAGE_BATTING_ORDER_1) {
			// A game is starting, transfer data from cup output to pendingGameSetup
			menuData->pendingGameSetup.team1 = cup_output.team1;
			menuData->pendingGameSetup.team2 = cup_output.team2;
			menuData->pendingGameSetup.team1_control = cup_output.team1_control;
			menuData->pendingGameSetup.team2_control = cup_output.team2_control;
			menuData->pendingGameSetup.halfInningsInPeriod = cup_output.innings;
			stateInfo->globalGameInfo->isCupGame = 1;
			initBattingOrderState(&menuData->batting_order, menuData->pendingGameSetup.team1, menuData->pendingGameSetup.team1_control, stateInfo);
		} else if (nextStage == MENU_STAGE_FRONT) {
			stateInfo->globalGameInfo->isCupGame = 0;
			stateInfo->tournamentState->cupInfo.userTeamIndexInTree = -1;
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
	clearReleasedKeys(keyStates);
}

// here we draw everything.
void drawMainMenu(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, double alpha, ResourceManager* rm, RenderState* rs)
{
	switch(menuData->stage) {
	case MENU_STAGE_FRONT:
		drawFrontMenu(&menuData->front_menu, rs, rm);
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
		drawHutunkeittoMenu(&menuData->hutunkeitto, rs, rm, menuData->pendingGameSetup.team1, menuData->pendingGameSetup.team2);
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
		drawCupMenu(&menuData->cup_menu, stateInfo, rs, rm);
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
