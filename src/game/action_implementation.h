#ifndef ACTION_IMPLEMENTATION_H
#define ACTION_IMPLEMENTATION_H

#include "globals.h"

void action_implementation(StateInfo* stateInfo, unsigned int* rng_seed);
void init_action_implementation(StateInfo* stateInfo);
void generic_sling_ball(BallInfo* ballInfo, float x, float y, float z);

#endif /* ACTION_IMPLEMENTATION_H */
