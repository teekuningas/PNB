#include "rules_runs.h"

int calculate_runs(int player_base, int player_original_base, int player_is_wounded, int can_make_run_of_honor, int has_made_run_on_third_base)
{
	if (player_is_wounded) {
		return 0;
	}

	// Normal run: safe at home base (base 4)
	if (player_base == 4) {
		return 1;
	}

	// Run of Honor: safe at 3rd base, started as batter, allowed, and not already counted
	if (player_base == 3 && player_original_base == 0 && can_make_run_of_honor && !has_made_run_on_third_base) {
		return 1;
	}

	return 0;
}
