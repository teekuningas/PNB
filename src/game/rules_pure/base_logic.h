#ifndef BASE_LOGIC_H
#define BASE_LOGIC_H

#include "globals.h"
#include <stdbool.h>

/**
 * Returns the next logical base in the sequence.
 * BASE_HOME -> BASE_FIRST
 * BASE_FIRST -> BASE_SECOND
 * BASE_SECOND -> BASE_THIRD
 * BASE_THIRD -> BASE_HOME_SCORED
 * BASE_HOME_SCORED -> BASE_NONE
 * BASE_NONE -> BASE_NONE
 */
BaseID base_get_next(BaseID id);

/**
 * Returns the previous logical base in the sequence.
 * BASE_FIRST -> BASE_HOME
 * BASE_SECOND -> BASE_FIRST
 * BASE_THIRD -> BASE_SECOND
 * BASE_HOME_SCORED -> BASE_THIRD
 * BASE_HOME -> BASE_NONE
 * BASE_NONE -> BASE_NONE
 */
BaseID base_get_prev(BaseID id);

/**
 * Checks if the ID represents a physical base on the field where a player can be safe.
 * Includes HOME, FIRST, SECOND, THIRD.
 * Excludes HOME_SCORED, NONE.
 */
bool base_is_safe_haven(BaseID id);

/**
 * Checks if the ID is a valid base index (0-3).
 * Useful for array indexing (like baseRun[4]).
 */
bool base_is_index(BaseID id);

#endif
