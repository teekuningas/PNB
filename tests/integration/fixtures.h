#ifndef INTEGRATION_FIXTURES_H
#define INTEGRATION_FIXTURES_H

#include "immutable_world.h"

StateInfo* setup_test_state();
void cleanup_test_state(StateInfo* state);
void set_test_player_state(StateInfo* state, int playerIndex, PlayerUnitState newState);

#endif // INTEGRATION_FIXTURES_H
