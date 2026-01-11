#include "rules_runs.h"

int calculate_runs(BaseID player_base, BaseID player_original_base, int player_is_wounded, const GameModeState* gameModeState, int has_made_run_on_third_base)
{
	int can_make_run_of_honor = gameModeState->canMakeRunOfHonor;

	// §41 Juoksu (Run): safe at home base (BASE_HOME_SCORED)
	if (player_base == BASE_HOME_SCORED &&player_is_wounded == 0) {
		return 1;
	}

	// §42 Kunniajuoksu (Run of Honor): safe at 3rd base, started as batter, allowed, and not already counted
	if (player_base == BASE_THIRD &&player_original_base == BASE_HOME &&can_make_run_of_honor &&!has_made_run_on_third_base &&player_is_wounded == 0) {
		return 1;
	}

	return 0;
}
