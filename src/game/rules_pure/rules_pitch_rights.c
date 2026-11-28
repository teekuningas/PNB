#include "rules_pitch_rights.h"
#include "rules_state.h"

PitchRightsState advance_pitch_rights(PitchRightsState current, int ball_at_base)
{
    switch (current) {
    case PITCH_RIGHTS_NEED_HOME:
        if (ball_at_base == 0) return PITCH_RIGHTS_NEED_FIELD_BASE;
        return PITCH_RIGHTS_NEED_HOME;

    case PITCH_RIGHTS_NEED_FIELD_BASE:
        if (ball_at_base >= 1 && ball_at_base <= 3) return PITCH_RIGHTS_NEED_RETURN_HOME;
        return PITCH_RIGHTS_NEED_FIELD_BASE;

    case PITCH_RIGHTS_NEED_RETURN_HOME:
        if (ball_at_base == 0) return PITCH_RIGHTS_GRANTED;
        return PITCH_RIGHTS_NEED_RETURN_HOME;

    case PITCH_RIGHTS_GRANTED:
    default:
        return PITCH_RIGHTS_GRANTED;
    }
}

PitchRightsState pitch_rights_after_foul_reset(int strikes_at_pitch_start)
{
    return (strikes_at_pitch_start < 2) ? PITCH_RIGHTS_GRANTED : PITCH_RIGHTS_NEED_HOME;
}
