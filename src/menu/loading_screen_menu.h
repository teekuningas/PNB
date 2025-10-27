#ifndef LOADING_SCREEN_MENU_H
#define LOADING_SCREEN_MENU_H

#include "globals.h"
#include "menu_types.h"
#include "resource_manager.h"
#include "render.h"

// Draw the initial loading screen before main menu startup.
// Takes explicit menuData pointer for state, plus resource and render state
void drawLoadingScreen(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo,
                       ResourceManager* rm, const RenderState* rs);

#endif // LOADING_SCREEN_MENU_H
