#ifndef BATTING_SYSTEM_H
#define BATTING_SYSTEM_H

#include "globals.h"

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

// Below this declared power the body bunts rather than swings. A shape of swing, not a rule: it
// picks the animation and how far the batter steps in, and the physics reads the same power either
// way. (Was a meter count of 20 out of a load of 36.)
#define SWING_BUNT_MAX_POWER 0.5555556f

// The declared power [0,1] scaled into the units calculate_batted_ball_velocity expects for its
// magnitude — the one place the old meter's length survives, as a conversion rather than a meter.
#define SWING_POWER_UNITS 36.0f

void init_batting_system(MatchSession* match);

void seat_batter(MatchSession* match, const RefereeState* referee, const FieldPositions* fieldPositions, int index);

// May a swing value be declared into the world as it stands? One question, two callers: the INGEST
// gate asks it to REFUSE, and the human's widget asks it to decide whether to show a meter at all —
// so a client can be helpful without the engine trusting it, the same shape §27's candidate rule has.
//
// It is derived, never stored. "How far along is this swing" is not something a producer tells the
// engine, and a stored "window open" flag would be a second copy of facts that are already durable:
// a batter is batting, a pitch is on its way, and this swing has not already been resolved.
int swing_may_be_declared(const MatchSession* match);

// The batter's frame. `aimDeclared`/`aim` is this tick's INTENT_SWING_ANGLE, ingested: the batting
// side's aim as an absolute angle on the arc, which the engine walks the body toward at
// BATTER_ANGLE_SPEED_CONSTANT. A tick with nothing declared leaves the batter where he stands.
// `passDeclared` is this tick's INTENT_SWING_PASS — the batting side withdrawing a swing it had
// already declared a power for, which is the "väärä!" call and is worth a ball rather than a strike.
// The swing's two VALUES are not parameters: they are held engine state with a lifetime, written by
// the gate into pendingActionState.swing, exactly as a destination or a declaration is.
void update_batting(
    MatchSession* match, const RefereeState* referee, const BetweenPitchState* betweenPitchState,
    const FieldPositions* fieldPositions, int aimDeclared, float aim, int passDeclared, int* playSoundEffect
);

#endif
