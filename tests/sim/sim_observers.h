#ifndef SIM_OBSERVERS_H
#define SIM_OBSERVERS_H

#include "sim_harness.h"
#include <stdio.h>

/**
 * @file sim_observers.h
 * @brief Built-in per-frame observers for the headless sim harness.
 *
 * Each observer is a (SimFrameHook, ctx) pair attached via sim_attach(). They are
 * the "attach to the running game and watch it" mechanism: assert invariants,
 * record a trace, or fingerprint the run for determinism checks.
 */

/* ---- Invariant observer ------------------------------------------------ *
 * Asserts the game never enters an invalid or stuck state. The production
 * state_validator (unique base occupancy, single batter) already guards the
 * frame via flowControl.pause; this adds bounds, monotonicity, and a no-stall
 * (liveness) check, and records that the game actually progressed. On a
 * violation it calls sim_fail(), which stops the run. */
typedef struct {
    long stall_limit; // max frames with no progress before declaring a stall

    // internal progress tracking (do not set)
    int initialized;
    long last_progress_frame;
    int p_outs, p_balls, p_strikes, p_runs0, p_runs1, p_inning, p_hasBall, p_pitchState;

    // observed totals (readable after the run for "game progressed" assertions)
    long pitches; // pitchReleased events seen
    long count_changes; // frames where outs/balls/strikes/runs/inning moved
} InvariantObserver;

void invariant_observer_init(InvariantObserver* o, long stall_limit);
void invariant_observer_hook(const SimGame* g, void* ctx);

/* ---- Trace observer ---------------------------------------------------- *
 * Appends one CSV row per (sampled) frame: the same key variables the debug
 * state dump tracks. This is the "see how the values develop" artifact and is
 * diffable across seeds and versions. */
typedef struct {
    FILE* f;
    long every; // sample period in frames (1 = every frame)
    int header_written;
} TraceObserver;

void trace_observer_init(TraceObserver* o, FILE* f, long every);
void trace_observer_hook(const SimGame* g, void* ctx);

/* ---- Checksum observer ------------------------------------------------- *
 * Folds the full MatchSession into a rolling FNV-1a hash each frame. Two runs
 * with the same seed must produce the same hash — the determinism fingerprint. */
typedef struct {
    unsigned long long hash;
} ChecksumObserver;

void checksum_observer_init(ChecksumObserver* o);
void checksum_observer_hook(const SimGame* g, void* ctx);

/* ---- shared helper ----------------------------------------------------- */

/** Count batting-team players currently standing on 1st/2nd/3rd base. */
int sim_runners_on_base(const SimGame* g);

#endif /* SIM_OBSERVERS_H */
