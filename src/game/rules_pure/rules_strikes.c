#include "rules_strikes.h"

PitchResult determine_pitch_result(float ball_x, float plate_width)
{
    if (ball_x < plate_width / 2.0f && ball_x > -plate_width / 2.0f) {
        return PITCH_RESULT_STRIKE;
    } else {
        return PITCH_RESULT_BALL;
    }
}

// §18(1) — "Lyöjä muuttuu lopullisesti etenijäksi, kun hän saa kolme oikeaa syöttöä."
// A pitch is always correct when the batter swings (§23), and a taken pitch that lands on the plate is
// correct too, so this engine's strike count IS the count of correct pitches the batter has received.
// A player who has permanently become a runner is no longer in the batting turn and no longer safe at
// home, so no further pitch may be aimed at him — in every mode.
int batter_has_become_runner_permanently(int correct_pitches_received)
{
    return correct_pitches_received >= CORRECT_PITCHES_PER_BATTING_TURN;
}
