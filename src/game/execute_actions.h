#ifndef EXECUTE_ACTIONS_H
#define EXECUTE_ACTIONS_H

#include "globals.h"

void execute_actions(StateInfo* stateInfo);
void init_execute_actions(StateInfo* stateInfo);
void generic_sling_ball(BallInfo* ballInfo, float x, float y, float z);
void update_meters(StateInfo* stateInfo);
void ai_update(StateInfo* stateInfo, unsigned int* rng_seed);

#endif /* EXECUTE_ACTIONS_H */
