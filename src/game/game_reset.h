#ifndef GAME_RESET_H
#define GAME_RESET_H

#include "globals.h"

void reset_physical_world(StateInfo* stateInfo, unsigned int* rng_seed);
void reset_for_new_half_inning(StateInfo* stateInfo, unsigned int* rng_seed);
void reset_for_foul_play(StateInfo* stateInfo, const RefereeState* referee, unsigned int* rng_seed);
void reset_for_next_pair(StateInfo* stateInfo, unsigned int* rng_seed);

#endif // GAME_RESET_H
