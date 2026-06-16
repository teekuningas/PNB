#include "test_helpers.h"
#include "sim_harness.h"
#include "sim_observers.h"
#include <stdlib.h>

// Generous budgets; tuned from measured runs. A half-inning is many pitches until
// 3 outs (or a scoreless cap). If the AI ever freezes, the half-inning never ends
// and we hit the budget — which is itself the stall guard.
#define HALF_INNING_MAX_FRAMES 60000L
#define STALL_LIMIT 20000L

int test_ai_vs_ai_half_inning(void)
{
    GameSetup setup;
    sim_make_normal_setup(&setup, 0, 1, CONTROL_AI, CONTROL_AI);

    SimGame* g = sim_create(&setup, 0xC0FFEEu);
    ASSERT_NOT_NULL(g, "sim_create returned NULL");

    InvariantObserver inv;
    invariant_observer_init(&inv, STALL_LIMIT);
    sim_attach(g, invariant_observer_hook, &inv);

    FILE* tf = NULL;
    TraceObserver tr;
    if (getenv("SIM_TRACE")) {
        tf = fopen("sim_trace_half_inning.csv", "w");
        trace_observer_init(&tr, tf, 1);
        sim_attach(g, trace_observer_hook, &tr);
    }

    BoxScoreObserver box;
    box_score_observer_init(&box, getenv("SIM_PBP") ? stdout : NULL);
    sim_attach(g, box_score_observer_hook, &box);

    long frames = sim_run_until(g, sim_pred_half_inning_ended, HALF_INNING_MAX_FRAMES);

    printf(
        "\n  [half-inning] frames=%ld pitches=%ld count_changes=%ld failed=%d reason='%s'\n", frames, inv.pitches,
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
        printf("  half-inning did not end within %ld frames\n", HALF_INNING_MAX_FRAMES);
        ok = 0;
    }
    if (ok && inv.pitches == 0) {
        printf("  game never released a pitch (AI inert)\n");
        ok = 0;
    }

    // Golden-baseline regression net (robust to randomness — floors, not exact values).
    // Today's AI: ~100% contact, but every contact fouls off and the half-inning is all
    // strikeouts (runs=0, nobody reaches base). We assert the floors that would break if
    // batting or pitching regressed; we deliberately do NOT pin runs/baserunners, so the
    // intended future improvement (the AI finally scoring) shows up as a louder box score,
    // not a red test.
    if (ok && box.contacts == 0) {
        printf("  AI never made contact (batting regressed)\n");
        ok = 0;
    }
    if (ok && box.contacts * 2 < box.pitches) {
        printf("  contact rate fell below 50%% (%ld/%ld) — batting regressed\n", box.contacts, box.pitches);
        ok = 0;
    }

    if (tf) fclose(tf);
    sim_destroy(g);
    return ok ? TEST_PASSED : TEST_FAILED;
}
