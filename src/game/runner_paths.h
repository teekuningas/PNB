#ifndef RUNNER_PATHS_H
#define RUNNER_PATHS_H

#include "field.h"
#include "world.h"

// A batting player's movement along the bases: on to the next, back to the previous, or a lead off the
// one he holds. One question is still asked here rather than at the gate: the batter does not leave home
// before his §32 right to (run_to_next_base returns without moving him).
void run_to_next_base(MatchSession* match, const FieldPositions* field_positions, int index, BaseID base);
void run_to_previous_base(MatchSession* match, const FieldPositions* field_positions, int index, BaseID base);
void lead_from_base(PlayerInfo* playerInfo, const FieldPositions* field_positions, int index);

#endif /* RUNNER_PATHS_H */
