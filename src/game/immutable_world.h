#ifndef IMMUTABLE_WORLD_H
#define IMMUTABLE_WORLD_H

#include "globals.h"
#include "resource_manager.h"

int init_immutable_world(StateInfo* stateInfo, ResourceManager* rm);
void draw_immutable_world(const StateInfo* stateInfo, double alpha, ResourceManager* rm);
int clean_immutable_world(StateInfo* stateInfo);

#endif /* IMMUTABLE_WORLD_H */
