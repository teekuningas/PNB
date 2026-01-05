#ifndef IMMUTABLE_WORLD_H
#define IMMUTABLE_WORLD_H

#include "globals.h"
#include "resource_manager.h"

int initImmutableWorld(StateInfo* stateInfo, ResourceManager* rm);
void drawImmutableWorld(const StateInfo* stateInfo, double alpha, ResourceManager* rm);
int cleanImmutableWorld(StateInfo* stateInfo);

#endif /* IMMUTABLE_WORLD_H */
