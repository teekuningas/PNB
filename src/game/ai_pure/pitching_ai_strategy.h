#ifndef PITCHING_AI_STRATEGY_H
#define PITCHING_AI_STRATEGY_H

#include "globals.h"

// Semantic AI pitch strategy: decide the declared {power, direction} aim directly. See the .c for the
// contract.
PitchAim decide_pitch_aim(
    int batting_team_players_on_field_count, int strikes, int balls, int rand_power, int rand_dir, int rand_choice
);

#endif
