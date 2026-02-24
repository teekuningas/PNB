#include "rules_runs.h"

int is_regular_run(BaseID player_base, BaseID player_original_base, int player_is_wounded)
{
    // §41 Juoksu (Run): safe at home base (BASE_HOME_SCORED)
    if (player_base == BASE_HOME_SCORED && player_is_wounded == 0) {
        return 1;
    }
    return 0;
}

int is_run_of_honor(
    BaseID player_base, BaseID player_original_base, int player_is_wounded, int has_made_run_on_third_base
)
{
    // §42 Kunniajuoksu (Run of Honor): Batter arrives at 3rd base with their own hit
    // Conditions:
    // 1. Arriving at 3rd base
    // 2. Started at home (was the batter)
    // 3. Not wounded
    // 4. Haven't already scored run of honor
    // Note: No need to check "going forward" - if same pitch, runOfHonorScored prevents double scoring.
    //       If new pitch, baseAtPitchStart would no longer be HOME.
    if (player_base == BASE_THIRD && player_original_base == BASE_HOME && !player_is_wounded &&
        !has_made_run_on_third_base) {
        return 1;
    }
    return 0;
}