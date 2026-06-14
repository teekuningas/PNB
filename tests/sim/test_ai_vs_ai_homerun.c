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

    long frames = sim_run_until(g, homerun_turn_done, HOMERUN_MAX_FRAMES);

    printf(
        "\n  [homerun] frames=%ld pitches=%ld count_changes=%ld failed=%d reason='%s'\n", frames, inv.pitches,
        inv.count_changes, g->failed, g->fail_reason
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
