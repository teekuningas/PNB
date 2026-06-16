#include "test_helpers.h"
#include "sim_harness.h"
#include "sim_observers.h"
#include <stdlib.h>

// Diagnostic: characterise AI offense across many seeds × a full first period, to answer
// "why 0 runs?" — is a runner that reaches 3rd stranded (never runs), thrown out heading home,
// wounded, or does the team simply never get there? This is a measurement test: it prints the
// breakdown and only asserts weak floors (the AI bats, and the run terminates), so it never
// goes red on the very improvement we are chasing. Set SIM_PBP=1 to also see the play-by-play.

#define SEED_COUNT 24
#define PERIOD_MAX_FRAMES 120000L
#define STALL_LIMIT 20000L

// Stay strictly inside the first period: it ends at inning == halfInningsInPeriod (4), and the
// period boundary hands off to a menu the single-phase sim harness does not yet autopilot
// (a documented deferred gap). Three half-innings of offense per seed, no boundary crossing.
static int three_half_innings_done(const SimGame* g)
{
    return (g->state->rules->scoreboard.inning - g->start_inning) >= 3 || g->state->screen != SCREEN_GAME;
}

int test_ai_offense_breakdown(void)
{
    long T_pitches = 0, T_contacts = 0, T_whiffs = 0, T_strikes = 0, T_balls = 0, T_outs = 0;
    long T_reached_base = 0, T_reached_third = 0, T_ran_third = 0, T_scored_third = 0;
    long T_out_third = 0, T_wound_third = 0, T_runs = 0;
    long T_s1_swings = 0, T_s1_err = 0;
    int seeds_reached_third = 0, seeds_ran_third = 0, seeds_scored = 0, seeds_incomplete = 0, seeds_stalled = 0;

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

        if (box.reached_third > 0) seeds_reached_third++;
        if (box.ran_from_third > 0) seeds_ran_third++;
        if (box.runs_scored > 0) seeds_scored++;

        sim_destroy(g);
    }

    printf(
        "\n  AI offense over %d seeds × 3 half-innings (incomplete=%d, stalled=%d):\n", SEED_COUNT, seeds_incomplete,
        seeds_stalled
    );
    printf(
        "    pitches=%ld contacts=%ld (%.0f%%) whiffs=%ld | strikes=%ld balls=%ld outs=%ld runs=%ld\n", T_pitches,
        T_contacts, T_pitches ? 100.0 * T_contacts / T_pitches : 0.0, T_whiffs, T_strikes, T_balls, T_outs, T_runs
    );
    printf(
        "    bases: reachedFirst=%ld reachedThird=%ld (in %d seeds)\n", T_reached_base, T_reached_third,
        seeds_reached_third
    );
    printf(
        "    third→home: ranForHome=%ld (in %d seeds) → scored=%ld, OUT=%ld, wounded=%ld\n", T_ran_third,
        seeds_ran_third, T_scored_third, T_out_third, T_wound_third
    );
    // Style-1 power only (the one style with a real intent). ≈ +1 means correct meter use.
    // NOT a hitting-quality metric and NOT direction — see AI_NOTES.md §2 for the proper redo.
    printf(
        "    meter (style-1 swings only): n=%ld meanPowerErr=%+.2f steps (expect ~+1; direction not measured)\n",
        T_s1_swings, T_s1_swings ? (double)T_s1_err / T_s1_swings : 0.0
    );

    // Weak floors only — measurement, not a behavioural lock.
    if (T_contacts == 0) {
        printf("  AI never made contact across all seeds (batting broken)\n");
        return TEST_FAILED;
    }
    return TEST_PASSED;
}
