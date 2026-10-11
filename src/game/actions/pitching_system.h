#ifndef PITCHING_SYSTEM_H
#define PITCHING_SYSTEM_H

#include "globals.h"
#include "pitching_physics.h"

// Constants moved from execute_actions.c
#define PITCHER_MOVE_AWAY_OFFSET 0.1f + DISTANCE_FROM_HOME_LOCATION_THRESHOLD + TARGET_ACHIEVED_THRESHOLD

void reset_pitching_system(MatchSession* match);

// Is a pitch physically on its way — the pitcher winding up, or the ball in the air?
//
// It exists because `pRAI.pitch_state` cannot answer this and looks as though it can. Between a
// pitch's RESOLUTION and the next pitch's RELEASE, consolidation stamps `pitch_state` back to NONE
// on every frame the resolved pitch's `pitchResult` is still standing — and that flag is only
// cleared at the next release. So from the second pitch of every half-inning onward, `pitch_state`
// reads NONE right through the windup. The windup is therefore asked of the engine's own pitch
// mutex, which nothing outside this file touches; only the flight is asked of `pitch_state`, where
// AIRBORNE means exactly what it says and nothing is stale.
//
// One definition, because two callers had improvised their own and disagreed: the batter's window
// (batting_system.c) and the batting controller's plan lifetime (batting_ai.c). Both were reading
// the lying signal, which cost the batter the entire power beat on all but a half-inning's opening
// pitch. Other callers still ask `pitch_state` the same question in their own words — see the debt
// table.
int pitch_is_being_delivered(const MatchSession* match);

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
    MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions,
    int correct_pitches_received, const PitchValues* declared
);

#endif
