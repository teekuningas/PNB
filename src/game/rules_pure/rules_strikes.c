#include "rules_strikes.h"

PitchResult determine_pitch_result(float ball_x, float plate_width)
{
    if (ball_x < plate_width / 2.0f && ball_x > -plate_width / 2.0f) {
        return PITCH_RESULT_STRIKE;
    } else {
        return PITCH_RESULT_BALL;
    }
}
