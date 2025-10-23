#ifndef LOADING_SCREEN_MENU_H
#define LOADING_SCREEN_MENU_H

#include "globals.h"
#include "menu_types.h"

// Draw the initial loading screen before main menu startup.
// Takes explicit menuData pointer for state
void drawLoadingScreen(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo);

#endif // LOADING_SCREEN_MENU_H
