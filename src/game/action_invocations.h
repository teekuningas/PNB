#ifndef ACTION_INVOCATIONS_H
#define ACTION_INVOCATIONS_H

#include "globals.h"

// The human half of the CONTROL stage: key states in, declared intent out.
// Reads scoreboard (const) for team assignment, referee (const) for base ownership (to interpret a
// base-run key as advance vs. come-back), halfInningState (const) for §12's spent-order clause (the
// batter cursor offers only what the rules allow), and keyStates for input.
// Writes each team's intent channel and clientInput (the human's tap windows, charge and meter
// memory) and NOTHING ELSE. It has no write access to the world beyond the channel: every action a
// human can take is now a declared message, so there is no flag left for this stage to set.
// What the swing meter reads right now, for the display only. Lives here because the widget does,
// and nothing else may look inside it.
float swing_widget_display(const SwingWidget* w);

void action_invocations(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, const Scoreboard* scoreboard,
    const RefereeState* referee, const HalfInningState* halfInningState, const BetweenPitchState* betweenPitchState,
    IntentChannels* channels
);

#endif /* ACTION_INVOCATIONS_H */
