#ifndef BATTING_AI_H
#define BATTING_AI_H

#include "globals.h"

void init_batting_ai(AIState* aiState);
void update_batting_ai(StateInfo* stateInfo, unsigned int* rng_seed);

#endif
