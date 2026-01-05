#ifndef RULES_STRIKES_H
#define RULES_STRIKES_H

#include "globals.h"

/**
 * Determines if the batter should change due to strikes.
 *
 * @param gameState Current game state containing strikes.
 * @return 1 if the batter should change, 0 otherwise.
 */
int should_change_batter_on_strikes(const GameState* gameState);

/**
 * Pure function to determine the outcome of a pitch.
 */
PitchResult determine_pitch_result(float ball_x, float plate_width, int bat_miss);

#endif // RULES_STRIKES_H
