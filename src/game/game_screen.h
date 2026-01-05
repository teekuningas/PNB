#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include "globals.h"
#include "menu_types.h"
#include "resource_manager.h"

int initGameScreen(StateInfo* stateInfo, ResourceManager* rm);
void updateGameScreen(StateInfo* stateInfo, MenuInfo* menuInfo, unsigned int* rng_seed);
void drawGameScreen(const StateInfo* stateInfo, double alpha, ResourceManager* rm, const RenderState* rs);
int cleanGameScreen(StateInfo* stateInfo);

#endif /* GAME_SCREEN_H */
