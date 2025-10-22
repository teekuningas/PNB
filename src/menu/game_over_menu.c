/*
 * game_over_menu.c
 *
 * Draws the Game-Over menu screen texts.
 */
#include "game_over_menu.h"
#include "render.h"
#include "font.h"
#include "menu_helpers.h"

// Helper to split runs into two characters each
static void calculateRuns(char *p1, char *p2, char *p3, char *p4, int runs1, int runs2)
{
	if (runs1 >= 10) {
		*p1 = (char)('0' + runs1 / 10);
		*p2 = (char)('0' + runs1 % 10);
	} else {
		*p1 = ' ';
		*p2 = (char)('0' + runs1);
	}
	if (runs2 >= 10) {
		*p3 = (char)('0' + runs2 / 10);
		*p4 = (char)('0' + runs2 % 10);
	} else {
		*p3 = ' ';
		*p4 = (char)('0' + runs2);
	}
}

void drawGameOverMenu(StateInfo* stateInfo)
{
	drawFontBackground();
	char str[21] = "Team x is victorious";
	char str2[19] = "First period xx-xx";
	char str3[20] = "Second period xx-xx";
	char str4[19] = "Super inning xx-xx";
	char str5[22] = "Homerun contest xx-xx";
	char str6[16] = "Congratulations";
	int teamIndex = stateInfo->globalGameInfo->teams[stateInfo->globalGameInfo->winner].value;
	char* str7 = stateInfo->teamData[teamIndex - 1].name;
	float left = -0.30f - strlen(str7)/100.0f;
	int runs1;
	int runs2;
	if(stateInfo->globalGameInfo->winner == 0) str[5] = '1';
	else str[5] = '2';
	printText(str, 20, -0.33f, -0.3f, 3);
	printText(str6, strlen(str6), left, -0.18f, 3);
	printText(str7, strlen(str7), left + 0.58f, -0.18f, 3);
	runs1 = stateInfo->globalGameInfo->teams[0].period0Runs;
	runs2 = stateInfo->globalGameInfo->teams[1].period0Runs;
	calculateRuns(&str2[13], &str2[14], &str2[16], &str2[17], runs1, runs2);
	printText(str2, 18, -0.22f, 0.0f, 2);
	runs1 = stateInfo->globalGameInfo->teams[0].period1Runs;
	runs2 = stateInfo->globalGameInfo->teams[1].period1Runs;
	calculateRuns(&str3[14], &str3[15], &str3[17], &str3[18], runs1, runs2);
	printText(str3, 19, -0.22f, 0.1f, 2);
	if(stateInfo->globalGameInfo->period >= 2) {
		runs1 = stateInfo->globalGameInfo->teams[0].period2Runs;
		runs2 = stateInfo->globalGameInfo->teams[1].period2Runs;
		calculateRuns(&str4[13], &str4[14], &str4[16], &str4[17], runs1, runs2);
		printText(str4, 18, -0.22f, 0.2f, 2);
	}
	if(stateInfo->globalGameInfo->period >= 4) {
		runs1 = stateInfo->globalGameInfo->teams[0].period3Runs;
		runs2 = stateInfo->globalGameInfo->teams[1].period3Runs;
		calculateRuns(&str5[16], &str5[17], &str5[19], &str5[20], runs1, runs2);
		printText(str5, 21, -0.22f, 0.3f, 2);
	}
}

MenuStage updateGameOverMenu(MenuData* md, StateInfo* stateInfo, KeyStates* keyStates, MenuInfo* menuInfo)
{
	int flag = 0;
	if (md->team1_control != 2) {
		if (keyStates->released[md->team1_control][KEY_2]) flag = 1;
	}
	if (md->team2_control != 2) {
		if (keyStates->released[md->team2_control][KEY_2]) flag = 1;
	}
	if (flag == 1) {
		resetMenuForNewGame(md);
		stateInfo->playSoundEffect = SOUND_MENU;

		if (md->cupGame == 1) {
			int i, j;
			int scheduleSlot = -1;
			int playerWon = 0;
			for (i = 0; i < 4; i++) {
				for (j = 0; j < 2; j++) {
					if (md->cupInfo.schedule[i][j] == md->cupInfo.userTeamIndexInTree) {
						scheduleSlot = i;
						if (j == stateInfo->globalGameInfo->winner) playerWon = 1;
					}
				}
			}
			if (playerWon == 1) {
				int advance = 0;
				if (md->cupInfo.gameStructure == 0) {
					if (md->cupInfo.slotWins[md->cupInfo.userTeamIndexInTree] == 2) {
						if (md->cupInfo.dayCount >= 11) advance = 1;
					}
				} else {
					if (md->cupInfo.slotWins[md->cupInfo.userTeamIndexInTree] == 0) {
						if (md->cupInfo.dayCount >= 3) advance = 1;
					}
				}
				if (advance == 1) md->stage_8_state = 7;
			}
			updateCupTreeAfterDay(md, stateInfo, scheduleSlot, stateInfo->globalGameInfo->winner);
			updateSchedule(md, stateInfo);
		}
	}
	return md->stage;
}