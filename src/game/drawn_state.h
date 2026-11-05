#ifndef DRAWN_STATE_H
#define DRAWN_STATE_H

#include "field.h"
#include "rules_state.h"
#include "world.h"

// Advance each player's animation and pick his mesh; a throw's recoil ends here.
void update_models(MatchSession* match);
// The home-run camera cue holds once the ball is home with third base clear.
void update_home_run_camera(MatchSession* match, const FieldPositions* field_positions, const RefereeState* referee);

#endif /* DRAWN_STATE_H */
