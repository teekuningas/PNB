#ifndef PLAYER_UTILS_H
#define PLAYER_UTILS_H

#include "globals.h"

/**
 * @brief Finds the index of the player who is currently at bat.
 *
 * This function iterates through all players (both teams, though typically only batting team matters)
 * to find the single player whose state is PLAYER_STATE_AT_BAT.
 *
 * @param game The MatchSession structure containing player states.
 * @return The index of the player at bat, or -1 if no player is at bat.
 */
int get_active_batter_index(const MatchSession* game);

/**
 * @brief Returns the teams[] index of the team currently batting.
 *
 * Pesäpallo alternates batting/catching roles each half-inning. The formula
 * accounts for inning progression, period transitions, and which team plays first.
 * The catching team index is always (result + 1) % 2.
 *
 * @param sb The scoreboard containing inning, period, and playsFirst.
 * @return 0 or 1 — the index into scoreboard.teams[] for the batting team.
 */
int get_batting_team_index(const Scoreboard* sb);

#endif /* PLAYER_UTILS_H */
