#include "rules_side_change.h"

int designate_last_batter(int last_regular_batter_index, int current_designation)
{
    if (last_regular_batter_index == -1) return current_designation;
    return last_regular_batter_index;
}

int is_last_batter_turn_reached(
    int designated_index, int has_batted_again, int runs_in_the_inning, int next_in_order_index
)
{
    if (designated_index == -1) return 0;

    if (runs_in_the_inning < 2) return next_in_order_index == designated_index; // §12(2)
    return has_batted_again; // §12(3)
}
