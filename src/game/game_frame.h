#ifndef GAME_FRAME_H
#define GAME_FRAME_H

#include "globals.h"
#include "menu_types.h"
#include "resource_manager.h"

int init_game_frame(StateInfo* stateInfo, ResourceManager* rm);
// One tick of the game, unless it is paused. Returns 1 if it ticked — the screen's clocks advance with the
// game and not with the screen, and only the tick knows whether it ran.
int update_game_frame(StateInfo* stateInfo, MenuInfo* menuInfo);
void draw_game_frame(const StateInfo* stateInfo, double alpha, ResourceManager* rm);
int clean_game_frame(StateInfo* stateInfo);

#endif /* GAME_FRAME_H */
