#ifndef STATE_VALIDATOR_H
#define STATE_VALIDATOR_H

#include "globals.h"

// Initialize the validator with an output file path (or NULL to disable dumping)
void StateValidator_Init(const char* jsonPath);

// Run validation checks. If check fails, dump state and pause game.
void StateValidator_Check(StateInfo* state);

// Explicitly set whether validation is active (e.g. via command line arg)
void StateValidator_SetActive(int active);

#endif
