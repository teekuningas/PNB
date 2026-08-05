#ifndef RNG_H
#define RNG_H

// Deterministic random number generator
// Returns a random integer in range [0, max-1]
// Pass the same seed to get the same sequence
int seeded_rand(unsigned int* seed, int max);

// Derive an independent child seed from a parent stream, advancing the parent.
// Used to give the engine and each controller their own stream from one master seed:
// the streams stay reproducible from that master, but never share draws.
unsigned int rng_split(unsigned int* seed);

#endif /* RNG_H */
