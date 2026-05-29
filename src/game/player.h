#ifndef PLAYER_H
#define PLAYER_H

#include "globals.h"
#include "resource_manager.h"

int init_player(StateInfo* stateInfo, ResourceManager* rm);
int clean_player(StateInfo* stateInfo);

#endif /* PLAYER_H */
