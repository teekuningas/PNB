#ifndef MUTABLE_WORLD_H
#define MUTABLE_WORLD_H

#include "globals.h"
#include "menu_types.h"
#include "resource_manager.h"

int initMutableWorld(StateInfo* stateInfo, ResourceManager* rm);
void updateMutableWorld(StateInfo* stateInfo, MenuInfo* menuInfo, unsigned int* rng_seed);
void reconcileLegalAndPhysicalState(StateInfo* stateInfo);
void drawMutableWorld(const StateInfo* stateInfo, double alpha, ResourceManager* rm);
int cleanMutableWorld(StateInfo* stateInfo);

#endif /* MUTABLE_WORLD_H */
