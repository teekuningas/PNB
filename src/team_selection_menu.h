#ifndef TEAM_SELECTION_MENU_H
#define TEAM_SELECTION_MENU_H

#include "menu_types.h"

void updateTeamSelectionMenu(MenuData* menuData, StateInfo* stateInfo, MenuInfo* menuInfo, KeyStates* keyStates, GlobalGameInfo* globalGameInfo);
void drawTeamSelectionMenu(MenuData* menuData, StateInfo* stateInfo, MenuInfo* menuInfo, double alpha);

#endif // TEAM_SELECTION_MENU_H
