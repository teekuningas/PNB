#include "rules_outs.h"
#include "foundation.h"

/**
 * @brief Checks if a runner is forced out according to §33 Pesäkilpa and §36 Koppilyönnillä eteneminen.
 *
 * A runner is burned (forced out) if the ball reaches the target base before the runner,
 * provided the runner is not safe on the base, is not taking a free walk, and the play is valid.
 *
 * In the case of a fly ball catch (§36), runners who were off-base (irti) lose their safety
 * and must reach the next base before the ball to avoid being out.
 */
int is_runner_forced_out(
    BaseID player_base, int player_is_on_base_flag, BaseID threatened_from_base, int taking_free_walk,
    int out_of_bounds_active
)
{
    // If the play is out of bounds (foul), outs are generally not recorded this way
    // (or the situation is handled by foulPlay logic reset).
    if (out_of_bounds_active) {
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

    // §33 base-race burn: ball held at base i threatens off-base runners advancing from base i-1
    // (passed in as threatened_from_base).
    if (player_base == threatened_from_base) {
        return 1;
    }

    return 0;
}