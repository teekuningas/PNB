#ifndef PITCHING_AI_STRATEGY_H
#define PITCHING_AI_STRATEGY_H

#include "globals.h"

void calculate_ai_pitch_targets(
    int rand1, int rand2, int rand3, int batting_team_players_on_field_count, const HalfInningState* halfInningState,
    int animation_frequency, unsigned int* out_first_limit, unsigned int* out_second_limit
);

#endif
