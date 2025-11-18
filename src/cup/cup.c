#include "cup.h"
#include <stdlib.h>

void updateSchedule(TournamentState* tournamentState, StateInfo* stateInfo)
{
	// If the cup is already won, there's nothing to schedule.
	if (tournamentState->cupInfo.winnerIndex != -1) {
		for (int i = 0; i < 4; i++) {
			tournamentState->cupInfo.schedule[i][0] = -1;
			tournamentState->cupInfo.schedule[i][1] = -1;
		}
		return;
	}

	int j;
	int counter = 0;
	for(j = 0; j < SLOT_COUNT/2; j++) {
		if(tournamentState->cupInfo.gameStructure == 0) {
			if(tournamentState->cupInfo.slotWins[j*2] < 3 && tournamentState->cupInfo.slotWins[j*2+1] < 3) {
				if(j < 4 || (j < 6 && tournamentState->cupInfo.dayCount >= 1) ||
				        (j == 6 && tournamentState->cupInfo.dayCount >= 2)) {
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
						tournamentState->cupInfo.winnerIndex =
						    tournamentState->cupInfo.cupTeamIndexTree[tournamentState->cupInfo.schedule[counter][winningTeam]];
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
						tournamentState->cupInfo.winnerIndex =
						    tournamentState->cupInfo.cupTeamIndexTree[tournamentState->cupInfo.schedule[counter][winningTeam]];
					}
				}
			}
			counter++;
		} else {
			done = 1;
		}
	}
}

void cup_process_finished_game(TournamentState* tournamentState, StateInfo* stateInfo, int gameWinner)
{
	// 1. Find the schedule slot that was just played
	int scheduleSlot = -1;
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 2; j++) {
			if (tournamentState->cupInfo.schedule[i][j] == tournamentState->cupInfo.userTeamIndexInTree) {
				scheduleSlot = i;
			}
		}
	}

	// If a match was found, process the result
	if (scheduleSlot != -1) {
		// 2. Determine which team in the schedule (slot 0 or 1) won the match
		// Get the actual team ID of the winner from globalGameInfo
		int winningTeamId = stateInfo->globalGameInfo->teams[gameWinner].value - 1;

		// Get the team ID for the first slot of the match that was played
		int teamIdInScheduleSlot0 = tournamentState->cupInfo.cupTeamIndexTree[tournamentState->cupInfo.schedule[scheduleSlot][0]];

		int winningSlotInSchedule;
		if (winningTeamId == teamIdInScheduleSlot0) {
			winningSlotInSchedule = 0;
		} else {
			winningSlotInSchedule = 1;
		}

		// 3. Update cup tree and schedule with the game result
		updateCupTreeAfterDay(tournamentState, stateInfo, scheduleSlot, winningSlotInSchedule);
		updateSchedule(tournamentState, stateInfo);
	}
}
