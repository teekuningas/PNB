#ifndef PITCHING_PHYSICS_H
#define PITCHING_PHYSICS_H

#include "globals.h" // for Vector3D

#define PITCH_BASE_SPEED 0.065f // dy at power 0 (minimum toss height)
#define PITCH_POWER_CONSTANT 0.12f // extra dy per unit power
#define PITCH_ANGLE_CONSTANT 0.15f // dx per unit placement (direction)
#define PITCH_DOWN_MAX 9 // crouch (pitch_down) frame count — render only (frames loaded 1..9)
#define PITCH_UP_MAX 13 // rise/throw (pitch_up) frame count — render only (frames loaded 1..13)
#define VERTICAL_ANGLE_LIMIT 5 // batting vertical-angle clamp (used by batting_system)

// The windup's three segments, in frames: a fixed crouch DOWN, a power-scaled HOLD, a fixed rise UP.
// Engine-owned and deterministic; the animation follows the clock, never the reverse. A higher toss
// holds the crouch longer (≈1.5s low to 2s high at the 50Hz update), which is what makes the
// pitcher's power PHYSICALLY visible to the batter before the ball exists — the only channel the
// fourth law allows the dance to use. Pure FEEL knobs, NOT derived from animation frame counts;
// changing them re-times the AI, so re-baseline the sim determinism hash deliberately.
//
// NOTE (2026-06-30): retuned ~30% faster (was 50/50/50 ≈ 2s–3s). That did not CAUSE bugs — it only
// reshuffles which AI-vs-AI games are played — but it exposed two latent ones the sim net then
// caught: the batting-AI orphaned-lock deadlock and a referee outs>3 double-count (#6).
#define PITCH_WINDUP_DOWN_FRAMES 40
#define PITCH_WINDUP_UP_FRAMES 35
#define PITCH_WINDUP_HOLD_MAX 25

// Total windup frames for a declared toss power. Pure arithmetic over the segments above, so the
// engine's clock, the client's aim meter and the geometry test all size themselves from ONE
// definition rather than three copies of the same sum.
int pitch_windup_total_frames(float power);

// The pitch actualizer (end-state): convert a declared PitchAim into the ball's launch velocity.
// power [0,1] -> toss height (dy); direction [-1,1] -> placement (dx), 0 = straight up over the plate
// (oikea / strike); dz is always 0. The result is a pure function of the DECLARED aim — never a live
// meter — which is the whole point of the pitch slice. Inputs are clamped to their ranges.
Vector3D pitch_velocity_from_aim(float power, float direction);

#endif
