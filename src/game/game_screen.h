#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include "globals.h"
#include "menu_types.h"

int initGameScreen(StateInfo* stateInfo);
void updateGameScreen(StateInfo* stateInfo, MenuInfo* menuInfo);
void drawGameScreen(StateInfo* stateInfo, double alpha, const RenderState* rs);
int cleanGameScreen(StateInfo* stateInfo);

#endif /* GAME_SCREEN_H */
