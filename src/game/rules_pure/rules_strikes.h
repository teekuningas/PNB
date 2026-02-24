#ifndef RULES_STRIKES_H
#define RULES_STRIKES_H

#include "globals.h"

/**
 * Pure function to determine the outcome of a pitch.
 */
PitchResult determine_pitch_result(float ball_x, float plate_width, int bat_miss);

#endif // RULES_STRIKES_H
