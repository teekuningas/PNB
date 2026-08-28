#ifndef ACTION_INVOCATIONS_H
#define ACTION_INVOCATIONS_H

#include "globals.h"

// The human half of the CONTROL stage: key states in, declared intent out.
// Reads scoreboard (const) for team assignment, referee (const) for base ownership (to interpret a
// base-run key as advance vs. come-back), halfInningState (const) for §12's spent-order clause (the
// batter cursor offers only what the rules allow), and keyStates for input.
// Writes each team's intent channel, clientInput (the human's tap-window / charge memory), and — for
// the actions not yet moved onto the channel — match->aF.
void action_invocations(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, const Scoreboard* scoreboard,
    const RefereeState* referee, const HalfInningState* halfInningState, IntentChannels* channels
);

#endif /* ACTION_INVOCATIONS_H */
