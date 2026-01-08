#ifndef REFEREE_H
#define REFEREE_H

#include "globals.h"

/**
 * @brief Pure analysis of the game state to determine rule outcomes.
 *
 * This function examines the current state (ball position, player positions,
 * game phase) and determines if any rule-based events should occur
 * (Outs, Runs, Wounds, Forced Advances).
 *
 * It does NOT modify the state. It returns a RefereeDecisions struct
 * containing all pending changes.
 *
 * @param stateInfo Pointer to the full game state (treated as Read-Only)
 * @return RefereeDecisions Struct containing all decisions for this frame
 */
RefereeDecisions Referee_Analyze(const StateInfo* stateInfo);

/**
 * @brief Initializes the RefereeState structure.
 * Sets all bases to BASE_NONE (-1) instead of 0.
 */
void initializeRefereeState(RefereeState* referee);

/**
 * @brief Query functions for wounding system (Milestone 15 consolidation)
 *
 * These functions provide read-only access to the wounding state,
 * following the same pattern as get_base_controller().
 */

/**
 * @brief Check if a wounding catch is currently pending
 */
int is_wounding_catch_pending(const RefereeState* ref);

/**
 * @brief Check if the wounding catch has been handled (started processing)
 */
int is_wounding_catch_handled(const RefereeState* ref);

/**
 * @brief Get the current wounding catch timer value (-1 if inactive)
 */
int get_wounding_timer(const RefereeState* ref);

/**
 * @brief Check if a specific player is marked for wounding
 * @param ref The referee state
 * @param playerIndex The player index to check
 * @return 1 if marked for wounding, 0 otherwise
 */
int is_player_marked_for_wound(const RefereeState* ref, int playerIndex);

#endif // REFEREE_H
