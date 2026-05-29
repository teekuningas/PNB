#include "pitching_ai_strategy.h"
#include "actions_pure/pitching_physics.h"

void calculate_ai_pitch_targets(
    int rand1, int rand2, int rand3, int batting_team_players_on_field_count, const HalfInningState* halfInningState,
    int animation_frequency, unsigned int* out_first_limit, unsigned int* out_second_limit
)
{
    int strikes = halfInningState->strikes;
    int balls = halfInningState->balls;
    int var = 0;

    if (batting_team_players_on_field_count == 1) {
        rand1 = 0;
    } else if (strikes != 0 && balls == 0) {
        if (rand3 == 9) {
            var = 10;
        } else if (rand3 == 8) {
            var = -10;
        }
    }

    *out_first_limit = (PITCH_UP_MAX - PITCH_DOWN_MAX) * animation_frequency + 5 + rand1;
    *out_second_limit = animation_frequency * PITCH_DOWN_MAX - 2 + rand2 + var;
}
