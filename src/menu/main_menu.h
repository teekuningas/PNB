#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include "globals.h"
#include "menu_types.h"

// Initialize menuData and prepare menu
int initMainMenu(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo);
// Update and draw take explicit MenuData pointer for state
// Update and draw now explicitly take MenuData pointer
void updateMainMenu(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, KeyStates* keyStates);
void drawMainMenu(StateInfo* stateInfo, MenuData* menuData, MenuInfo* menuInfo, double alpha);
int cleanMainMenu(MenuData* menuData);

#endif /* MAIN_MENU_H */
