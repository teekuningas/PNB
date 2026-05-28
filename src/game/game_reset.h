#ifndef GAME_RESET_H
#define GAME_RESET_H

#include "globals.h"

void reset_physical_world(MatchSession* match, const FieldPositions* field_positions, unsigned int* rng_seed);
void reset_for_new_half_inning(
    MatchSession* match, const FieldPositions* field_positions, const TeamData* team_data, unsigned int* rng_seed
);
void reset_for_foul_play(
    MatchSession* match, const FieldPositions* field_positions, const RefereeState* referee, unsigned int* rng_seed
);
void reset_for_next_pair(MatchSession* match, const FieldPositions* field_positions, unsigned int* rng_seed);

#endif // GAME_RESET_H
