#ifndef THROWING_SYSTEM_H
#define THROWING_SYSTEM_H

#include "globals.h"

#define THROW_MAX 50

void init_throwing_system(StateInfo* stateInfo);

void prepare_throw(StateInfo* stateInfo, BaseID base);

void throw_release(StateInfo* stateInfo);
void throw_load(StateInfo* stateInfo, BaseID base);
void fielder_move(StateInfo* stateInfo, int direction);
void fielder_stop_move(StateInfo* stateInfo, int direction);
void drop_ball(StateInfo* stateInfo);
void update_controlled_player_speed(StateInfo* stateInfo);

#endif
