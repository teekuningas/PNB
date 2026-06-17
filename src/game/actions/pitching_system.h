#ifndef PITCHING_SYSTEM_H
#define PITCHING_SYSTEM_H

#include "globals.h"
#include "pitching_physics.h"

// Constants moved from execute_actions.c
#define PITCHER_MOVE_AWAY_OFFSET 0.1f + DISTANCE_FROM_HOME_LOCATION_THRESHOLD + TARGET_ACHIEVED_THRESHOLD

void start_pitch(MatchSession* match);
void continue_pitch(MatchSession* match);
void release_pitch(MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions);
void reset_pitching_system(MatchSession* match);
void update_pitching_meter(MatchSession* match);

#endif
