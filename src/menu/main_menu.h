#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include "globals.h"
#include "menu_types.h"
#include "resource_manager.h"
#include "render.h"

// Initialize menuData and prepare menu
int initMainMenu(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, ResourceManager* rm, RenderState* rs);
// Update and draw take explicit MenuData pointer for state
// Update and draw now explicitly take MenuData pointer
void updateMainMenu(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, KeyStates* keyStates);
void drawMainMenu(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, double alpha, ResourceManager* rm, RenderState* rs);
int cleanMainMenu(MenuData* menuData);

#endif /* MAIN_MENU_H */
