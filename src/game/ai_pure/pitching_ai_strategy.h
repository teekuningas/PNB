#ifndef PITCHING_AI_STRATEGY_H
#define PITCHING_AI_STRATEGY_H

#include "globals.h"

// A pitch as the AI decides it: both values at once, which it then declares as PITCH_POWER and
// PITCH_AIM on the same tick. A human reaches the same two values across the dance instead, one click
// each — the engine cannot tell the difference, because only the values ever reach it.
//   - power     : [0,1]  toss height (dy = PITCH_BASE_SPEED + power*PITCH_POWER_CONSTANT).
//   - direction : [-1,1] placement; 0 = straight up over the plate (oikea / strike).
//                 dx = direction*PITCH_ANGLE_CONSTANT. See pitch_velocity_from_aim.
typedef struct _PitchAim {
    float power;
    float direction;
} PitchAim;

// Semantic AI pitch strategy: decide the declared {power, direction} aim directly. See the .c for the
// contract.
PitchAim decide_pitch_aim(
    int batting_team_players_on_field_count, int strikes, int balls, int rand_power, int rand_dir, int rand_choice
);

#endif
