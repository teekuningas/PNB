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

/**
 * @brief Whether a team's decisions are made by the AI rather than a human pad.
 *
 * Single source of truth for the "is this team AI-controlled?" question. Used by
 * both the in-game decision path (whether to run ai_update / skip input) and the
 * menu-decision path (whether menus auto-advance instead of reading a pad). This
 * is the seam that keeps "who decides" decoupled from the pipeline: today a team
 * is either a human pad or the AI; future sources (network, replay) slot in here.
 *
 * @param control The team's TeamControlMode.
 * @return 1 if AI-controlled, 0 otherwise.
 */
int team_is_ai(TeamControlMode control);

/** The most candidates §27 can ever offer: the lyöntivuoroinen regular plus the three jokers. */
#define BATTER_CANDIDATE_MAX (1 + JOKER_COUNT)

/**
 * @brief The players who may take the bat right now, in the order §27 offers them.
 *
 * The list every producer chooses from, and the same rule the INGEST gate refuses with — one
 * function so a client can be helpful (never showing a human an option the rules forbid) without a
 * second copy of the rule to drift from the first. A client that gets it wrong anyway is still
 * refused at the gate; this exists so it does not have to be.
 *
 * Order is the rulebook's own: the *lyöntivuoroinen* regular first, then the unused jokers, which is
 * also the order the engine's old offer cycled through — so a controller that takes the first
 * acceptable candidate makes the same choice it always did.
 *
 * @param out Filled with player indices; must hold at least BATTER_CANDIDATE_MAX.
 * @return How many candidates were written. Zero means nobody may bat, which is §12's own signal
 *         that the half-inning is over as soon as the ball is in a home fielder's hands.
 */
int list_batter_candidates(const MatchSession* game, const Scoreboard* sb, const HalfInningState* his, int* out);

#endif /* PLAYER_UTILS_H */
