#include "globals.h"
#include "render.h"
#include "font.h"
#include "menu_helpers.h"
#include "menu_types.h"
#include "loading_screen_menu.h"

// Heights for loading texts
#define LOADING_MODELS_HEIGHT      -0.15f
#define LOADING_APPRECIATED_HEIGHT  0.0f
#define LOADING_AUTHOR_HEIGHT       0.45f

// Print the static loading messages
static void drawLoadingTexts()
{
	printText("Loading resources", 17, -0.21f, LOADING_MODELS_HEIGHT, 2);
	printText("Your patience is appreciated", 28, -0.48f, LOADING_APPRECIATED_HEIGHT, 3);
	printText("Erkka Heinila 2013", 18, -0.22f, LOADING_AUTHOR_HEIGHT, 2);
}

// Reset main-menu state and render loading screen
void drawLoadingScreen(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo)
{
	// Reset shared menu state as in original startup
	resetMenuForNewGame(menuData, stateInfo);
	// Set up camera identical to main menu
	gluLookAt(menuData->cam.x, menuData->cam.y, menuData->cam.z,
	          menuData->look.x, menuData->look.y, menuData->look.z,
	          menuData->up.x,   menuData->up.y,   menuData->up.z);
	drawFontBackground();
	drawLoadingTexts();
}
