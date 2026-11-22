#ifndef PLAYER_EVENTS_H
#define PLAYER_EVENTS_H

#include <stdbool.h>

typedef enum {
    PLAYER_ACTIVITY_NONE,
    PLAYER_ACTIVITY_RESET,
    PLAYER_ACTIVITY_MOVEMENT_CANCELLED,
    PLAYER_ACTIVITY_TARGET_REACHED,
    PLAYER_ACTIVITY_WALKING_TO_TARGET,
    PLAYER_ACTIVITY_RUNNING_ROUTE,
    PLAYER_ACTIVITY_CONTROLLED_RUNNING,
    PLAYER_ACTIVITY_CONTROLLED_ARRIVAL,
    PLAYER_ACTIVITY_PITCH_BEGUN,
    PLAYER_ACTIVITY_PITCH_RELEASED,
    PLAYER_ACTIVITY_PITCH_FAKED,
    PLAYER_ACTIVITY_THROW_BEGUN,
    PLAYER_ACTIVITY_THROW_RELEASED,
    PLAYER_ACTIVITY_BALL_DROPPED,
    PLAYER_ACTIVITY_BALL_CAUGHT,
    PLAYER_ACTIVITY_BATTER_READY,
    PLAYER_ACTIVITY_SWINGING,
    PLAYER_ACTIVITY_BUNTING,
    PLAYER_ACTIVITY_WITHDRAWING
} PlayerActivity;

// Within one physical phase, the latest activity and the latest fresh start are independent:
// catching a returned ball can replace the activity of a throw begun in the same phase.
typedef struct _PlayerActivityEvents {
    PlayerActivity latest;
    PlayerActivity started;
} PlayerActivityEvents;

static inline void record_player_activity(PlayerActivityEvents* events, PlayerActivity activity, bool started)
{
    events->latest = activity;
    if (started) events->started = activity;
}

#endif
