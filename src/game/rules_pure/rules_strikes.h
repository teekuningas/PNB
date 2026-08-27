#ifndef RULES_STRIKES_H
#define RULES_STRIKES_H

#include "globals.h"

/**
 * Pure function to determine the outcome of a pitch.
 */
PitchResult determine_pitch_result(float ball_x, float plate_width);

// §28: the batter has the right to three correct pitches in his batting turn.
#define CORRECT_PITCHES_PER_BATTING_TURN 3

/**
 * §18(1): has the batter permanently become a runner by spending his three correct pitches?
 */
int batter_has_become_runner_permanently(int correct_pitches_received);

#endif // RULES_STRIKES_H
