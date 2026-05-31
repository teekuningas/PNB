#ifndef PITCHING_SYSTEM_H
#define PITCHING_SYSTEM_H

#include "globals.h"
#include "pitching_physics.h"

// Constants moved from execute_actions.c
#define PITCHER_MOVE_AWAY_OFFSET 0.1f + DISTANCE_FROM_HOME_LOCATION_THRESHOLD + TARGET_ACHIEVED_THRESHOLD

void start_pitch(StateInfo* stateInfo);
void continue_pitch(StateInfo* stateInfo);
void release_pitch(StateInfo* stateInfo);
void reset_pitching_system(StateInfo* stateInfo);
void update_pitching_meter(StateInfo* stateInfo);

#endif
