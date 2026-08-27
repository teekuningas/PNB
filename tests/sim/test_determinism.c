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

// The recorded behaviour baseline for the AI-vs-AI half-inning at seed 0x1234ABCD. This is a
// FINGERPRINT OF BEHAVIOUR, not of the struct layout: the checksum observer folds curated
// game-meaningful fields (positions, base ids, player states, ball, the engine seed, the legal
// counters), so adding or removing an unrelated field cannot move it — only the game playing out
// differently can.
//
// What it is for: a slice that claims to be behaviour-preserving can say so as a fact instead of a
// hope. A moved hash is then a leak to investigate, never something to re-baseline over. When a
// slice changes timing or the RNG on purpose, this constant is re-recorded DELIBERATELY, in that
// slice's own commit, alongside the argument for why the new behaviour is the intended one.
// Re-recorded 2026-08-27 by the fielder-movement slice, deliberately. The catching AI stopped
// puppeteering arrow keys and now declares a destination, so every fielder path in the game is
// walked by the engine instead of quantised to eight directions and re-aimed every tenth frame.
// Timing moves everywhere as a result; the argument that the new behaviour is the intended one is
// the fielding bands in test_ai_offense_breakdown, which were recorded on the OLD behaviour before
// any of this landed and all fifteen of which hold — with the defence measurably better
// (mean recovery 102.4 -> 101.1 frames, worst case 800 -> 558).
#define SIM_BEHAVIOUR_BASELINE_HASH 0x1411c0a41f2064f8ULL

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

// Determinism (above) proves the machine repeats ITSELF; this proves the machine still plays the
// SAME GAME it played before. The two are independent: a refactor that quietly changes when a
// fielder starts moving keeps every determinism property intact and still fails here.
//
// If this goes red, the question is never "what is the new number" — it is "what did I change about
// the game". The tests/sim box-score and offense-breakdown tests are the tools for answering it.
int test_sim_hash_matches_recorded_baseline(void)
{
    unsigned long long h = run_once(0x1234ABCDu);
    ASSERT(
        h == SIM_BEHAVIOUR_BASELINE_HASH,
        "the AI-vs-AI half-inning no longer plays out the same game (see the baseline constant above)"
    );
    return TEST_PASSED;
}
