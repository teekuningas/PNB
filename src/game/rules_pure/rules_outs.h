#ifndef RULES_OUTS_H
#define RULES_OUTS_H

#include "globals.h"

/**
 * @brief Checks if a runner is forced out according to §33 Pesäkilpa.
 *
 * @param player_base The current base index of the player (the base they are running FROM, i.e., baseIndex = i-1).
 * @param player_is_on_base_flag Flag indicating if the player is currently on a base (1 for true, 0 for false).
 * @param ball_at_base_index The ID of the base where the ball is currently located/caught.
 * @param taking_free_walk Flag indicating if the player is taking a free walk (vapaataival).
 * @param out_of_bounds_active Flag indicating if a foul/out-of-bounds state is active.
 * @return 1 if the runner is forced out, 0 otherwise.
 */
int is_runner_forced_out(
    BaseID player_base, int player_is_on_base_flag, BaseID ball_at_base_index, int taking_free_walk,
    int out_of_bounds_active
);

#endif // RULES_OUTS_H
