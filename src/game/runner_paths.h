#ifndef RUNNER_PATHS_H
#define RUNNER_PATHS_H

#include "field.h"
#include "world.h"

// A batting player's movement along the bases: on to the next, back to the previous, or a lead off the
// one he holds. Pure movement mechanics; permissions such as §32 are enforced at the INGEST gate.
void run_to_next_base(MatchSession* match, const FieldPositions* field_positions, int index, BaseID base);
void run_to_previous_base(MatchSession* match, const FieldPositions* field_positions, int index, BaseID base);
void lead_from_base(PlayerInfo* playerInfo, const FieldPositions* field_positions, int index);

#endif /* RUNNER_PATHS_H */
