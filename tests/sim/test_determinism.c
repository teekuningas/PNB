#include "test_helpers.h"
#include "sim_harness.h"
#include "sim_observers.h"

// Run one AI-vs-AI half-inning at a fixed seed and return the rolling MatchSession
// fingerprint. Determinism means two such runs are byte-identical every frame.
static unsigned long long run_once(unsigned int seed)
{
    GameSetup setup;
    sim_make_normal_setup(&setup, 0, 1, CONTROL_AI, CONTROL_AI);

    SimGame* g = sim_create(&setup, seed);

    ChecksumObserver cs;
    checksum_observer_init(&cs);
    sim_attach(g, checksum_observer_hook, &cs);

    sim_run_until(g, sim_pred_half_inning_ended, 60000L);

    unsigned long long h = cs.hash;
    long frames = g->frame;
    sim_destroy(g);

    // Stash frame count alongside hash via the printf for diagnostics.
    printf("    seed=%u frames=%ld hash=%016llx\n", seed, frames, h);
    return h;
}

int test_ai_vs_ai_determinism(void)
{
    unsigned long long a = run_once(0x1234ABCDu);
    unsigned long long b = run_once(0x1234ABCDu);
    ASSERT(a == b, "same-seed AI-vs-AI runs diverged (nondeterminism)");
    return TEST_PASSED;
}
