#ifndef STATE_ADAPTER_H
#define STATE_ADAPTER_H

#include "globals.h"

// Synchronizes the new Enum-based state from the old Flag-based state
void update_player_state_from_flags(PlayerInfo* p);

// Synchronizes the old Flag-based state from the new Enum-based state
// This ensures legacy code (renderer, AI) still works even if logic uses Enums
void update_player_flags_from_state(PlayerInfo* p);

// Synchronizes global game enums (Period, Events)
void update_global_state_from_flags(StateInfo* state);
void update_global_flags_from_state(StateInfo* state);

#endif /* STATE_ADAPTER_H */
