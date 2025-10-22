/*
 * game_over_menu.h
 *
 * Handles drawing the Game-Over menu screen.
 */
#ifndef GAME_OVER_MENU_H
#define GAME_OVER_MENU_H

#include "menu_types.h"
#include "globals.h"

// Draw function for the Game-Over stage
// 'state' is currently unused but kept for symmetry
void drawGameOverMenu(StateInfo* stateInfo);
MenuStage updateGameOverMenu(MenuData* md, StateInfo* stateInfo, KeyStates* keyStates, MenuInfo* menuInfo);

#endif // GAME_OVER_MENU_H
