#ifndef THROWING_SYSTEM_H
#define THROWING_SYSTEM_H

#include "globals.h"

// The engine-owned windup is a FIXED length (in frames), the same for every throw — NOT scaled by the
// declared power. Rationale (owner, 2026-06-25): a powerful throw already costs the human the time spent
// charging the meter; making the windup *also* scale with power penalized them twice ("wait to charge,
// then wait for a long windup"). Power still drives the release velocity (throw_release), so a charged
// throw still flies farther/faster — it just leaves the hand on a fixed, prompt cadence (~0.5 s at 30).
//
// This is a pure FEEL knob — tune freely. It is NOT load-bearing for correctness: a shorter windup
// re-times the catching relay and can provoke a throw_going_on/current_catching_action drift, but the
// safety-net heal in execute_actions clears that drift, so no value deadlocks (verified: 30 runs the
// 24-seed sim net clean; the drift's root cause — the AI's duplicate pitch-lock machine — is deleted at
// the pitch slice). Changing this re-times the AI, so re-baseline the sim determinism hash deliberately.
#define THROW_WINDUP 30

// Default declared throw power [0,1]. The AI declares this atomically; the human grows its own power with
// the client-local charge gesture below (action_invocations.c). Kept for the AI path.
#define THROW_POWER_DEFAULT (3.0f / 4.0f)

// Human throw-charge gesture tuning (client-local). The charge counter rises one step per held frame up
// to THROW_CHARGE_MAX (≈ how long a full-power hold takes, in frames), then maps to a declared power in
// [THROW_POWER_MIN, 1]. A bare tap therefore still throws (at THROW_POWER_MIN), never a dead 0-power
// throw. Feel constants — tune freely at the graphical pass; no game logic depends on the exact values.
#define THROW_CHARGE_MAX 45
#define THROW_POWER_MIN 0.2f

// Map a held charge counter [0, THROW_CHARGE_MAX] to a declared throw power [THROW_POWER_MIN, 1].
float throw_charge_to_power(int charge);

// Client-visual only: while a human is charging a throw, face the ball-holder toward the latched target
// base. Lives with the movement/orientation code (next to update_controlled_player_speed) and is called
// from execute_actions after the per-direction move handling, so it is the single orientation writer for
// a charging thrower. Never affects the throw outcome (the direction is computed at declaration).
void update_thrower_facing(MatchSession* match, const FieldPositions* fieldPositions);

void init_throwing_system(MatchSession* match);

void prepare_throw(MatchSession* match, const FieldPositions* fieldPositions, BaseID base);

void throw_release(MatchSession* match);
void throw_load(MatchSession* match, BaseID base, float power);
void fielder_move(MatchSession* match, int direction);
void fielder_stop_move(MatchSession* match, int direction);
void drop_ball(MatchSession* match);
void update_controlled_player_speed(MatchSession* match);

#endif
