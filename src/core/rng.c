#include "rng.h"
#include <stddef.h>

// One step of the linear congruential generator.
// Same constants as glibc's rand() for consistency.
static void lcg_step(unsigned int* seed)
{
    *seed = (*seed * 1103515245 + 12345) & 0x7fffffff;
}

int seeded_rand(unsigned int* seed, int max)
{
    if (seed == NULL || max <= 0) return 0;

    lcg_step(seed);

    // Return value in range [0, max-1]
    return (*seed >> 16) % max;
}

unsigned int rng_split(unsigned int* seed)
{
    if (seed == NULL) return 0;

    // Advance the parent so successive splits hand out different children.
    lcg_step(seed);

    // Handing the child the parent's raw state would not actually split anything: two LCG
    // states one step apart generate the SAME sequence, merely offset by one draw. So the
    // state is bit-mixed (an xor-shift/multiply finalizer) before it becomes a child seed,
    // which decorrelates parent from child and each child from its siblings.
    unsigned int s = *seed;
    s ^= s >> 15;
    s *= 2246822519u;
    s ^= s >> 13;
    s *= 3266489917u;
    s ^= s >> 16;

    // Keep children inside the LCG's 31-bit state space.
    return s & 0x7fffffff;
}
