#include "front_menu.h"
#include "render.h"
#include "font.h"
#include "main_menu.h"
#include "globals.h"
#include "team_selection_menu.h"

#define ARROW_SCALE 0.05f
#define FIGURE_SCALE 0.4f
#define FRONT_ARROW_POS 0.15f

#define PLAY_TEXT_HEIGHT -0.1f
#define CUP_TEXT_HEIGHT 0.0f
#define HELP_TEXT_HEIGHT 0.1f
#define QUIT_TEXT_HEIGHT 0.2f

static void drawFrontTexts();

void initFrontMenuState(FrontMenuState *state)
{
	state->pointer = 0;
	state->rem = 4;
}

MenuStage updateFrontMenu(FrontMenuState *state, TeamSelectionState *teamSelectionState, KeyStates *keyStates, StateInfo* stateInfo)
{
	if(keyStates->released[0][KEY_DOWN]) {
		state->pointer +=1;
		state->pointer = (state->pointer+state->rem)%state->rem;
	}
	if(keyStates->released[0][KEY_UP]) {
		state->pointer -=1;
		state->pointer = (state->pointer+state->rem)%state->rem;
	}
	if(keyStates->released[0][KEY_2]) {
		if(state->pointer == 0) {
			initTeamSelectionState(teamSelectionState);
			teamSelectionState->rem = stateInfo->numTeams;
			return MENU_STAGE_TEAM_SELECTION;
		} else if(state->pointer == 1) {
			return MENU_STAGE_CUP;
		} else if(state->pointer == 2) {
			return MENU_STAGE_HELP;
		} else if(state->pointer == 3) stateInfo->screen = -1;
	}
	return MENU_STAGE_FRONT;
}

void drawFrontMenu(const FrontMenuState *state, const struct MenuData *menuData)
{
	drawFontBackground();
	// arrow
	glBindTexture(GL_TEXTURE_2D, menuData->arrowTexture);
	glPushMatrix();
	if(state->pointer == 0) glTranslatef(FRONT_ARROW_POS, 1.0f, PLAY_TEXT_HEIGHT);
	else if(state->pointer == 1) glTranslatef(FRONT_ARROW_POS, 1.0f, CUP_TEXT_HEIGHT);
	else if(state->pointer == 2) glTranslatef(FRONT_ARROW_POS, 1.0f, HELP_TEXT_HEIGHT);
	else glTranslatef(FRONT_ARROW_POS, 1.0f, QUIT_TEXT_HEIGHT);
	glScalef(ARROW_SCALE, ARROW_SCALE, ARROW_SCALE);
	glCallList(menuData->planeDisplayList);
	glPopMatrix();
	// catcher
	glBindTexture(GL_TEXTURE_2D, menuData->catcherTexture);
	glPushMatrix();
	glTranslatef(0.7f, 1.0f, 0.0f);
	glScalef(FIGURE_SCALE, FIGURE_SCALE, FIGURE_SCALE);
	glCallList(menuData->planeDisplayList);
	glPopMatrix();
	// batter
	glBindTexture(GL_TEXTURE_2D, menuData->batterTexture);
	glPushMatrix();
	glTranslatef(-0.6f, 1.0f, 0.0f);
	glScalef(FIGURE_SCALE/2, FIGURE_SCALE, FIGURE_SCALE);
	glCallList(menuData->planeDisplayList);
	glPopMatrix();
	drawFrontTexts();
}

static void drawFrontTexts()
{
	printText("P N B", 5, -0.23f, -0.4f, 8);
	printText("Play", 4, -0.1f, PLAY_TEXT_HEIGHT, 3);
	printText("Cup", 3, -0.085f, CUP_TEXT_HEIGHT, 3);
	printText("Help", 4, -0.1f, HELP_TEXT_HEIGHT, 3);
	printText("Quit", 4, -0.1f, QUIT_TEXT_HEIGHT, 3);
}
