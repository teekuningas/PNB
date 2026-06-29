#ifndef PITCHING_SYSTEM_H
#define PITCHING_SYSTEM_H

#include "globals.h"
#include "pitching_physics.h"

// Constants moved from execute_actions.c
#define PITCHER_MOVE_AWAY_OFFSET 0.1f + DISTANCE_FROM_HOME_LOCATION_THRESHOLD + TARGET_ACHIEVED_THRESHOLD

// Windup cadence (engine-owned, deterministic; the animation follows it — timing dictates animation, never
// the reverse). Three segments: a fixed crouch DOWN, a power-scaled HOLD (a higher toss = the pitcher stays
// crouched longer), and a fixed rise/throw UP. Total = DOWN + power*HOLD_MAX + UP ≈ 2s (low pitch) to 3s
// (high pitch) at the 50Hz update. Pure FEEL/timing knobs, NOT derived from animation frame counts.
// Changing them re-times the AI — re-baseline the sim determinism hash deliberately.
#define PITCH_WINDUP_DOWN_FRAMES 50
#define PITCH_WINDUP_UP_FRAMES 50
#define PITCH_WINDUP_HOLD_MAX 50

// Total windup length for a declared toss power [0,1]: DOWN + power*HOLD_MAX + UP. The producer must
// declare the aim within it; the ball releases (or fakes) at the end. Shared by the engine (release
// timing), the renderer (animation arc), and the contract test.
int pitch_windup_total_frames(float power);

void reset_pitching_system(MatchSession* match);
// The pitch actualizer: read the phased PitchDeclaration, run the windup clock, release on AIMED / fake
// otherwise. Called once per frame from execute_actions.
void update_pitch_actualization(MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions);

#endif
