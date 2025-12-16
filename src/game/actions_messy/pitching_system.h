#ifndef PITCHING_SYSTEM_H
#define PITCHING_SYSTEM_H

#include "globals.h"
#include "pitching_physics.h"

// Constants moved from action_implementation.c
#define PITCHER_MOVE_AWAY_OFFSET 0.1f + DISTANCE_FROM_HOME_LOCATION_THRESHOLD + TARGET_ACHIEVED_THRESHOLD

void startPitch(StateInfo* stateInfo);
void continuePitch(StateInfo* stateInfo);
void releasePitch(StateInfo* stateInfo);
void resetPitchingSystem(void);
void updatePitchingMeter(StateInfo* stateInfo);

void updateAIPitching(StateInfo* stateInfo);

#endif