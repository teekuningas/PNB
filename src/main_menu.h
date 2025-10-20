#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include "globals.h"
#include "menu_types.h"
#include "team_selection_menu.h"

int initMainMenu(StateInfo* stateInfo, MenuInfo* menuInfo);
void updateMainMenu(StateInfo* stateInfo, MenuInfo* menuInfo, KeyStates* keyStates, GlobalGameInfo* globalGameInfo);
void drawMainMenu(StateInfo* stateInfo, MenuInfo* menuInfo, double alpha);
int cleanMainMenu(StateInfo* stateInfo);
void drawLoadingScreen(StateInfo* stateInfo, MenuInfo* menuInfo);

#endif /* MAIN_MENU_H */
