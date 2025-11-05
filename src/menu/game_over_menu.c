/*
 * game_over_menu.c
 *
 * Draws the Game-Over menu screen texts.
 */
#include "game_over_menu.h"
#include "font.h"
#include "menu_helpers.h"
#include "globals.h"

void initGameOverMenu(void)
{
	// This menu is stateless, so nothing to do here
}

MenuStage updateGameOverMenu(const GameConclusion* conclusion, const KeyStates* keyStates, int team1_control, int team2_control)
{
	int flag = 0;
	if (team1_control != 2) {
		if (keyStates->released[team1_control][KEY_2]) flag = 1;
	}
	if (team2_control != 2) {
		if (keyStates->released[team2_control][KEY_2]) flag = 1;
	}

	if (flag == 1) {
		if (conclusion->isCupGame) {
			return MENU_STAGE_CUP;
		}
		return MENU_STAGE_FRONT;
	}

	return MENU_STAGE_GAME_OVER;
}

void drawGameOverMenu(const GameConclusion* conclusion, const TeamData* teamData, RenderState* rs, ResourceManager* rm)
{
	begin_2d_render(rs);
	drawMenuLayout2D(rm, rs);

	char buffer[128];
	const float center_x = VIRTUAL_WIDTH / 2.0f;

	sprintf(buffer, "Team %d is victorious", conclusion->winner + 1);
	draw_text_2d(buffer, center_x, 150, 60.0f, TEXT_ALIGN_CENTER, rs);

	const char* winner_name = teamData[conclusion->winner].name;
	sprintf(buffer, "Congratulations %s!", winner_name);
	draw_text_2d(buffer, center_x, 220, 50.0f, TEXT_ALIGN_CENTER, rs);

	sprintf(buffer, "First period: %d - %d", conclusion->period0Runs[0], conclusion->period0Runs[1]);
	draw_text_2d(buffer, center_x, 350, 40.0f, TEXT_ALIGN_CENTER, rs);

	sprintf(buffer, "Second period: %d - %d", conclusion->period1Runs[0], conclusion->period1Runs[1]);
	draw_text_2d(buffer, center_x, 400, 40.0f, TEXT_ALIGN_CENTER, rs);

	if (conclusion->period2Runs[0] > 0 || conclusion->period2Runs[1] > 0) {
		sprintf(buffer, "Super inning: %d - %d", conclusion->period2Runs[0], conclusion->period2Runs[1]);
		draw_text_2d(buffer, center_x, 450, 40.0f, TEXT_ALIGN_CENTER, rs);
	}

	if (conclusion->period3Runs[0] > 0 || conclusion->period3Runs[1] > 0) {
		sprintf(buffer, "Homerun contest: %d - %d", conclusion->period3Runs[0], conclusion->period3Runs[1]);
		draw_text_2d(buffer, center_x, 500, 40.0f, TEXT_ALIGN_CENTER, rs);
	}
}
