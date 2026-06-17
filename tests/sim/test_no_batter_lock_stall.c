#include "test_helpers.h"
#include "sim_harness.h"
#include "sim_observers.h"

// Regression knight for the orphaned-AI-batting-lock deadlock (KNOWN_BUGS).
//
// When an at-bat ended (e.g. a 3rd-strike forced run) while the AI batting team still held
// AI_BATTING_LOCK from its in-progress swing, the lock was never released: the swing's release
// path lives inside the batter_ready block, which no longer runs once the at-bat is over. The
// next batter decision requires AI_NO_LOCK, so the whole AI-vs-AI game deadlocked.
//
// Mislabelled the "inning-rollover stall" in older notes — it is not a rollover/reset bug, it
// surfaces mid-half-inning. These three seeds reproduced it (within the first period); they must
// now play three full half-innings without the invariant observer tripping a no-progress stall.

#define PERIOD_MAX_FRAMES 120000L
#define STALL_LIMIT 20000L

static int three_half_innings_done(const SimGame* g)
{
    return (g->state->rules->scoreboard.inning - g->start_inning) >= 3 || g->state->screen != SCREEN_GAME;
}

int test_no_batter_lock_stall(void)
{
    const unsigned int seeds[] = {0xF4A133D7u, 0x0BB6944Cu, 0xC1036E72u};

    for (unsigned int s = 0; s < sizeof(seeds) / sizeof(seeds[0]); s++) {
        GameSetup setup;
        sim_make_normal_setup(&setup, 0, 1, CONTROL_AI, CONTROL_AI);
        SimGame* g = sim_create(&setup, seeds[s]);
        ASSERT_NOT_NULL(g, "sim_create returned NULL");

        InvariantObserver inv;
        invariant_observer_init(&inv, STALL_LIMIT);
        sim_attach(g, invariant_observer_hook, &inv);

        sim_run_until(g, three_half_innings_done, PERIOD_MAX_FRAMES);

        int stalled = g->failed;
        char reason[SIM_FAIL_REASON_LEN];
        snprintf(reason, sizeof(reason), "%s", g->fail_reason);
        sim_destroy(g);

        if (stalled) {
            printf("  seed 0x%08X stalled: %s\n", seeds[s], reason);
            return TEST_FAILED;
        }
    }
    return TEST_PASSED;
}
