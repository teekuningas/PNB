#ifndef GAME_MANIPULATION_H
#define GAME_MANIPULATION_H

#include "globals.h"

// World physics: ball movement, fielder ranking, catches, base arrivals.
// Reads referee state (via get_base_controller for base safety queries).
void game_manipulation(
    MatchSession* match, const FieldPositions* field_positions, const RefereeState* referee, int* play_sound_effect
);
void init_game_manipulation(GameFlowState* gameFlowState);

#endif /* GAME_MANIPULATION_H */
