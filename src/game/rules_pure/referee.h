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

#endif // REFEREE_H
