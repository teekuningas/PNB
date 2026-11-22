#include "frame_events.h"

void clear_frame_events(GameEvents* events)
{
    events->catch_made = 0;
    events->player_arrived_at_base = 0;
    events->pitch_released = 0;
    events->ball_hit_by_bat = 0;
    events->ball_missed_by_bat = 0;
    events->ball_hit_ground = 0;
    events->free_walk_accepted = 0;
    events->batter_entered = 0;
    events->homeward_runs_started = 0;
}
