#include "player_utils.h"

int get_active_batter_index(const MatchSession* game)
{
    if (!game) return -1;

    // Based on mutable_world.c and common_logic.c:
    // Indices 0 to 11 (PLAYERS_IN_TEAM + JOKER_COUNT) are always the current batting team.
    // Indices 12 to 20 are the fielding team.

    int limit = PLAYERS_IN_TEAM + JOKER_COUNT;

    for (int i = 0; i < limit; i++) {
        // We look for the player specifically marked as AT_BAT
        if (game->playerInfo[i].bTPI.state == PLAYER_STATE_AT_BAT) {
            return i;
        }
    }

    return -1;
}
