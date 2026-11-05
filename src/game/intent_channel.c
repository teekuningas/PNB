#include "intent_channel.h"

// A full channel raises `overflowed` rather than growing or wrapping. Dropping the oldest message
// would be the same bug with better manners: an intent silently lost is a divergence between two
// machines that agreed on everything else.
void intent_push(IntentChannel* channel, IntentMessage message)
{
    if (channel->count >= INTENT_CHANNEL_CAPACITY) {
        channel->overflowed = 1;
        return;
    }
    channel->message[channel->count] = message;
    channel->count++;
}
