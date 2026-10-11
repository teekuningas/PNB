#ifndef THROWING_SYSTEM_H
#define THROWING_SYSTEM_H

#include "globals.h"

// The throw windup is an engine-owned, deterministic clock (CatchingAction.timer) — the shared "central
// authority" both networked peers agree on, and the throw's counterpart of the pitch windup.
//
// Hold-release feel (owner, 2026-07-01): "hold longer = throw harder." The throw is two values, a target
// and a power. The AI declares both in one tick; the human declares the target when it starts holding
// and the power when it lets go, purely so the windup can run WHILE it picks power on its meter — no
// double-wait. The power is a client value (AI strategy, or a client-local CHARGE widget — the input never
// reads this clock). Once the power is known, the ENGINE owns the release instant: it fires when this
// clock reaches throw_windup_total_frames(power) — for BOTH producers.
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

// Default declared throw power [0,1]. The AI declares it with the target, in the same tick; the engine
// sizes the windup to it. The human never uses it (power comes from the hold duration).
#define THROW_POWER_DEFAULT (3.0f / 4.0f)

// Frame count of the throw-gather (throw_load) animation mesh sequence. The render arc maps the windup
// clock across these frames (timing dictates animation), so the gather deepens as the windup runs.
#define THROW_LOAD_FRAMES 11

// How long the windup for a declared power lasts (unit-tested). The release waits for it, whoever declared.
int throw_windup_total_frames(float power);

// The point a throw to `base` is aimed at. One definition, used by the throw itself and by any
// controller reasoning about where a throw could go from where.
Vector3D throw_target_point(const FieldPositions* fieldPositions, BaseID base);

// This tick's throw values, as the INGEST gate let them through — a per-tick command block, like the
// pitch's. What a throw IS once it has begun lives in the CatchingAction.
typedef struct _ThrowValues {
    int targetDeclared;
    BaseID target;
    int powerDeclared;
    float power;
} ThrowValues;

// The throw actualizer: a target begins the gather, a power is held on it, and the ball leaves once the
// power is known and its windup has elapsed. Called once per frame from execute_actions. Timing is the
// master; the animation only follows.
void update_throw_actualization(MatchSession* match, const FieldPositions* fieldPositions, const ThrowValues* declared);

void drop_ball(MatchSession* match);

#endif
