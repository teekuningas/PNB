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
// What the batter's bar should SHOW this frame — the whole of it, answered in one place.
//
// The input layer answers it because the bar IS the gesture: which marks exist and where is a fact
// about the widget, and nothing outside this file may look inside one. The renderer draws what this
// says and interprets nothing.
//
// It replaces a single float that could not say what it meant. Zero read both as "the cursor is at
// the far left" and as "there is no cursor", and the two are genuinely different states of the
// gesture: between committing a power and the ball leaving the pitcher's hand the batter has NO
// moving cursor — he has a committed mark and a wait. That beat is the batter's alone; the pitcher's
// aim meter arms on the same frame his power locks, so for him "a marker is descending" and "a power
// is committed" are the same condition, and a display keyed on the first happened to be right.
typedef struct _SwingMeterView {
    int cursorLive; // a marker is moving: the power sweep, or the elevation descent
    float cursor; // [0,1] where it is — meaningless unless cursorLive
    int powerCommitted; // a power has been declared and this gesture still holds it
    float power; // [0,1] where it was committed — meaningless unless powerCommitted
} SwingMeterView;

SwingMeterView swing_widget_view(const SwingWidget* w);

void action_invocations(
    MatchSession* match, ClientInputState* clientInput, const KeyStates* key_states, const Scoreboard* scoreboard,
    const RefereeState* referee, const HalfInningState* halfInningState, IntentChannels* channels
);

#endif /* ACTION_INVOCATIONS_H */
