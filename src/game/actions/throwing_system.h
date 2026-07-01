#ifndef THROWING_SYSTEM_H
#define THROWING_SYSTEM_H

#include "globals.h"

// The throw windup is an engine-owned, deterministic clock (ThrowActualization) — the shared "central
// authority" both networked peers agree on. It is the throw analog of the pitch windup, and unifies
// pitch/throw/swing on one pattern: a phased declaration driving one engine windup clock.
//
// Hold-release feel (owner, 2026-07-01): the windup length is NOT fixed — it scales with power, and power
// is reached by holding. A human holds KEY_2 + a direction and the gather animation plays WHILE HELD (the
// windup clock runs); releasing reads the power off the clock. The AI declares power atomically and the
// engine sizes the windup to it. Both ride the same clock, so "hold longer = throw harder" is symmetric.
// This restores the pre-refactor gather-while-charging feel that the interim atomic throw lost.
//
// Windup length spans [MIN, MAX] frames as power spans [THROW_POWER_MIN, 1]:
//   - a bare tap releases near MIN → floor power (THROW_POWER_MIN); a throw is never a dead 0-power throw.
//   - a full hold caps at MAX → power 1; the clock rests at MAX until the release edge.
// Pure FEEL knobs — tune freely. Changing them re-times the AI, so re-baseline the sim determinism hash.
#define THROW_WINDUP_MIN_FRAMES 8 // a bare tap: quick release, floor power (~0.16s at 50Hz)
#define THROW_WINDUP_MAX_FRAMES 45 // a full hold: max power, gather fully wound (~0.9s at 50Hz)
#define THROW_POWER_MIN 0.2f // floor power (a tap still throws)

// Default declared throw power [0,1]. The AI declares this atomically (THROW_DECL_COMMITTED); the engine
// sizes the windup to it. The human never uses it (power comes from the hold duration).
#define THROW_POWER_DEFAULT (3.0f / 4.0f)

// Frame count of the throw-gather (throw_load) animation mesh sequence. The render arc maps the windup
// clock across these frames (timing dictates animation), so the gather deepens as the windup runs.
#define THROW_LOAD_FRAMES 11

// Round-trip pair between declared power and windup length (unit-tested):
//   throw_windup_total_frames(power) — AI: how long to wind up for a declared power (COMMITTED release).
//   throw_power_from_windup(timer)   — human: what power a hold of `timer` frames declares (RELEASED edge).
int throw_windup_total_frames(float power);
float throw_power_from_windup(int timer);

void init_throwing_system(MatchSession* match);

// The throw actualizer — the single engine entry point (the throw analog of update_pitch_actualization).
// Reads the phased ThrowDeclaration (cTAF.throw), runs the deterministic windup clock, and releases:
// COMMITTED at throw_windup_total_frames(power); GATHERING when the producer sets RELEASED (power read
// from the clock). Consumer-clears the declaration at resolution. Called once per frame from
// execute_actions. Timing is the master; the animation only follows.
void update_throw_actualization(MatchSession* match, const FieldPositions* fieldPositions);

void fielder_move(MatchSession* match, int direction);
void fielder_stop_move(MatchSession* match, int direction);
void drop_ball(MatchSession* match);
void update_controlled_player_speed(MatchSession* match);

#endif
