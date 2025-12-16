#ifndef ACTION_IMPLEMENTATION_H
#define ACTION_IMPLEMENTATION_H

#include "globals.h"

void actionImplementation(StateInfo* stateInfo, unsigned int* rng_seed);
void initActionImplementation(StateInfo* stateInfo);
void flushKeys(StateInfo* stateInfo);
void genericSlingBall(StateInfo* stateInfo, float x, float y, float z);

#endif /* ACTION_IMPLEMENTATION_H */
