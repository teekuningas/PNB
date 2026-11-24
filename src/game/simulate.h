#ifndef SIMULATE_H
#define SIMULATE_H

#include "field.h"
#include "world.h"

// SIMULATE — stage 4 of the tick. Pure physical step (cannot see GameRulesState): the ball flies and
// meets the ground, a fielder may meet the ball, the fielders' behaviours, every body moves and runners
// arrive, the held ball follows its holder, and a thrower's recovery clock runs down. Three of the
// frame's events are born here (ball_hit_ground, catch_made, player_arrived_at_base), each inside the
// step that makes it.
void simulate(MatchSession* match, const FieldPositions* field_positions, int* play_sound_effect);

#endif /* SIMULATE_H */
