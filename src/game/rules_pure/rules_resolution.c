#include "rules_resolution.h"

int player_has_reached_final_resolution(int has_scored, int is_wounded, int is_burnt, int advance_voided)
{
    return (has_scored != 0) || (is_wounded != 0) || (is_burnt != 0) || (advance_voided != 0);
}
