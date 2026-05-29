#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include "globals.h"
#include "menu_types.h"
#include "resource_manager.h"

int init_game_screen(StateInfo* stateInfo, ResourceManager* rm);
void update_game_screen(StateInfo* stateInfo, MenuInfo* menuInfo, unsigned int* rng_seed);
void draw_game_screen(const StateInfo* stateInfo, double alpha, ResourceManager* rm, const RenderState* rs);
int clean_game_screen(StateInfo* stateInfo);

#endif /* GAME_SCREEN_H */
