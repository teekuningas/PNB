#ifndef BATTING_SYSTEM_H
#define BATTING_SYSTEM_H

#include "globals.h"

#define BAT_LOAD_MAX (4 * 9)
#define BAT_SWING_MAX (4 * 13)

// The batter's arc around the pitching plate. `batter_angle` is where on it he stands, and the
// batting physics doubles it into the launch heading (theta = -batter_angle*2), so this is AIM and
// not a stance: it is the batting side's answer to the same question the fielder's destination
// answers on the other side of the field.
//
// Public because the human's aim widget nudges a target across the same arc at the same rate, so the
// body tracks the cursor exactly. The LIMIT is a physical property of the arc, enforced by the
// engine's walk — a producer may declare anything and simply walks to the end and stands there, the
// same bounded failure a destination on the field boundary gives a fielder.
#define BATTER_ANGLE_SPEED_CONSTANT 0.02f
#define BATTER_ANGLE_LIMIT (PI / 7)

// How far off the centre of the plate the ball may be and still be reachable by the bat. A swing at
// a ball outside this is a miss whatever the timing was — which is why it is the OTHER miss cause,
// and why an instrument that cannot tell the two apart cannot say whether a timing change helped.
#define BALL_MAX_OFFSET 1.0f

void init_batting_system(MatchSession* match);

void seat_batter(MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions, int index);
void select_power(MatchSession* match);
void select_angle(MatchSession* match);
// The batter's frame. `aimDeclared`/`aim` is this tick's INTENT_SWING_ANGLE, ingested: the batting
// side's aim as an absolute angle on the arc, which the engine walks the body toward at
// BATTER_ANGLE_SPEED_CONSTANT. A tick with nothing declared leaves the batter where he stands.
void update_batting(
    MatchSession* match, const RefereeState* referee, const BetweenPitchState* betweenPitchState,
    const FieldPositions* fieldPositions, int aimDeclared, float aim, int* playSoundEffect
);
void update_batting_meter(MatchSession* match);

#endif
