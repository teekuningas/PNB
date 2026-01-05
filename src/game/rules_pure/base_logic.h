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
 * Equivalent to: id >= BASE_HOME && id <= BASE_THIRD
 */
bool base_is_index(BaseID id);

/**
 * Checks if a base allows advancement (running forward).
 * True for HOME, FIRST, SECOND.
 * False for THIRD (can't advance to 4th base, you score instead), HOME_SCORED, NONE.
 * Replaces logic like: base >= 0 && base < 3
 */
bool base_can_advance(BaseID id);

/**
 * Checks if id is numerically greater than or equal to threshold.
 * Relying on the enum order: HOME(0) < FIRST(1) < SECOND(2) < THIRD(3) < HOME_SCORED(4)
 */
bool base_is_at_least(BaseID id, BaseID threshold);

/**
 * Compares two bases.
 * Returns positive if a > b, negative if a < b, 0 if equal.
 * Useful for finding the "most advanced" runner.
 */
int base_cmp(BaseID a, BaseID b);

/**
 * Safely converts BaseID to an integer index.
 * Returns -1 if not a valid index (HOME..THIRD).
 */
int base_to_int_index(BaseID id);

/**
 * Checks if a player is in a protected state (safe on base, at bat, or walking freely).
 * This represents general immunity from being forced out while occupying a base.
 */
bool player_is_protected(PlayerUnitState state);

/**
 * Checks if a player is safe from being wounded by a fly ball catch (koppi).
 * In Pesäpallo, a runner is safe if they have not left their original base,
 * or if they are taking a free walk.
 */
bool player_is_safe_from_fly(PlayerUnitState state, BaseID current_base, BaseID original_base);

#endif