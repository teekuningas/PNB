#include "globals.h"
#include "render.h"
#include "font.h"

#include "main_menu.h"
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
#include "cup.h"
#include "input.h"

#define CAM_HEIGHT 2.3f

int initMainMenu(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, ResourceManager* rm, RenderState* rs)
{
	menuInfo->mode = MENU_ENTRY_NORMAL;

	resource_manager_load_all_menu_assets(rm);

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
			stateInfo->screen = SCREEN_LOADING;
		}
		menuData->stage = nextStage;
		break;
	}
	case MENU_STAGE_TEAM_SELECTION: {
		nextStage = updateTeamSelectionMenu(&menuData->team_selection, keyStates, &menuData->pendingGameSetup);
		if (nextStage != menuData->stage) {
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
				// 1. Process the finished game to update the tournament's logical state.
				if (stateInfo->gameConclusion->isCupGame && stateInfo->cup != NULL) {
					cup_update_match_result(
					    stateInfo->cup,
					    stateInfo->currently_played_cup_match_index,
					    stateInfo->gameConclusion->winner
					);
					// Advance to next day after user's game
					cup_advance_day(stateInfo->cup);
				}

				// 2. Re-initialize the cup menu to reflect the updated state.
				initCupMenu(&menuData->cup_menu, stateInfo);

				// 3. Check if the user won the entire cup and set the screen to the trophy/credits screen.
				if (stateInfo->cup != NULL && stateInfo->cup->matches[0].winner_id == stateInfo->cup->user_team_id) {
					menuData->cup_menu.screen = CUP_MENU_SCREEN_END_CREDITS;
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
		break;

	}
}

int cleanMainMenu(MenuData* menuData)
{
	// All resources are now cleaned up by the resource manager.
	return 0;
}
