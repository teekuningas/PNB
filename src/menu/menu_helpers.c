#include "menu_helpers.h"
#include "front_menu.h"

void resetMenuForNewGame(MenuData* menuData, StateInfo* stateInfo)
{
	int i;
	glDisable(GL_LIGHTING);
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
		menuData->stage_8_state = 0;
	} else {
		menuData->stage = MENU_STAGE_CUP;
		menuData->rem = 5;
		menuData->pointer = 0;
		menuData->stage_8_state = 2;
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
