#include "rules_strikes.h"

// §26 Syötön tuomitseminen
/**
 * Determines if the batter should change due to strikes.
 *
 * @param gameState Current game state containing strikes.
 * @param safe_on_first_base_index Index of the player safe on first base (-1 if none).
 * @return 1 if the batter should change, 0 otherwise.
 */
int should_change_batter_on_strikes(const GameState* gameState, int safe_on_first_base_index)
{
	if(gameState->strikes == 3 && safe_on_first_base_index != -1) {
		return 1;
	}
	return 0;
}
