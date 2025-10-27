#include "globals.h"
#include "render.h"
#include "font.h"
#include "menu_helpers.h"
#include "menu_types.h"
#include "resource_manager.h"
#include "loading_screen_menu.h"

// Heights for loading texts (normalized coords)
#define LOADING_MODELS_Y      -0.15f
#define LOADING_APPRECIATED_Y  0.0f
#define LOADING_AUTHOR_Y       0.45f

// Reset main-menu state and render loading screen using new 2D pipeline
void drawLoadingScreen(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo,
                       ResourceManager* rm, const RenderState* rs)
{

	// Begin 2D orthographic rendering
	begin_2d_render(rs);
	// Draw shared menu background
	drawMenuLayout2D(rm, rs);

	float cw = rs->window_width;
	float ch = rs->window_height;
	printText2D("Loading resources", 17,
	            cw * 0.5f - 200.0f,
	            ch * 0.4f,
	            48.0f);
	printText2D("Your patience is appreciated", 28,
	            cw * 0.5f - 250.0f,
	            ch * 0.55f,
	            36.0f);
	printText2D("Erkka Heinila 2013", 18,
	            cw * 0.5f - 100.0f,
	            ch * 0.7f,
	            24.0f);

}
