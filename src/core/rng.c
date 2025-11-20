#include "rng.h"
#include <stddef.h>

int seeded_rand(unsigned int* seed, int max)
{
	if (seed == NULL || max <= 0) return 0;

	// Linear congruential generator (LCG)
	// Same constants as glibc's rand() for consistency
	*seed = (*seed * 1103515245 + 12345) & 0x7fffffff;

	// Return value in range [0, max-1]
	return (*seed >> 16) % max;
}
