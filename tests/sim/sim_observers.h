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

/* ---- Box-score observer ------------------------------------------------ *
 * Turns frame-by-frame state transitions into a baseball box score (and, if a
 * log file is attached, a human-readable play-by-play). The CSV trace answers
 * "what were the raw values each frame?"; this answers "what *happened* in the
 * game?" — so a headless run can be followed like a radio broadcast.
 *
 * Every counter is derived from a rising edge or a positive delta (game events
 * themselves are cleared before observers run), so it never double-counts. */
typedef struct {
    FILE* log; // optional play-by-play sink; NULL = count silently

    // internal edge-detection state (do not set)
    int initialized;
    int p_pitchState, p_outs, p_balls, p_strikes, p_runs0, p_runs1, p_inning, p_period;
    int p_batOutcome;
    int p_baseId[2 * PLAYERS_IN_TEAM + JOKER_COUNT];
    int p_state[2 * PLAYERS_IN_TEAM + JOKER_COUNT]; // PlayerUnitState last frame

    // box score (readable after the run)
    long pitches; // pitches released (rising edge into AIRBORNE)
    long contacts; // bat made contact (batOutcome → HIT)
    long whiffs; // swing and a miss (batOutcome → MISSED)
    long strikes_called; // strike-count increments
    long balls_called; // ball-count increments
    long outs_made; // out-count increments
    long runs_scored; // both teams' run increments
    long reached_base; // batters that became runners (HOME → 1st)
    int furthest_base; // furthest base any runner stood on (1..3), 0 if none

    // base-running breakdown — the "stranded vs thrown out vs never tried" question
    long reached_third; // distinct arrivals onto 3rd base
    long ran_from_third; // a runner on 3rd broke for home (entered RUNNING/LEADING)
    long scored_from_third; // a 3rd-base runner reached HOME_SCORED
    long out_from_third; // a 3rd-base runner went OUT while heading home
    long wound_from_third; // a 3rd-base runner was WOUNDED while heading home

    // Batting meter, STYLE-1 swings only. decidedSwingTrigger is the AI's real power intent only
    // for batting style 1 ("normal swing"); styles 0 (bunt) and 2 leave it stale, so their power
    // is NOT measurable this way and is deliberately excluded. Direction (decidedAngle vs the
    // realized batter_angle) is not measured yet — see AI_NOTES.md §2 for the proper redo.
    long s1_swings; // style-1 swings measured
    long s1_power_err_sum; // Σ (actual − intent) in meter steps; ≈ +1 means the AI hit its target
} BoxScoreObserver;

void box_score_observer_init(BoxScoreObserver* o, FILE* log);
void box_score_observer_hook(const SimGame* g, void* ctx);

/* ---- shared helper ----------------------------------------------------- */

/** Count batting-team players currently standing on 1st/2nd/3rd base. */
int sim_runners_on_base(const SimGame* g);

#endif /* SIM_OBSERVERS_H */
