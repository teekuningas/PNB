#include "test_helpers.h"
#include "pitching_ai_strategy.h"
#include "actions_pure/pitching_physics.h"
#include "field.h"
#include <math.h>
#include <stdio.h>

int test_decide_pitch_aim(void)
{
    // Default (count 9, 0-0): rand_dir=3 -> direction exactly 0 (dead-centre strike).
    PitchAim a = decide_pitch_aim(9, 0, 0, 2, 3, 0);
    ASSERT_TRUE(a.direction > -0.001f && a.direction < 0.001f, "rand_dir=3 -> direction 0 (strike)");

    // Empty bases (1 active batting player) still vary toss height across low, medium, and high.
    PitchAim empty_lo = decide_pitch_aim(1, 0, 0, 0, 3, 0);
    PitchAim empty_hi = decide_pitch_aim(1, 0, 0, 4, 3, 0);
    ASSERT_TRUE(empty_lo.power < 0.30f, "empty bases include low quick tosses");
    ASSERT_TRUE(empty_hi.power > 0.75f, "empty bases include high tolppa tosses");

    // Ahead in the count with runners on base widens the contrast between quick low and tolppa.
    PitchAim ahead_lo = decide_pitch_aim(3, 1, 0, 0, 3, 0);
    PitchAim ahead_hi = decide_pitch_aim(3, 1, 0, 4, 3, 0);
    ASSERT_TRUE(ahead_lo.power < empty_lo.power, "runners ahead throws quicker low tosses than empty bases");
    ASSERT_TRUE(ahead_hi.power > empty_hi.power, "runners ahead throws higher tolppa than empty bases");

    // Every intended strike stays strictly inside ±PLATE_WIDTH/2 even at the highest toss height.
    for (int rp = 0; rp < 5; rp++) {
        for (int rd = 0; rd < 7; rd++) {
            PitchAim st = decide_pitch_aim(3, 1, 1, rp, rd, 0);
            Vector3D v = pitch_velocity_from_aim(st.power, st.direction);
            float landingOffset = fabsf(v.x) * (2.0f * v.y / GRAVITY);
            ASSERT_TRUE(landingOffset < PLATE_WIDTH / 2.0f, "intended strike must land inside the plate");
        }
    }

    // Ahead in the count (a strike, no balls): deliberate väärä placements off the plate.
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
