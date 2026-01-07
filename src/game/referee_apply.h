#ifndef REFEREE_APPLY_H
#define REFEREE_APPLY_H

#include "globals.h"
#include "rules_pure/referee.h"

/**
 * @brief Applies the decisions made by the Referee to the game state.
 *
 * This function is the "Write" phase of the decoupled rules engine.
 * It takes the read-only decisions and performs the necessary state mutations
 * (moving players, updating scores, setting flags).
 *
 * @param stateInfo Pointer to the game state to modify.
 * @param decisions Pointer to the decisions calculated by Referee_Analyze.
 */
void Referee_Apply(StateInfo* stateInfo, const RefereeDecisions* decisions);

#endif // REFEREE_APPLY_H
