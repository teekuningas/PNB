#ifndef ACTION_INVOCATIONS_H
#define ACTION_INVOCATIONS_H

#include "globals.h"

// The human half of the CONTROL stage: key states in, declared intent out.
// Reads scoreboard (const) for team assignment and referee (const) for base ownership
// (to interpret a base-run key as advance vs. come-back), keyStates for input.
// Writes each team's intent channel, clientInput (the human's tap-window / charge memory), and — for
// the actions not yet moved onto the channel — match->aF.
void action_invocations(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, const Scoreboard* scoreboard,
    const RefereeState* referee, IntentChannels* channels
);

#endif /* ACTION_INVOCATIONS_H */
