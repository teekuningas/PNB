#ifndef CATCHING_AI_H
#define CATCHING_AI_H

#include "globals.h"

#define TIMEOUT_CONSTANT 200

void initCatchingAI(AIState* aiState);
void updateCatchingAI(StateInfo* stateInfo, unsigned int* rng_seed);
void moveControlledPlayerToLocation(StateInfo* stateInfo, Vector3D* target);
void throwBallToBase(StateInfo* stateInfo, int base);

#endif
