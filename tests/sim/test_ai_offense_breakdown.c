#include "test_helpers.h"
#include "sim_harness.h"
#include "sim_observers.h"
#include <stdlib.h>

// Characterise AI offense across many seeds × a full first period, and then hold the
// characterisation to asserted bands.
//
// This test used to print its numbers and assert almost nothing (contacts != 0, stalls == 0),
// which meant an AI that swung at 40% of pitches, or whose direction fan collapsed to centre,
// or that stopped reaching base, went green. The numbers themselves lived in commit messages,
// so every session re-derived its own baseline and a slow drift across slices was invisible.
//
// The bands below fix both halves of that: the currently-measured value sits beside each band
// in the table, so the baseline lives in the repo, and the band is wide enough to survive a
// legitimate re-baseline while still catching degradation. They exist because the
// slice 1a moves when the AI observes the world and therefore re-baselines the determinism hash
// BY DESIGN — the hash cannot gate that change, so this net has to.
//
// WHAT THIS TEST CANNOT SEE. The net reports runs=0. The batter is weak by
// design until the swing slice, so no run is ever scored here, and nothing downstream of a run
// — the award paths, should_period_end, the pool refresh, period transitions — is covered by
// these bands, by the box score, or by the determinism hash. For referee scoring and flow work,
// tier 3 (scenario) is the primary net. Deliberately, no band mentions runs: a green band must
// not imply coverage it does not have. Slice 4 is what gives this tier teeth over that half of
// the rules.
//
// Set SIM_PBP=1 to also see the play-by-play.

#define SEED_COUNT 24
#define PERIOD_MAX_FRAMES 120000L
#define STALL_LIMIT 20000L
#define BAND_MAX 16

// Stay strictly inside the first period: it ends at inning == halfInningsInPeriod (4), and the
// period boundary hands off to a menu the single-phase sim harness does not yet autopilot
// (a documented deferred gap). Three half-innings of offense per seed, no boundary crossing.
static int three_half_innings_done(const SimGame* g)
{
    return (g->state->rules->scoreboard.inning - g->start_inning) >= 3 || g->state->screen != SCREEN_GAME;
}

// One asserted property of the AI's play. `measured` is the value this run produced; `lo`/`hi`
// are the band it must fall inside; `baseline` records what the number was when the band was
// written, so a drift is readable without digging up an old PR.
typedef struct {
    const char* name;
    double measured;
    double lo;
    double hi;
    const char* baseline;
    const char* guards; // the degradation this band exists to catch
} Band;

static void band_add(Band* bands, int* n, Band b)
{
    if (*n < BAND_MAX) bands[(*n)++] = b;
}

static double percent(long part, long whole)
{
    return whole ? 100.0 * (double)part / (double)whole : 0.0;
}

int test_ai_offense_breakdown(void)
{
    long T_pitches = 0, T_contacts = 0, T_whiffs = 0, T_strikes = 0, T_balls = 0, T_outs = 0;
    long T_reached_base = 0, T_reached_third = 0, T_ran_third = 0, T_scored_third = 0;
    long T_out_third = 0, T_wound_third = 0, T_runs = 0;
    long T_s1_swings = 0, T_s1_err = 0;
    long T_fouls = 0, T_power_sum = 0, T_power_n = 0, T_dir[5] = {0};
    int T_power_min = 9999, T_power_max = -9999;
    int seeds_reached_third = 0, seeds_ran_third = 0, seeds_incomplete = 0, seeds_stalled = 0;
    long T_recoveries = 0, T_recovery_frames = 0, T_recovery_max = 0, T_abandoned = 0;
    long T_chase_samples = 0, T_step_frames = 0;
    double T_chase_dist = 0.0, T_step_sum = 0.0;

    for (int s = 0; s < SEED_COUNT; s++) {
        unsigned int seed = 0xA11CE000u + (unsigned int)s * 0x9E3779B1u;

        GameSetup setup;
        sim_make_normal_setup(&setup, 0, 1, CONTROL_AI, CONTROL_AI);
        SimGame* g = sim_create(&setup, seed);
        ASSERT_NOT_NULL(g, "sim_create returned NULL");

        InvariantObserver inv;
        invariant_observer_init(&inv, STALL_LIMIT);
        sim_attach(g, invariant_observer_hook, &inv);

        BoxScoreObserver box;
        box_score_observer_init(&box, getenv("SIM_PBP") ? stdout : NULL);
        sim_attach(g, box_score_observer_hook, &box);

        FieldingObserver fld;
        fielding_observer_init(&fld);
        sim_attach(g, fielding_observer_hook, &fld);

        long frames = sim_run_until(g, three_half_innings_done, PERIOD_MAX_FRAMES);
        if (frames < 0) seeds_incomplete++; // budget hit or failure; counters still valid

        // A stall is a real finding (an inning-rollover hang), but it must not abort the
        // measurement — record it, keep the counters gathered up to the stall, continue.
        if (g->failed) {
            seeds_stalled++;
            printf("  seed 0x%08X stalled: %s\n", seed, g->fail_reason);
        }

        T_pitches += box.pitches;
        T_contacts += box.contacts;
        T_whiffs += box.whiffs;
        T_strikes += box.strikes_called;
        T_balls += box.balls_called;
        T_outs += box.outs_made;
        T_reached_base += box.reached_base;
        T_reached_third += box.reached_third;
        T_ran_third += box.ran_from_third;
        T_scored_third += box.scored_from_third;
        T_out_third += box.out_from_third;
        T_wound_third += box.wound_from_third;
        T_runs += box.runs_scored;
        T_s1_swings += box.s1_swings;
        T_s1_err += box.s1_power_err_sum;
        T_fouls += box.fouls;
        T_power_sum += box.contact_power_sum;
        T_power_n += box.contact_power_n;
        if (box.contact_power_n > 0) {
            if (box.contact_power_min < T_power_min) T_power_min = box.contact_power_min;
            if (box.contact_power_max > T_power_max) T_power_max = box.contact_power_max;
        }
        for (int b = 0; b < 5; b++)
            T_dir[b] += box.dir_bins[b];

        T_recoveries += fld.recoveries;
        T_recovery_frames += fld.recovery_frames_sum;
        if (fld.recovery_frames_max > T_recovery_max) T_recovery_max = fld.recovery_frames_max;
        T_abandoned += fld.abandoned;
        T_chase_samples += fld.chase_samples;
        T_chase_dist += fld.chase_dist_sum;
        T_step_frames += fld.step_frames;
        T_step_sum += fld.step_sum;

        if (box.reached_third > 0) seeds_reached_third++;
        if (box.ran_from_third > 0) seeds_ran_third++;

        sim_destroy(g);
    }

    printf(
        "\n  AI offense over %d seeds × 3 half-innings (incomplete=%d, stalled=%d):\n", SEED_COUNT, seeds_incomplete,
        seeds_stalled
    );
    printf(
        "    pitches=%ld contacts=%ld (%.0f%%) whiffs=%ld | strikes=%ld balls=%ld outs=%ld runs=%ld\n", T_pitches,
        T_contacts, percent(T_contacts, T_pitches), T_whiffs, T_strikes, T_balls, T_outs, T_runs
    );
    printf(
        "    bases: reachedFirst=%ld reachedThird=%ld (in %d seeds)\n", T_reached_base, T_reached_third,
        seeds_reached_third
    );
    printf(
        "    third→home: ranForHome=%ld (in %d seeds) → scored=%ld, OUT=%ld, wounded=%ld\n", T_ran_third,
        seeds_ran_third, T_scored_third, T_out_third, T_wound_third
    );
    // Style-1 power-meter accuracy (intent vs realized). ≈ +1 means correct meter use.
    printf(
        "    meter (style-1 swings only): n=%ld meanPowerErr=%+.2f steps (expect ~+1)\n", T_s1_swings,
        T_s1_swings ? (double)T_s1_err / T_s1_swings : 0.0
    );
    // Actualized out-of-bounds rate: a hit is either OUT OF BOUNDS or not (a caught ball counts as
    // in-bounds — "else"). Most hits should land fair, but some fouls are wanted for variety.
    printf(
        "    out of bounds: %ld of %ld contacts = %.0f%% (rest land fair or are caught)\n", T_fouls, T_contacts,
        percent(T_fouls, T_contacts)
    );
    // Actualized batted-ball power (0..36): the AI should generally hit with real strength.
    printf(
        "    power (all contacts): n=%ld mean=%.1f min=%d max=%d (of 36 max)\n", T_power_n,
        T_power_n ? (double)T_power_sum / T_power_n : 0.0, T_power_n ? T_power_min : 0, T_power_n ? T_power_max : 0
    );
    // Normal-swing (style-1) direction spread (realized launch angle, right→left, 5 equal buckets):
    // should be a broad, roughly uniform fan — NOT collapsed to center, NOT piled at the extremes.
    printf("    style-1 direction R→L: [%ld %ld %ld %ld %ld]\n", T_dir[0], T_dir[1], T_dir[2], T_dir[3], T_dir[4]);
    printf(
        "    fielding: recoveries=%ld abandoned=%ld meanFrames=%.2f maxFrames=%ld meanChase=%.3f meanStep=%.5f\n",
        T_recoveries, T_abandoned, T_recoveries ? (double)T_recovery_frames / (double)T_recoveries : 0.0,
        T_recovery_max, T_chase_samples ? T_chase_dist / (double)T_chase_samples : 0.0,
        T_step_frames ? T_step_sum / (double)T_step_frames : 0.0
    );

    // A stall IS a hard defect (an AI-vs-AI deadlock), distinct from the quality bands below:
    // the sim tier is the regression net, so a silent stall must not pass green.
    if (seeds_stalled != 0) {
        printf("  %d seed(s) stalled (AI-vs-AI deadlock)\n", seeds_stalled);
        return TEST_FAILED;
    }

    // ---- the bands -------------------------------------------------------------------
    // Every baseline below was measured on this branch at sim hash 204f50ffa89e8b0a
    // (2026-08-17, 24 seeds × 3 half-innings — re-baselined with the control-stage slice).

    long dir_total = 0, dir_min = -1, dir_max = -1;
    for (int b = 0; b < 5; b++) {
        dir_total += T_dir[b];
        if (dir_min < 0 || T_dir[b] < dir_min) dir_min = T_dir[b];
        if (T_dir[b] > dir_max) dir_max = T_dir[b];
    }

    Band bands[BAND_MAX];
    int band_count = 0;

    band_add(
        bands, &band_count,
        (Band){"pitches thrown", (double)T_pitches, 300, 1500, "627",
               "the pitcher stops pitching, or at-bats never end"}
    );
    band_add(
        bands, &band_count,
        (Band){"contact rate %", percent(T_contacts, T_pitches), 70, 100, "90",
               "the batter stops meeting the ball (a swing that no longer connects)"}
    );
    band_add(
        bands, &band_count,
        (Band){"balls called", (double)T_balls, 10, 300, "58",
               "pitch aim collapsing onto the plate — every pitch a called strike"}
    );
    band_add(
        bands, &band_count,
        (Band){"outs recorded", (double)T_outs, 150, 240, "216",
               "half-innings ending some other way than on three outs"}
    );
    band_add(
        bands, &band_count,
        (Band){"batters reaching first", (double)T_reached_base, 50, 400, "137",
               "contact that never turns into a runner"}
    );
    band_add(
        bands, &band_count,
        (Band){"batted-ball power mean (of 36)", T_power_n ? (double)T_power_sum / T_power_n : 0.0, 14, 30, "21.7",
               "the swing going soft — the meter starved, as bug #4 did it"}
    );
    band_add(
        bands, &band_count,
        (Band){"out-of-bounds rate %", percent(T_fouls, T_contacts), 2, 30, "12",
               "direction drifting foul, or every ball dumped safely into the field"}
    );
    band_add(
        bands, &band_count,
        (Band){"meter error, style-1 swings (steps)", T_s1_swings ? (double)T_s1_err / T_s1_swings : 0.0, -1.0, 3.0,
               "+0.77", "the AI no longer releasing at the power level it decided on"}
    );
    band_add(
        bands, &band_count,
        (Band){"direction fan: smallest bucket %", percent(dir_min, dir_total), 4, 100, "13",
               "the fan collapsing — the centre-bias the swing slice removes structurally"}
    );
    band_add(
        bands, &band_count,
        (Band){"direction fan: largest bucket %", percent(dir_max, dir_total), 0, 55, "27",
               "the fan piling into one direction"}
    );

    // ---- the fielding bands ----------------------------------------------------------
    // The ten bands above are every one of them batting- or pitching-side, so until these
    // arrived the whole catching half of the game could regress without a single number
    // moving. They were baselined 2026-08-27 on UNCHANGED code, before the movement slice
    // touched anything — recording them first is what lets the slice argue from them
    // afterwards, since the slice re-baselines the determinism hash by design and cannot
    // lean on it.
    band_add(
        bands, &band_count,
        (Band){"mean frames to recover a batted ball",
               T_recoveries ? (double)T_recovery_frames / (double)T_recoveries : 0.0, 60, 150, "102.4",
               "the defence getting slower end to end — a fielder that sets off late, or not at all"}
    );
    band_add(
        bands, &band_count,
        (Band){"worst frames to recover", (double)T_recovery_max, 0, 2000, "800",
               "one chase that never converges, hidden inside a healthy mean"}
    );
    band_add(
        bands, &band_count,
        (Band){"chases that ended in possession %", percent(T_recoveries, T_recoveries + T_abandoned), 70, 100, "87.6",
               "balls the catching side simply never brings back"}
    );
    band_add(
        bands, &band_count,
        (Band){"mean chase distance", T_chase_samples ? T_chase_dist / (double)T_chase_samples : 0.0, 10, 22, "15.12",
               "the controlled fielder not going where the engine sent it"}
    );
    // The floor here is the load-bearing one: RUN_SPEED is 0.12 and WALK_SPEED is 0.06, so a
    // controlled fielder quietly moved onto the auto-fielders' walk — exactly what reusing
    // move_to_target would have done — cannot stay inside this band.
    band_add(
        bands, &band_count,
        (Band){"mean step per moving frame", T_step_frames ? T_step_sum / (double)T_step_frames : 0.0, 0.09, 0.125,
               "0.1073", "the controlled fielder silently switched to a different speed"}
    );

    int breached = 0;
    printf("\n  AI-quality bands:\n");
    for (int i = 0; i < band_count; i++) {
        const Band* b = &bands[i];
        int ok = b->measured >= b->lo && b->measured <= b->hi;
        if (!ok) breached++;
        printf(
            "    %-36s %8.2f  in [%g, %g]  baseline %-6s %s\n", b->name, b->measured, b->lo, b->hi, b->baseline,
            ok ? "ok" : "OUT OF BAND"
        );
        if (!ok) printf("        this band guards against: %s\n", b->guards);
    }

    if (breached != 0) {
        printf(
            "\n  %d AI-quality band(s) breached. These are not tuning targets: a breach means the AI\n"
            "  plays measurably worse than it did, or the band was wrong. Decide which — and if the\n"
            "  change is intended, move the band and its baseline in the same commit.\n",
            breached
        );
        return TEST_FAILED;
    }
    return TEST_PASSED;
}
