#include "test_helpers.h"
#include "sim_harness.h"
#include "sim_observers.h"
#include "fixtures.h" // src/include/fixtures.h — GameSetup factories
#include <stdlib.h>

#define HOMERUN_MAX_FRAMES 60000L
#define STALL_LIMIT 20000L

// The contest turn is done when this half-inning rolls over, or when the contest
// concludes the game and the pipeline leaves SCREEN_GAME.
static int homerun_turn_done(const SimGame* g)
{
    return g->state->rules->scoreboard.inning != g->start_inning || g->state->screen != SCREEN_GAME;
}

int test_ai_vs_ai_homerun(void)
{
    GameSetup setup;
    fixture_create_homerun_contest(&setup, 0, 1, CONTROL_AI, CONTROL_AI);

    SimGame* g = sim_create(&setup, 0xBA5EBA11u);
    ASSERT_NOT_NULL(g, "sim_create returned NULL");

    InvariantObserver inv;
    invariant_observer_init(&inv, STALL_LIMIT);
    sim_attach(g, invariant_observer_hook, &inv);

    FILE* tf = NULL;
    TraceObserver tr;
    if (getenv("SIM_TRACE")) {
        tf = fopen("sim_trace_homerun.csv", "w");
        trace_observer_init(&tr, tf, 1);
        sim_attach(g, trace_observer_hook, &tr);
    }

    BoxScoreObserver box;
    box_score_observer_init(&box, getenv("SIM_PBP") ? stdout : NULL);
    sim_attach(g, box_score_observer_hook, &box);

    long frames = sim_run_until(g, homerun_turn_done, HOMERUN_MAX_FRAMES);

    printf(
        "\n  [homerun] frames=%ld pitches=%ld count_changes=%ld failed=%d reason='%s'\n", frames, inv.pitches,
        inv.count_changes, g->failed, g->fail_reason
    );
    printf(
        "  box score: pitches=%ld contacts=%ld whiffs=%ld strikes=%ld balls=%ld outs=%ld reachedBase=%ld "
        "furthestBase=%d runs=%ld\n",
        box.pitches, box.contacts, box.whiffs, box.strikes_called, box.balls_called, box.outs_made, box.reached_base,
        box.furthest_base, box.runs_scored
    );

    int ok = 1;
    if (g->failed) {
        printf("  invariant/stall failure: %s\n", g->fail_reason);
        ok = 0;
    }
    if (frames < 0) {
        printf("  homerun turn did not complete within %ld frames\n", HOMERUN_MAX_FRAMES);
        ok = 0;
    }

    if (tf) fclose(tf);
    sim_destroy(g);
    return ok ? TEST_PASSED : TEST_FAILED;
}

// ---------------------------------------------------------------------------
// The contest across many seeds — the net that would have caught bug #8.
//
// The single run above proves the contest plays. It cannot prove the contest is *sound*: bug #8
// (a fourth pitch to a batter who had spent his three correct pitches) broke roughly one contest
// turn in twenty, and this tier ran exactly one seed, so it hid for as long as the contest has
// existed. A rate that low is invisible to any single-seed test and obvious to a sweep — which is
// the general lesson, not a fact about this bug.
//
// What each seed asserts is the invariant observer's whole bounds set, most of it contest-relevant:
// strikes in [0,3] (§18(1) — three correct pitches end the batting turn, so a fourth pitch cannot
// be counted), the pair counter in [0, pairCount], no stall, and the turn actually completing.
//
// Measured 2026-08-27 on the fix that closed bug #8: 0 breaches in 400 seeds, where the code before
// it broke 21. SEED_COUNT is the runtime/confidence trade: at bug #8's ~5% rate, 120 seeds miss it
// about twice in a thousand runs.
//
// WHAT THIS CANNOT SEE: the same blindness the single run has. The AI batter is weak by design until
// the swing slice, so most turns score nothing, and no band here speaks about the *quality* of the
// contest — only that it stays inside the rules and finishes.
#define HOMERUN_SEED_COUNT 120

int test_homerun_contest_seed_sweep(void)
{
    int failures = 0, incomplete = 0;
    long total_frames = 0;

    for (int i = 0; i < HOMERUN_SEED_COUNT; i++) {
        // Golden-ratio stride from a fixed base: a spread, reproducible seed set.
        unsigned int seed = 0x1000u + (unsigned int)i * 2654435761u;

        GameSetup setup;
        fixture_create_homerun_contest(&setup, 0, 1, CONTROL_AI, CONTROL_AI);
        SimGame* g = sim_create(&setup, seed);
        ASSERT_NOT_NULL(g, "sim_create returned NULL");

        InvariantObserver inv;
        invariant_observer_init(&inv, STALL_LIMIT);
        sim_attach(g, invariant_observer_hook, &inv);

        long frames = sim_run_until(g, homerun_turn_done, HOMERUN_MAX_FRAMES);

        if (g->failed) {
            printf("  seed 0x%08X failed at frame %ld: %s\n", seed, g->frame, g->fail_reason);
            failures++;
        } else if (frames < 0) {
            printf("  seed 0x%08X did not finish its turn within %ld frames\n", seed, HOMERUN_MAX_FRAMES);
            incomplete++;
        } else {
            total_frames += frames;
        }

        sim_destroy(g);
    }

    int ok = HOMERUN_SEED_COUNT - failures - incomplete;
    printf(
        "\n  [homerun sweep] seeds=%d ok=%d invariant-failures=%d unfinished=%d mean frames/turn=%ld\n",
        HOMERUN_SEED_COUNT, ok, failures, incomplete, ok > 0 ? total_frames / ok : 0
    );

    return (failures == 0 && incomplete == 0) ? TEST_PASSED : TEST_FAILED;
}
