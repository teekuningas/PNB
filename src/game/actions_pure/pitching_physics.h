#ifndef PITCHING_PHYSICS_H
#define PITCHING_PHYSICS_H

#include "globals.h" // for Vector3D

#define PITCH_BASE_SPEED 0.065f // dy at power 0 (minimum toss height)
#define PITCH_POWER_CONSTANT 0.12f // extra dy per unit power
#define PITCH_ANGLE_CONSTANT 0.15f // dx per unit placement (direction)
#define PITCH_DOWN_MAX 9 // crouch (pitch_down) frame count — render only (frames loaded 1..9)
#define PITCH_UP_MAX 13 // rise/throw (pitch_up) frame count — render only (frames loaded 1..13)
#define VERTICAL_ANGLE_LIMIT 5 // batting vertical-angle clamp (used by batting_system)

// The pitch actualizer (end-state): convert a declared PitchAim into the ball's launch velocity.
// power [0,1] -> toss height (dy); direction [-1,1] -> placement (dx), 0 = straight up over the plate
// (oikea / strike); dz is always 0. The result is a pure function of the DECLARED aim — never a live
// meter — which is the whole point of the pitch slice. Inputs are clamped to their ranges.
Vector3D pitch_velocity_from_aim(float power, float direction);

#endif
