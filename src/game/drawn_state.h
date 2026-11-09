#ifndef DRAWN_STATE_H
#define DRAWN_STATE_H

#include "world.h"

// Advance each player's animation and pick his mesh; physical clocks are read-only inputs.
void update_models(MatchSession* match);

#endif /* DRAWN_STATE_H */
