#ifndef THROWING_SYSTEM_H
#define THROWING_SYSTEM_H

#include "globals.h"

// The throw windup is an engine-owned, deterministic clock (ThrowActualization) — the shared "central
// authority" both networked peers agree on. It is the throw analog of the pitch windup, and unifies
// pitch/throw/swing on one pattern: a phased declaration driving one engine windup clock.
//
// Hold-release feel (owner, 2026-07-01): "hold longer = throw harder." The throw is ONE intent
// {direction, power}: the AI declares it in one frame; the human streams it in two (INITIATED{direction}
// then COMMITTED{power}) purely so the windup can run WHILE it picks power on the meter — no double-wait. The
// power is a client value (AI strategy, or a client-local CHARGE widget for the human — the input never reads
// this clock — the engine↔client contract). Once COMMITTED, the ENGINE owns the release instant: it fires
// when this clock reaches throw_windup_total_frames(power) — for BOTH producers.
//
// The windup length spans [MIN, MAX] frames as declared power spans [THROW_POWER_MIN, 1]:
//   - floor power (THROW_POWER_MIN) → MIN frames; even a tap can't beat the min windup (the ball never leaves
//     the hand instantly — that is the point of the physical windup).
//   - power 1 → MAX frames.
// Feel note: keep the client charge duration ≥ this windup (THROW_CHARGE_FRAMES in action_invocations.c) so a
// human's ball releases right on key-up; if charge is faster, the engine briefly holds the ball to finish the
// windup (correct, just less snappy). Pure FEEL knobs — tune freely. Changing MIN/MAX re-times the AI, so
// re-baseline the sim determinism hash.
#define THROW_WINDUP_MIN_FRAMES 8 // floor-power windup (~0.16s at 50Hz)
#define THROW_WINDUP_MAX_FRAMES 45 // max-power windup, gather fully wound (~0.9s at 50Hz)
#define THROW_POWER_MIN 0.2f // floor power (a tap still throws)

// Default declared throw power [0,1]. The AI declares this atomically (THROW_DECL_COMMITTED); the engine
// sizes the windup to it. The human never uses it (power comes from the hold duration).
#define THROW_POWER_DEFAULT (3.0f / 4.0f)

// Frame count of the throw-gather (throw_load) animation mesh sequence. The render arc maps the windup
// clock across these frames (timing dictates animation), so the gather deepens as the windup runs.
#define THROW_LOAD_FRAMES 11

// AI windup sizing (unit-tested): how long to wind up for a declared power (COMMITTED release). The human
// path does not use this — its power is a value declared from the client charge widget.
int throw_windup_total_frames(float power);

void init_throwing_system(MatchSession* match);

// The throw actualizer — the single engine entry point (the throw analog of update_pitch_actualization).
// Reads the phased ThrowDeclaration (cTAF.throw), runs the deterministic windup clock, and releases:
// COMMITTED at throw_windup_total_frames(power); RELEASED (the human's fire edge) with the power declared on
// the declaration (sampled client-side from the charge widget — never read off this clock).
// Consumer-clears the declaration at resolution. Called once per frame from execute_actions. Timing is the
// master; the animation only follows.
void update_throw_actualization(MatchSession* match, const FieldPositions* fieldPositions);

void fielder_move(MatchSession* match, int direction);
void fielder_stop_move(MatchSession* match, int direction);
void drop_ball(MatchSession* match);
void update_controlled_player_speed(MatchSession* match);

#endif
