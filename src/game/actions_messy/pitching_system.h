#ifndef PITCHING_SYSTEM_H
#define PITCHING_SYSTEM_H

#include "globals.h"

// Constants moved from action_implementation.c
#define PITCH_BASE_SPEED 0.065f
#define PITCHER_MOVE_AWAY_OFFSET 0.1f + DISTANCE_FROM_HOME_LOCATION_THRESHOLD + TARGET_ACHIEVED_THRESHOLD
#define PITCH_POWER_CONSTANT 0.12f
#define PITCH_ANGLE_CONSTANT 0.15f
#define PITCH_DOWN_MAX 9
#define PITCH_UP_MAX 13
#define VERTICAL_ANGLE_LIMIT 5

void startPitch(StateInfo* stateInfo);
void continuePitch(StateInfo* stateInfo);
void releasePitch(StateInfo* stateInfo);
void resetPitchingSystem(void);

#endif
