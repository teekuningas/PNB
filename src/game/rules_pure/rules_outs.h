#ifndef RULES_OUTS_H
#define RULES_OUTS_H

/**
 * @brief Checks if a runner is forced out according to §33 Pesäkilpa.
 *
 * @param player_base The current base index of the player (the base they are running FROM, i.e., baseIndex = i-1).
 * @param player_is_on_base_flag Flag indicating if the player is currently on a base (1 for true, 0 for false).
 * @param ball_at_base_index The index of the base where the ball is currently located/caught.
 * @param taking_free_walk Flag indicating if the player is taking a free walk (vapaataival).
 * @param is_out_of_bounds Flag indicating if the player is out of bounds.
 * @return 1 if the runner is forced out, 0 otherwise.
 */
int is_runner_forced_out(int player_base, int player_is_on_base_flag, int ball_at_base_index, int taking_free_walk, int is_out_of_bounds);

#endif // RULES_OUTS_H
