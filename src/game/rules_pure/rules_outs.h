#ifndef RULES_OUTS_H
#define RULES_OUTS_H

#include "foundation.h"

/**
 * @brief Checks if a runner is burnt in a base race according to §33 Pesäkilpa.
 *
 * @param player_base The base the runner is advancing FROM (their current base_id).
 * @param player_is_on_base_flag 1 if the runner currently holds safety on their base, 0 if off-base (irti).
 * @param threatened_from_base The FROM base threatened by the ball's location (e.g., ball at 1st threatens BASE_HOME,
 *        ball at home threatens BASE_THIRD).
 * @param taking_free_walk 1 if the runner is advancing on a free walk (vapaataival), 0 otherwise.
 * @param out_of_bounds_active 1 if a foul/out-of-bounds state is active, 0 otherwise.
 * @return 1 if the runner is burnt, 0 otherwise.
 */
int is_runner_forced_out(
    BaseID player_base, int player_is_on_base_flag, BaseID threatened_from_base, int taking_free_walk,
    int out_of_bounds_active
);

#endif // RULES_OUTS_H
