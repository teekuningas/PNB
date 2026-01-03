#ifndef RULES_STRIKES_H
#define RULES_STRIKES_H

#include "globals.h"

/**
 * Determines if the batter should change due to strikes.
 *
 * References:
 * §26 Syötön tuomitseminen
 *
 * @param gameState Current game state containing strikes.
 * @param safe_on_first_base_index Index of the player safe on first base (-1 if none).
 * @return 1 if the batter should change, 0 otherwise.
 */
int should_change_batter_on_strikes(const GameState* gameState, int safe_on_first_base_index);

#endif
