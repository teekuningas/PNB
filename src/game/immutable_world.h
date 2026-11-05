#ifndef IMMUTABLE_WORLD_H
#define IMMUTABLE_WORLD_H

#include "session.h"
#include "resource_manager.h"

int init_immutable_world(StateInfo* stateInfo, ResourceManager* rm);
void draw_immutable_world(ResourceManager* rm);
int clean_immutable_world(StateInfo* stateInfo);

#endif /* IMMUTABLE_WORLD_H */
