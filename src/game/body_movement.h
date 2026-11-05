#ifndef BODY_MOVEMENT_H
#define BODY_MOVEMENT_H

#include "globals.h"

// Every body takes one step toward its target and faces where it should; arrivals are raised here.
void player_location_orientation_and_targets(MatchSession* match, const FieldPositions* field_positions);
// Runners who arrived this frame settle, or carry on along their path.
void base_runner_movements_on_base_arrivals(MatchSession* match, const RefereeState* referee);

#endif /* BODY_MOVEMENT_H */
