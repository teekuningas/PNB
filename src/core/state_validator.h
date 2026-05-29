#ifndef STATE_VALIDATOR_H
#define STATE_VALIDATOR_H

#include "globals.h"

// Initialize the validator with an output file path (or NULL to disable dumping)
void state_validator_init(const char* jsonPath);

// Run validation checks. Returns 1 if valid, 0 if invalid.
// Does NOT dump or pause automatically anymore.
int state_validator_check(StateInfo* state);

// Explicitly set whether validation is active (e.g. via command line arg)
void state_validator_set_active(int active);

// Capture a snapshot of the current state for the debug history log
// Label describes the event (e.g. "PITCH_START")
void state_validator_capture_snapshot(StateInfo* state, const char* label);

// Force a debug dump of the current state and history buffer
void state_validator_dump(StateInfo* state, const char* reason);

#endif