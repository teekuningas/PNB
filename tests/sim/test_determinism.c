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

// The necessary complement to the test above. "Same seed → same game" is satisfied just as well
// by a game with no randomness left in it at all: if a seed were zeroed on every reset, or a
// stream were never actually advanced, the determinism test would still pass while the game
// quietly played the identical inning forever.
//
// This matters directly for the engine/AI stream split: both streams are now seeded once at
// match start and owned by different structs, so a reset recipe clearing the wrong field is a
// real and silent hazard. Requiring different seeds to produce different games pins it.
int test_different_seeds_produce_different_games(void)
{
    unsigned long long a = run_once(0x0000AAAAu);
    unsigned long long b = run_once(0xBBBB0000u);
    ASSERT(a != b, "two different seeds produced an identical game — randomness is not reaching the sim");
    return TEST_PASSED;
}
