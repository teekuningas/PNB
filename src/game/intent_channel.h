#ifndef INTENT_CHANNEL_H
#define INTENT_CHANNEL_H

#include "globals.h"

// The channel's producer-side operation: append one message to a team's channel. Silently refusing to
// grow: a full channel records that it overflowed and the state validator fails the frame. Its other
// operation, draining, is the INGEST gate's alone.
void intent_push(IntentChannel* channel, IntentMessage message);

#endif /* INTENT_CHANNEL_H */
