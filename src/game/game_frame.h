#ifndef GAME_FRAME_H
#define GAME_FRAME_H

#include "globals.h"
#include "menu_types.h"
#include "resource_manager.h"

int init_game_frame(StateInfo* stateInfo, ResourceManager* rm);
void update_game_frame(StateInfo* stateInfo, MenuInfo* menuInfo, unsigned int* rng_seed);
void draw_game_frame(const StateInfo* stateInfo, double alpha, ResourceManager* rm);
int clean_game_frame(StateInfo* stateInfo);

#endif /* GAME_FRAME_H */
