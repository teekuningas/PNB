#ifndef GAME_MANIPULATION_H
#define GAME_MANIPULATION_H

#include "globals.h"

// World physics: ball movement, fielder ranking, catches, base arrivals.
// Reads NO referee-owned state — only physical world + field geometry.
void game_manipulation(MatchSession* match, const FieldPositions* field_positions, int* play_sound_effect);
void init_game_manipulation(GameFlowState* gameFlowState);

#endif /* GAME_MANIPULATION_H */
