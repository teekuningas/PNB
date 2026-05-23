#ifndef GAME_RESET_H
#define GAME_RESET_H

#include "globals.h"

void resetPhysicalWorld(StateInfo* stateInfo, unsigned int* rng_seed);
void resetForNewHalfInning(StateInfo* stateInfo, unsigned int* rng_seed);
void resetForFoulPlay(StateInfo* stateInfo, unsigned int* rng_seed);
void resetForNextPair(StateInfo* stateInfo, unsigned int* rng_seed);

#endif // GAME_RESET_H
