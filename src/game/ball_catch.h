#ifndef BALL_CATCH_H
#define BALL_CATCH_H

#include "field.h"
#include "world.h"

// If a fielder is within reach of the loose ball and may take it, he catches it; catchMade is raised here.
void check_if_ball_can_be_caught(MatchSession* match, const FieldPositions* field_positions, int* play_sound_effect);

#endif /* BALL_CATCH_H */
