#ifndef BASE_CONTROL_H
#define BASE_CONTROL_H

#include "globals.h"

/**
 * @brief Determines which player currently "controls" (is safe at) a given base.
 *
 * This is the SINGLE SOURCE OF TRUTH for base ownership.
 * It derives ownership from the RefereeState (currentSafetyBase).
 *
 * @param game Pointer to LocalGameInfo containing player and referee state.
 * @param base The BaseID to check (BASE_HOME, BASE_FIRST, etc.)
 * @return The index of the player controlling the base, or -1 if none.
 */
int get_base_controller(const LocalGameInfo* game, BaseID base);

/**
 * @brief Determines which base the ball is currently at/near.
 *
 * @param stateInfo The full game state (needed for ball and field positions).
 * @return The BaseID (as int 0-3) or -1 if not at any base.
 */
int get_ball_at_base_index(const StateInfo* stateInfo);

#endif // BASE_CONTROL_H

