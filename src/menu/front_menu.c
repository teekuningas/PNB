#include "front_menu.h"
#include "render.h"
#include "font.h"
#include "globals.h"
#include "menu_types.h"
#include "resource_manager.h"
#include "menu_helpers.h"

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
	// --- Layout Constants ---
	const float title_y = rs->window_height * 0.1f;
	const float menu_start_y = rs->window_height * 0.4f;
	const float menu_spacing = rs->window_height * 0.1f;
	const float title_fontsize = 120.0f;
	const float menu_fontsize = 60.0f;

	// Setup orthographic 2D projection and draw shared menu background
	begin_2d_render(rs);
	drawMenuLayout2D(rm, rs);

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

	// 3. Draw Text and Arrow
	const float center_x = rs->window_width / 2.0f;
	draw_text_2d("P N B", center_x, title_y, title_fontsize, TEXT_ALIGN_CENTER, rs);
	draw_text_2d("Play", center_x, menu_start_y, menu_fontsize, TEXT_ALIGN_CENTER, rs);
	draw_text_2d("Cup", center_x, menu_start_y + menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);
	draw_text_2d("Help", center_x, menu_start_y + 2 * menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);
	draw_text_2d("Quit", center_x, menu_start_y + 3 * menu_spacing, menu_fontsize, TEXT_ALIGN_CENTER, rs);

	// Draw Arrow
	float arrow_size = 80.0f;
	float arrow_x = center_x + 120.0f;
	float arrow_y = menu_start_y + (state->pointer * menu_spacing) - (arrow_size - menu_fontsize) / 2.0f;

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
}
