#ifndef GAME_CONSOLIDATION_H
#define GAME_CONSOLIDATION_H

#include "globals.h"
#include "menu_types.h"

// Initializes flow control counters and state
void consolidation_init(GameFlowState* gameFlowState);

// Main update function: Reacts to Referee decisions, manages flow, and enforces physical state
// referee is const — consolidation READS legal state but never WRITES to it.
void consolidation_update(
    StateInfo* stateInfo, const RefereeState* referee, MenuInfo* menuInfo, unsigned int* rng_seed
);

#endif
