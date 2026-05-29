#ifndef BASE_CONTROL_H
#define BASE_CONTROL_H

#include "globals.h"

/**
 * @brief Determines which player currently "controls" (is safe at) a given base.
 *
 * This is the SINGLE SOURCE OF TRUTH for base ownership.
 * A player controls a base if they BOTH have safety at the base (currentSafetyBase)
 * AND are physically at the base (baseId). This distinction matters for vapaataival
 * (free walk) where safety is granted immediately but control comes upon arrival.
 *
 * @param game Pointer to MatchSession containing player physical state.
 * @param referee Pointer to RefereeState containing player legal state.
 * @param base The BaseID to check (BASE_HOME, BASE_FIRST, etc.)
 * @return The index of the player controlling the base, or -1 if none.
 */
int get_base_controller(const MatchSession* game, const RefereeState* referee, BaseID base);

/**
 * @brief Determines which base the ball is currently at/near.
 *
 * @param match The match session (for ball position and player index info).
 * @param field_positions The field geometry (for base positions).
 * @return The BaseID (as int 0-3) or -1 if not at any base.
 */
int get_ball_at_base_index(const MatchSession* match, const FieldPositions* field_positions);

#endif // BASE_CONTROL_H
