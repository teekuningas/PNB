#include "cup_helpers.h"

void updateSchedule(MenuData* md, StateInfo* stateInfo)
{
	int j;
	int counter = 0;
	for(j = 0; j < SLOT_COUNT/2; j++) {
		if(md->cupInfo.gameStructure == 0) {
			if(md->cupInfo.slotWins[j*2] < 3 && md->cupInfo.slotWins[j*2+1] < 3) {
				if(j < 4 || (j < 6 && md->cupInfo.dayCount >= 5) ||
				        (j == 6 && md->cupInfo.dayCount >= 10)) {
					md->cupInfo.schedule[counter][0] = j*2;
					md->cupInfo.schedule[counter][1] = j*2+1;
					counter++;
				}
			}
		} else {
			if(md->cupInfo.slotWins[j*2] < 1 && md->cupInfo.slotWins[j*2+1] < 1) {
				if(j < 4 || (j < 6 && md->cupInfo.dayCount >= 1) ||
				        (j == 6 && md->cupInfo.dayCount >= 2)) {
					md->cupInfo.schedule[counter][0] = j*2;
					md->cupInfo.schedule[counter][1] = j*2+1;
					counter++;
				}
			}
		}
	}
	for(j = counter; j < 4; j++) {
		md->cupInfo.schedule[j][0] = -1;
		md->cupInfo.schedule[j][1] = -1;
	}
}

void updateCupTreeAfterDay(MenuData* md, StateInfo* stateInfo, int scheduleSlot, int winningSlot)
{
	int i;
	int counter = 0;
	int done = 0;
	while(done == 0 && counter < 4) {
		if(md->cupInfo.schedule[counter][0] != -1) {
			int team1Index = md->cupInfo.cupTeamIndexTree[md->cupInfo.schedule[counter][0]];
			int team2Index = md->cupInfo.cupTeamIndexTree[md->cupInfo.schedule[counter][1]];
			int index = 8 + md->cupInfo.schedule[counter][0] / 2;
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
			md->cupInfo.slotWins[md->cupInfo.schedule[counter][winningTeam]] += 1;
			if(md->cupInfo.gameStructure == 0) {
				if(md->cupInfo.slotWins[md->cupInfo.schedule[counter][winningTeam]] == 3) {
					if(index < 14) {
						md->cupInfo.cupTeamIndexTree[index] =
						    md->cupInfo.cupTeamIndexTree[md->cupInfo.schedule[counter][winningTeam]];
						if(md->cupInfo.schedule[counter][winningTeam] == md->cupInfo.userTeamIndexInTree) {
							md->cupInfo.userTeamIndexInTree = index;
						}
					} else {
						md->cupInfo.winnerIndex = md->cupInfo.cupTeamIndexTree[md->cupInfo.schedule[counter][winningTeam]];
					}
				}
			} else {
				if(md->cupInfo.slotWins[md->cupInfo.schedule[counter][winningTeam]] == 1) {
					if(index < 14) {
						md->cupInfo.cupTeamIndexTree[index] =
						    md->cupInfo.cupTeamIndexTree[md->cupInfo.schedule[counter][winningTeam]];
						if(md->cupInfo.schedule[counter][winningTeam] == md->cupInfo.userTeamIndexInTree) {
							md->cupInfo.userTeamIndexInTree = index;
						}
					} else {
						md->cupInfo.winnerIndex = md->cupInfo.cupTeamIndexTree[md->cupInfo.schedule[counter][winningTeam]];
					}
				}
			}
			counter++;
		} else {
			done = 1;
		}
	}
}
