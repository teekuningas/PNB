#ifndef ACTION_INVOCATIONS_H
#define ACTION_INVOCATIONS_H

#include "globals.h"

void init_action_invocations(StateInfo* stateInfo);

// Translates key inputs into ActionFlags based on team control mode.
// Reads scoreboard (const) for team assignment, keyStates for input.
// Writes only to match->aF (ActionFlags).
void action_invocations(MatchSession* match, const KeyStates* key_states, const Scoreboard* scoreboard);

#endif /* ACTION_INVOCATIONS_H */
