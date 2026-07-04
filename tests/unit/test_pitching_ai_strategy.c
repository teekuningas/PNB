#include "test_helpers.h"
#include "pitching_ai_strategy.h"
#include "pitching_physics.h"
#include <stdio.h>

int test_decide_pitch_aim(void)
{

    // Default (count 9, 0-0): rand_dir=3 -> direction exactly 0 (dead-centre strike).
    PitchAim a = decide_pitch_aim(9, 0, 0, 2, 3, 0);
    ASSERT_TRUE(a.direction > -0.001f && a.direction < 0.001f, "rand_dir=3 -> direction 0 (strike)");

    // One batter left -> fixed toss height (no jitter), regardless of rand_power.
    PitchAim s1 = decide_pitch_aim(1, 0, 0, 0, 3, 0);
    PitchAim s2 = decide_pitch_aim(1, 0, 0, 4, 3, 0);
    ASSERT_TRUE(s1.power == s2.power, "one batter -> power has no jitter");

    // rand_power moves the toss height around the base when more than one batter remains.
    PitchAim lo = decide_pitch_aim(9, 0, 0, 0, 3, 0);
    PitchAim hi = decide_pitch_aim(9, 0, 0, 4, 3, 0);
    ASSERT_TRUE(hi.power > lo.power, "more rand_power -> higher toss");

    // Ahead in the count (a strike, no balls): deliberate ball placements off the plate.
    PitchAim ball_hi = decide_pitch_aim(9, 1, 0, 2, 3, 9);
    ASSERT_TRUE(ball_hi.direction > 0.1f, "rand_choice=9 -> deliberate ball (off plate +)");
    PitchAim ball_lo = decide_pitch_aim(9, 1, 0, 2, 3, 8);
    ASSERT_TRUE(ball_lo.direction < -0.1f, "rand_choice=8 -> deliberate ball (off plate -)");

    // Ahead but rand_choice not 8/9 -> still a strike near the centre.
    PitchAim strike = decide_pitch_aim(9, 1, 0, 2, 3, 0);
    ASSERT_TRUE(strike.direction > -0.05f && strike.direction < 0.05f, "no deliberate ball -> centre");

    // Not ahead (balls already on the count) -> never a deliberate ball, even on rand_choice 8/9.
    PitchAim notahead = decide_pitch_aim(9, 1, 1, 2, 3, 9);
    ASSERT_TRUE(notahead.direction > -0.05f && notahead.direction < 0.05f, "balls>0 -> no deliberate ball");

    return TEST_PASSED;
}
