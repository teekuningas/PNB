#include "base_control.h"

int get_base_controller(const LocalGameInfo* game, BaseID base)
{
	if (base < 0 || base >= BASE_COUNT) return -1;

	// Iterate through all players to find who claims safety at this base.
	// This replaces the cached baseControlIndex array.
	for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
		// We trust the Referee State as the single source of truth for safety.
		if (game->referee.battingPlayers[i].currentSafetyBase == base) {
			return i;
		}
	}
	return -1;
}
