#ifndef PITCHING_SYSTEM_H
#define PITCHING_SYSTEM_H

#include "globals.h"
#include "pitching_physics.h"

// Constants moved from execute_actions.c
#define PITCHER_MOVE_AWAY_OFFSET 0.1f + DISTANCE_FROM_HOME_LOCATION_THRESHOLD + TARGET_ACHIEVED_THRESHOLD

// Fixed windup length, in frames — the engine-owned deterministic cadence of the pitch dance. The
// producer must declare power and aim within this window; the ball releases (or fakes) at the end. A pure
// FEEL/timing knob, NOT derived from any animation frame count (timing dictates animation, never the
// reverse). Changing it re-times the AI — re-baseline the sim determinism hash deliberately.
#define PITCH_WINDUP_FRAMES 40

void reset_pitching_system(MatchSession* match);
// The pitch actualizer: read the phased PitchDeclaration, run the windup clock, release on AIMED / fake
// otherwise. Called once per frame from execute_actions.
void update_pitch_actualization(MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions);

#endif
