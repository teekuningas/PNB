#ifndef BATTING_SYSTEM_H
#define BATTING_SYSTEM_H

#include "globals.h"

#define BAT_LOAD_MAX (4 * 9)
#define BAT_SWING_MAX (4 * 13)

void init_batting_system(StateInfo* stateInfo);

void select_batter(StateInfo* stateInfo);
void start_increase_batter_angle(StateInfo* stateInfo);
void stop_increase_batter_angle(StateInfo* stateInfo);
void start_decrease_batter_angle(StateInfo* stateInfo);
void stop_decrease_batter_angle(StateInfo* stateInfo);
void select_power(StateInfo* stateInfo);
void select_angle(StateInfo* stateInfo);
void update_batting(StateInfo* stateInfo);
void update_batting_meter(StateInfo* stateInfo);

#endif
