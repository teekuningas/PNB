#ifndef BATTING_AI_H
#define BATTING_AI_H

#include "globals.h"

void initBattingAI(AIState* aiState);
void updateBattingAI(StateInfo* stateInfo, unsigned int* rng_seed);

#endif
