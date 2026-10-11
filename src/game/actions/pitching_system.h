#ifndef PITCHING_SYSTEM_H
#define PITCHING_SYSTEM_H

#include "globals.h"
#include "pitching_physics.h"

// Constants moved from execute_actions.c
#define PITCHER_MOVE_AWAY_OFFSET 0.1f + DISTANCE_FROM_HOME_LOCATION_THRESHOLD + TARGET_ACHIEVED_THRESHOLD

void reset_pitching_system(MatchSession* match);

// Is a pitch physically on its way — the pitcher winding up, or the ball in the air? Two halves with two
// homes: the windup is the catching action's, the flight is `pitch_state`'s, and this is the one place
// that asks both.
//
// One definition, because callers used to improvise their own and disagree: the batter's window
// (batting_system.c) and the batting controller's plan lifetime (batting_ai.c) once asked `pitch_state`
// alone, back when it also carried a windup state that consolidation wiped on every windup after a
// half-inning's first — which cost the batter the entire power beat on all but the opening pitch. That
// state is gone; `pitch_state` now answers only the question it always answered truthfully.
int pitch_is_being_delivered(const MatchSession* match);

// May a pitch begin in the world as it stands? The INGEST gate asks it of a declared power; the answer names
// the rule that refuses. One home for the rule, asked once, against the world at the top of the tick.
Permission pitch_may_begin(const MatchSession* match, int correct_pitches_received);

// This tick's pitch values, as the INGEST gate let them through. A per-tick command block, never held:
// what a pitch IS once it has begun lives in the CatchingAction, and these only contribute to it.
typedef struct _PitchValues {
    int powerDeclared;
    float power;
    int aimDeclared;
    float direction;
} PitchValues;

// The pitch actualizer: a power begins the windup, an aim is held until release, and the engine's
// clock decides when the ball leaves — aimed, a real pitch; unaimed, a valesyöttö. Called once per
// frame from execute_actions.
void update_pitch_actualization(
    MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions, const PitchValues* declared
);

#endif
