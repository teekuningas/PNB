#ifndef PLAYER_H
#define PLAYER_H

#include "globals.h"

int initPlayer(StateInfo* stateInfo);
void drawPlayer(StateInfo* stateInfo, PlayerInfo *playerInfo, double alpha);
int cleanPlayer(StateInfo* stateInfo);

#endif /* PLAYER_H */
