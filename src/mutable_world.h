#ifndef MUTABLE_WORLD_H
#define MUTABLE_WORLD_H

#include "globals.h"
#include "menu_types.h"

int initMutableWorld(StateInfo* stateInfo);
void updateMutableWorld(StateInfo* stateInfo, MenuInfo* menuInfo);
void drawMutableWorld(StateInfo* stateInfo, double alpha);
int cleanMutableWorld(StateInfo* stateInfo);

#endif /* MUTABLE_WORLD_H */
