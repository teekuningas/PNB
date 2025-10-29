#include "globals.h"
#include "render.h"
#include "font.h"
#include "menu_helpers.h"
#include "menu_types.h"
#include "resource_manager.h"
#include "loading_screen_menu.h"

// Reset main-menu state and render loading screen using new 2D pipeline
void drawLoadingScreen(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo,
                       ResourceManager* rm, const RenderState* rs)
{

	// Begin 2D orthographic rendering
	begin_2d_render(rs);
	// Draw shared menu background
	drawMenuLayout2D(rm, rs);

	const float center_x = VIRTUAL_WIDTH * 0.5f;

	draw_text_2d("Loading resources", center_x, VIRTUAL_HEIGHT * 0.4f, 52.0f, TEXT_ALIGN_CENTER, rs);
	draw_text_2d("Your patience is appreciated", center_x, VIRTUAL_HEIGHT * 0.55f, 40.0f, TEXT_ALIGN_CENTER, rs);
	draw_text_2d("Erkka Heinila 2013", center_x, VIRTUAL_HEIGHT * 0.7f, 28.0f, TEXT_ALIGN_CENTER, rs);
}
