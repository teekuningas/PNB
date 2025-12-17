#include "rules_outs.h"

/**
 * @brief Checks if a runner is forced out according to §33 Pesäkilpa.
 *
 * A runner is burned (forced out) if the ball reaches the target base before the runner,
 * provided the runner is not safe on the base, is not taking a free walk, and the play is valid.
 */
int is_runner_forced_out(int player_base, int player_is_on_base_flag, int ball_at_target_base_index, int taking_free_walk, int is_out_of_bounds)
{
    // If the play is out of bounds (foul), outs are generally not recorded this way
    // (or the situation is handled by foulPlay logic reset).
    if (is_out_of_bounds) {
        return 0;
    }

    // If the runner is currently safe on a base, they cannot be forced out.
    if (player_is_on_base_flag) {
        return 0;
    }

    // If the runner is taking a free walk (vapaataival), they are protected from being forced out.
    if (taking_free_walk) {
        return 0;
    }

    // Force out condition:
    // The runner is advancing to a base (player_base) and the ball is controlled at that base
    // (ball_at_target_base_index). Since the runner is not yet on the base (checked above),
    // they lose the "base race" (pesäkilpa).
    if (player_base == ball_at_target_base_index) {
        return 1;
    }

    return 0;
}