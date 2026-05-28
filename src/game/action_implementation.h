#ifndef ACTION_IMPLEMENTATION_H
#define ACTION_IMPLEMENTATION_H

#include "globals.h"

void action_implementation(StateInfo* stateInfo, unsigned int* rng_seed);
void init_action_implementation(StateInfo* stateInfo);
void flushKeys(StateInfo* stateInfo);
void genericSlingBall(BallInfo* ballInfo, float x, float y, float z);

#endif /* ACTION_IMPLEMENTATION_H */
