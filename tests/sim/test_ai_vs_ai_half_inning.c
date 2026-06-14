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

    long frames = sim_run_until(g, sim_pred_half_inning_ended, HALF_INNING_MAX_FRAMES);

    printf(
        "\n  [half-inning] frames=%ld pitches=%ld count_changes=%ld failed=%d reason='%s'\n", frames, inv.pitches,
        inv.count_changes, g->failed, g->fail_reason
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

    if (tf) fclose(tf);
    sim_destroy(g);
    return ok ? TEST_PASSED : TEST_FAILED;
}
