#ifndef CATCHING_AI_H
#define CATCHING_AI_H

#include "globals.h"

#define TIMEOUT_CONSTANT 200

void init_catching_ai(AIState* aiState);
void update_catching_ai(StateInfo* stateInfo, unsigned int* rng_seed);
void move_controlled_player_to_location(StateInfo* stateInfo, Vector3D* target);
void throw_ball_to_base(StateInfo* stateInfo, BaseID base);

#endif
