#include "test_helpers.h"
#include "rng.h"

// rng_split derives an independent child stream from a parent. The hazard it exists to avoid is
// specific: two LCG states one step apart generate the SAME sequence, merely offset by one draw.
// A "split" that just handed over the parent's raw state would therefore split nothing at all —
// the engine and the AI controller would still be walking one shared sequence, which is exactly
// the entanglement the split is meant to remove.
int test_rng_split_parent_and_child_do_not_share_a_sequence(void)
{
    unsigned int parent = 0x1234ABCDu;
    unsigned int child = rng_split(&parent);

    // Slide the parent forward and check the child never reproduces a run of its draws.
    for (int offset = 0; offset < 64; offset++) {
        unsigned int p = parent;
        for (int skip = 0; skip < offset; skip++) {
            seeded_rand(&p, 1000);
        }

        unsigned int c = child;
        int identical = 1;
        for (int i = 0; i < 8; i++) {
            if (seeded_rand(&p, 1000) != seeded_rand(&c, 1000)) {
                identical = 0;
                break;
            }
        }
        ASSERT(!identical, "child stream reproduces the parent's sequence at some offset");
    }

    return TEST_PASSED;
}

// Two children split from one master must also be independent of each other — this is the actual
// production shape (initialize_game_from_menu splits the engine seed and the AI seed from the same
// app seed). Identical or offset streams here would correlate engine draws with AI draws.
int test_rng_split_siblings_are_independent(void)
{
    unsigned int master = 42u;
    unsigned int a = rng_split(&master);
    unsigned int b = rng_split(&master);

    ASSERT(a != b, "two children split from one master must not be the same seed");

    // Over many draws in [0,1000) two independent streams collide about 1 time in 1000. Identical
    // or one-step-offset streams would collide on essentially every draw.
    int collisions = 0;
    const int draws = 5000;
    for (int i = 0; i < draws; i++) {
        if (seeded_rand(&a, 1000) == seeded_rand(&b, 1000)) {
            collisions++;
        }
    }
    ASSERT(collisions < draws / 20, "sibling streams collide far more than chance — they are correlated");

    return TEST_PASSED;
}

// A split stream must still be a usable generator, not just a distinct one.
int test_rng_split_child_stream_is_uniform(void)
{
    unsigned int master = 0xBEEFu;
    unsigned int child = rng_split(&master);

    const int buckets = 10;
    const int draws = 100000;
    int counts[10] = {0};
    for (int i = 0; i < draws; i++) {
        counts[seeded_rand(&child, buckets)]++;
    }

    // Expect draws/buckets each; allow a generous 10% band so the test is not flaky by design.
    const int expected = draws / buckets;
    for (int i = 0; i < buckets; i++) {
        ASSERT(counts[i] > expected - expected / 10, "split stream bucket badly under-filled");
        ASSERT(counts[i] < expected + expected / 10, "split stream bucket badly over-filled");
    }

    return TEST_PASSED;
}
