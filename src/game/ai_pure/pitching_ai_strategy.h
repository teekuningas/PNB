#ifndef PITCHING_AI_STRATEGY_H
#define PITCHING_AI_STRATEGY_H

#include "globals.h"

void calculate_ai_pitch_targets(
    int rand1,
    int rand2,
    int rand3,
    const PlayerCounters* playerCounters,
    const GameState* gameState,
    int animation_frequency,
    unsigned int* out_first_limit,
    unsigned int* out_second_limit
);

#endif
