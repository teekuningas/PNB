#ifndef FIELDER_MOVEMENT_H
#define FIELDER_MOVEMENT_H

#include "globals.h"

// The controlled fielder's walk — an ENGINE BEHAVIOUR, not a controller and not "AI".
//
// A controller says WHERE (INTENT_MOVE_TARGET, which ingestion turns into
// catchingState.controlledMoveTarget); this walks the fielder there, identically for every producer,
// at zero wire cost. Runs once per tick from the engine's actualization order.
//
// It is idempotent by construction: the walk is a function of (position, destination), so a producer
// restating its destination changes nothing, and a fielder standing on its destination is left
// exactly as it is. That is the property the whole message shape rests on — and it is not a nicety:
// without it a restated target makes the fielder oscillate (arrive, stop, be sent to the point it is
// standing on, start), which on measurement was enough to drive the home-run contest into an
// illegal state.
void update_controlled_fielder_movement(MatchSession* match);

#endif /* FIELDER_MOVEMENT_H */
