#include "menu_helpers.h"
#include "resource_manager.h"
#include "render.h"
#include "front_menu.h"

// Draws a full-screen 2D background quad for menus
// Uses the "empty_background" texture from ResourceManager
void drawMenuLayout2D(ResourceManager* rm, const RenderState* rs)
{
	// Bind the shared empty background texture
	GLuint tex = resource_manager_get_texture(rm, "data/textures/empty_background.tga");
	glBindTexture(GL_TEXTURE_2D, tex);
	glBegin(GL_QUADS);
	// TL
	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(0.0f, 0.0f);
	// BL
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(0.0f, rs->window_height);
	// BR
	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(rs->window_width, rs->window_height);
	// TR
	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(rs->window_width, 0.0f);
	glEnd();
}

void createGameSetup(GameSetup* gameSetup, MenuData* menuData, MenuInfo* menuInfo)
{
	// Set game mode based on menu entry mode
	switch (menuInfo->mode) {
	case MENU_ENTRY_NORMAL:
		gameSetup->gameMode = GAME_MODE_NORMAL;
		break;
	case MENU_ENTRY_SUPER_INNING:
		gameSetup->gameMode = GAME_MODE_SUPER_INNING;
		break;
	case MENU_ENTRY_HOMERUN_CONTEST:
		gameSetup->gameMode = GAME_MODE_HOMERUN_CONTEST;
		break;
	default:
		// Default to normal, though this shouldn't be reached in normal flow
		gameSetup->gameMode = GAME_MODE_NORMAL;
		break;
	}

	gameSetup->team1 = menuData->team1;
	gameSetup->team2 = menuData->team2;
	gameSetup->team1_control = menuData->team1_control;
	gameSetup->team2_control = menuData->team2_control;
	gameSetup->inningsInPeriod = menuData->inningsInPeriod;

	// Logic for playsFirst is now here
	if (menuInfo->mode == MENU_ENTRY_NORMAL || menuInfo->mode == MENU_ENTRY_SUPER_INNING) {
		gameSetup->playsFirst = menuData->playsFirst;
	} else {
		// Default or for other modes if needed
		gameSetup->playsFirst = 0;
	}

	memcpy(gameSetup->team1_batting_order, menuData->team1_batting_order, sizeof(menuData->team1_batting_order));
	memcpy(gameSetup->team2_batting_order, menuData->team2_batting_order, sizeof(menuData->team2_batting_order));

	if (menuInfo->mode == MENU_ENTRY_HOMERUN_CONTEST) {
		gameSetup->homerun_choice_count = menuData->homerun1.choiceCount / 2;
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < gameSetup->homerun_choice_count; j++) {
				gameSetup->homerun_choices1[i][j] = menuData->homerun1.choices[i][j];
				gameSetup->homerun_choices2[i][j] = menuData->homerun2.choices[i][j];
			}
		}
	} else {
		gameSetup->homerun_choice_count = 0;
	}
}
void resetMenuForNewGame(MenuData* menuData, StateInfo* stateInfo)
{
	int i;
	for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		menuData->team1_batting_order[i] = i;
		menuData->team2_batting_order[i] = i;
	}
	menuData->inningsInPeriod = 0;
	menuData->team1 = 0;
	menuData->team2 = 0;
	menuData->team1_control = 0;
	menuData->team2_control = 0;

	if (stateInfo->globalGameInfo->isCupGame != 1) {
		initFrontMenuState(&menuData->front_menu);
		menuData->stage = MENU_STAGE_FRONT;
	} else {
		menuData->stage = MENU_STAGE_CUP;
		menuData->cup_menu.screen = CUP_MENU_SCREEN_ONGOING;
		menuData->cup_menu.pointer = 0;
		menuData->cup_menu.rem = 5;
	}
}

void updateSchedule(TournamentState* tournamentState, StateInfo* stateInfo)
{
	int j;
	int counter = 0;
	for(j = 0; j < SLOT_COUNT/2; j++) {
		if(tournamentState->cupInfo.gameStructure == 0) {
			if(tournamentState->cupInfo.slotWins[j*2] < 3 && tournamentState->cupInfo.slotWins[j*2+1] < 3) {
				if(j < 4 || (j < 6 && tournamentState->cupInfo.dayCount >= 5) ||
				        (j == 6 && tournamentState->cupInfo.dayCount >= 10)) {
					tournamentState->cupInfo.schedule[counter][0] = j*2;
					tournamentState->cupInfo.schedule[counter][1] = j*2+1;
					counter++;
				}
			}
		} else {
			if(tournamentState->cupInfo.slotWins[j*2] < 1 && tournamentState->cupInfo.slotWins[j*2+1] < 1) {
				if(j < 4 || (j < 6 && tournamentState->cupInfo.dayCount >= 1) ||
				        (j == 6 && tournamentState->cupInfo.dayCount >= 2)) {
					tournamentState->cupInfo.schedule[counter][0] = j*2;
					tournamentState->cupInfo.schedule[counter][1] = j*2+1;
					counter++;
				}
			}
		}
	}
	for(j = counter; j < 4; j++) {
		tournamentState->cupInfo.schedule[j][0] = -1;
		tournamentState->cupInfo.schedule[j][1] = -1;
	}
}

void updateCupTreeAfterDay(TournamentState* tournamentState, StateInfo* stateInfo, int scheduleSlot, int winningSlot)
{
	int i;
	int counter = 0;
	int done = 0;
	while(done == 0 && counter < 4) {
		if(tournamentState->cupInfo.schedule[counter][0] != -1) {
			int team1Index = tournamentState->cupInfo.cupTeamIndexTree[(tournamentState->cupInfo.schedule[counter][0])];
			int team2Index = tournamentState->cupInfo.cupTeamIndexTree[(tournamentState->cupInfo.schedule[counter][1])];
			int index = 8 + tournamentState->cupInfo.schedule[counter][0] / 2;
			int team1Points = 0;
			int team2Points = 0;
			int random = rand()%100;
			int difference;
			int winningTeam;
			for(i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
				team1Points += stateInfo->teamData[team1Index].players[i].speed;
				team1Points += stateInfo->teamData[team1Index].players[i].power;
				team2Points += stateInfo->teamData[team2Index].players[i].speed;
				team2Points += stateInfo->teamData[team2Index].players[i].power;
			}
			difference = team2Points - team1Points;
			// update schedule and cup tree
			if(counter != scheduleSlot) {
				if(random + difference*3 >= 50) {
					winningTeam = 1;
				} else {
					winningTeam = 0;
				}
			} else {
				winningTeam = winningSlot;
			}
			tournamentState->cupInfo.slotWins[tournamentState->cupInfo.schedule[counter][winningTeam]] += 1;
			if(tournamentState->cupInfo.gameStructure == 0) {
				if(tournamentState->cupInfo.slotWins[tournamentState->cupInfo.schedule[counter][winningTeam]] == 3) {
					if(index < 14) {
						tournamentState->cupInfo.cupTeamIndexTree[index] =
						    tournamentState->cupInfo.cupTeamIndexTree[tournamentState->cupInfo.schedule[counter][winningTeam]];
						if(tournamentState->cupInfo.schedule[counter][winningTeam] == tournamentState->cupInfo.userTeamIndexInTree) {
							tournamentState->cupInfo.userTeamIndexInTree = index;
						}
					} else {
						tournamentState->cupInfo.winnerIndex = tournamentState->cupInfo.cupTeamIndexTree[tournamentState->cupInfo.schedule[counter][winningTeam]];
					}
				}
			} else {
				if(tournamentState->cupInfo.slotWins[tournamentState->cupInfo.schedule[counter][winningTeam]] == 1) {
					if(index < 14) {
						tournamentState->cupInfo.cupTeamIndexTree[index] =
						    tournamentState->cupInfo.cupTeamIndexTree[tournamentState->cupInfo.schedule[counter][winningTeam]];
						if(tournamentState->cupInfo.schedule[counter][winningTeam] == tournamentState->cupInfo.userTeamIndexInTree) {
							tournamentState->cupInfo.userTeamIndexInTree = index;
						}
					} else {
						tournamentState->cupInfo.winnerIndex = tournamentState->cupInfo.cupTeamIndexTree[tournamentState->cupInfo.schedule[counter][winningTeam]];
					}
				}
			}
			counter++;
		} else {
			done = 1;
		}
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
