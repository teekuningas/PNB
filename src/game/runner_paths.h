#ifndef RUNNER_PATHS_H
#define RUNNER_PATHS_H

#include "player_motion.h"

// A batting player's movement along the bases: on to the next, back to the previous, or a lead off the
// one he holds. Whether he may is not asked here — the gate and the referee have already decided.
void run_to_next_base(MatchSession* match, const FieldPositions* field_positions, int index, BaseID base);
void run_to_previous_base(MatchSession* match, const FieldPositions* field_positions, int index, BaseID base);
void lead_from_base(
    PlayerInfo* playerInfo, PlayerRuntimeState* playerRuntime, const FieldPositions* field_positions, int index
);

#endif /* RUNNER_PATHS_H */
