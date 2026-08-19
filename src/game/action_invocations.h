#ifndef ACTION_INVOCATIONS_H
#define ACTION_INVOCATIONS_H

#include "globals.h"

// Translates key inputs into ActionFlags based on team control mode.
// Reads scoreboard (const) for team assignment and referee (const) for base ownership
// (to interpret a base-run key as advance vs. come-back), keyStates for input.
// Writes match->aF (ActionFlags) and clientInput (the human's tap-window / charge memory).
void action_invocations(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, const Scoreboard* scoreboard,
    const RefereeState* referee
);

#endif /* ACTION_INVOCATIONS_H */
