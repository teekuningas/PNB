#ifndef RNG_H
#define RNG_H

// Deterministic random number generator
// Returns a random integer in range [0, max-1]
// Pass the same seed to get the same sequence
int seeded_rand(unsigned int* seed, int max);

#endif /* RNG_H */
