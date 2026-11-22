#ifndef FRAME_EVENTS_H
#define FRAME_EVENTS_H

#include "world.h"

// The frame's events end with the frame: CLEAR, the tick's last stage, empties them, and so does a reset
// that begins the world afresh.
void clear_frame_events(GameEvents* events);

#endif /* FRAME_EVENTS_H */
