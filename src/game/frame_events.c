#include "frame_events.h"

void clear_frame_events(GameEvents* events)
{
    events->catchMade = 0;
    events->playerArrivedAtBase = 0;
    events->pitchReleased = 0;
    events->ballHitByBat = 0;
    events->ballMissedByBat = 0;
    events->ballHitGround = 0;
    events->freeWalkAccepted = 0;
    events->batterEntered = 0;
    events->homeward_runs_started = 0;
}
