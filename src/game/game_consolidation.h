#ifndef GAME_CONSOLIDATION_H
#define GAME_CONSOLIDATION_H

#include "globals.h"
#include "menu_types.h"

// Initializes flow control counters and state
void GameConsolidation_Init(GameFlowState* gameFlowState);

// Main update function: Reacts to Referee decisions, manages flow, and enforces physical state
void GameConsolidation_Update(StateInfo* stateInfo, MenuInfo* menuInfo, unsigned int* rng_seed);

#endif
