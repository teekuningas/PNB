#include "front_menu.h"
#include "render.h"
#include "font.h"
#include "globals.h"
#include "menu_types.h"
#include "resource_manager.h"

// --- UI layout constants (now in 2D screen space) ---
#define FONT_SIZE_LARGE 120
#define FONT_SIZE_MEDIUM 60

#define TITLE_Y 100
#define MENU_START_Y 300
#define MENU_SPACING 100

static void drawFrontTexts(const RenderState* rs);

void initFrontMenuState(FrontMenuState *state)
{
	state->pointer = 0;
	state->rem = 4;
}

MenuStage updateFrontMenu(FrontMenuState *state, KeyStates *keyStates, StateInfo* stateInfo)
{
	if(keyStates->released[0][KEY_DOWN]) {
		state->pointer +=1;
		state->pointer = (state->pointer+state->rem)%state->rem;
	}
	if(keyStates->released[0][KEY_UP]) {
		state->pointer -=1;
		state->pointer = (state->pointer+state->rem)%state->rem;
	}
	if (keyStates->released[0][KEY_2]) {
		if (state->pointer == 0) {
			return MENU_STAGE_TEAM_SELECTION;
		} else if (state->pointer == 1) {
			return MENU_STAGE_CUP;
		} else if (state->pointer == 2) {
			return MENU_STAGE_HELP;
		} else if (state->pointer == 3) {
			return MENU_STAGE_QUIT;
		}
	}
	return MENU_STAGE_FRONT;
}

// New orthographic-only front menu rendering
void drawFrontMenu(const FrontMenuState *state, const RenderState* rs, ResourceManager* rm, const struct MenuData *menuData)
{
	// Setup orthographic 2D projection
	begin_2d_render(rs);
	// Disable depth and lighting for 2D rendering
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);

	// 1. Draw full-screen background quad
	glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, "data/textures/empty_background.tga"));
	glBegin(GL_QUADS);
	// All quads are defined Clockwise (TL, BL, BR, TR) to be compatible with the global GL_CULL_FACE setting
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

	// 2. Draw Batter and Catcher
	// Re-calculate size and position to better match the original perspective layout
	float imgHeight = rs->window_height * 0.8f;
	float yPos = rs->window_height - imgHeight - (rs->window_height * 0.10f); // 10% from bottom

	// Batter on left
	float batterImgWidth = imgHeight * 0.5f;
	float batterX = rs->window_width * 0.1f;
	glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, "data/textures/batter.tga"));
	glBegin(GL_QUADS);
	// TL
	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(batterX, yPos);
	// BL
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(batterX, yPos + imgHeight);
	// BR
	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(batterX + batterImgWidth, yPos + imgHeight);
	// TR
	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(batterX + batterImgWidth, yPos);
	glEnd();

	// Catcher on right
	float catcherImgWidth = imgHeight * 0.8f;
	float catcherX = rs->window_width * 1.05f - catcherImgWidth;
	glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, "data/textures/catcher.tga"));
	glBegin(GL_QUADS);
	// TL
	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(catcherX, yPos);
	// BL
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(catcherX, yPos + imgHeight);
	// BR
	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(catcherX + catcherImgWidth, yPos + imgHeight);
	// TR
	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(catcherX + catcherImgWidth, yPos);
	glEnd();

	// 3. Draw Arrow
	float center_x = rs->window_width / 2.0f;
	// Move arrow to the right of the text
	float arrow_x = center_x + 120;
	float arrow_y = MENU_START_Y + (state->pointer * MENU_SPACING);
	// Make arrow larger
	float arrow_size = 80.0f;

	glBindTexture(GL_TEXTURE_2D, resource_manager_get_texture(rm, "data/textures/arrow.tga"));
	glBegin(GL_QUADS);
	// The original arrow was drawn with CW winding (BL, BR, TR, TL), so it was correct.
	// Re-defining it here in a more standard CW order (TL, BL, BR, TR) for consistency.
	// TL
	glTexCoord2f(0, 1);
	glVertex2f(arrow_x, arrow_y);
	// BL
	glTexCoord2f(0, 0);
	glVertex2f(arrow_x, arrow_y + arrow_size);
	// BR
	glTexCoord2f(1, 0);
	glVertex2f(arrow_x + arrow_size, arrow_y + arrow_size);
	// TR
	glTexCoord2f(1, 1);
	glVertex2f(arrow_x + arrow_size, arrow_y);
	glEnd();

	// 4. Draw Text
	drawFrontTexts(rs);
}


static void drawFrontTexts(const RenderState* rs)
{
	float center_x = rs->window_width / 2.0f;
	// Adjust X offset based on new larger font size
	printText2D("P N B", 5, center_x - 200, TITLE_Y, FONT_SIZE_LARGE);
	printText2D("Play", 4, center_x - 80, MENU_START_Y, FONT_SIZE_MEDIUM);
	printText2D("Cup", 3, center_x - 60, MENU_START_Y + MENU_SPACING, FONT_SIZE_MEDIUM);
	printText2D("Help", 4, center_x - 80, MENU_START_Y + 2 * MENU_SPACING, FONT_SIZE_MEDIUM);
	printText2D("Quit", 4, center_x - 80, MENU_START_Y + 3 * MENU_SPACING, FONT_SIZE_MEDIUM);
}
