#ifndef EXECUTE_ACTIONS_H
#define EXECUTE_ACTIONS_H

#include "globals.h"

// The INGEST gate plus the engine's actualization order. `channels` is drained here and left empty:
// it is an input to this tick, not somewhere intent lives.
void execute_actions(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions, IntentChannels* channels,
    int* playSoundEffect
);

// Append one message to a team's channel. Silently refusing to grow: a full channel records that it
// overflowed and the state validator fails the frame.
void intent_push(IntentChannel* channel, IntentMessage message);
void init_execute_actions(MatchSession* match, ClientInputState* clientInput);
void generic_sling_ball(BallInfo* ballInfo, float x, float y, float z);
void update_meters(MatchSession* match, const ClientInputState* clientInput);

#endif /* EXECUTE_ACTIONS_H */
