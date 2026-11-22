#include "test_helpers.h"
#include "rules_state.h"
#include "batting_ai_strategy.h"
#include "actions/batting_system.h"
#include "actions_pure/swing_geometry.h"
#include <stdio.h>
#include <math.h>

int test_batting_strategy_decision_tree(void)
{
    BattingStrategy s;
    HalfInningState gs = {0};

    // Period < 4, Strikes 0 -> Style 1 probe, no pre-pitch running
    gs.strikes = 0;
    s = calculate_batting_strategy(&gs, 0, 0, 0, 0);
    ASSERT_EQ(1, s.style, "Strikes 0 -> Style 1");
    ASSERT_EQ(0, s.run_base_runners, "Strikes 0 -> No run base");
    ASSERT_EQ(0, s.run_batter, "Strikes 0 -> No run batter");

    // Period < 4, Strikes 1, Field 0 (empty), Slow batter -> tries a normal hit instead of wounding
    gs.strikes = 1;
    s = calculate_batting_strategy(&gs, 0, 0, 2, 0); // speed 2 is slow
    ASSERT_EQ(1, s.style, "S1, F0, Slow -> Style 1 (Normal)");
    ASSERT_EQ(1, s.run_batter, "S1, F0, Slow -> Run Batter");

    // Period < 4, Strikes 1, Field 0, Fast batter (speed > 2)
    gs.strikes = 1;
    s = calculate_batting_strategy(&gs, 0, 0, 3, 0); // speed 3 is fast
    ASSERT_EQ(0, s.style, "S1, F0, Fast -> Style 0 (Bunt)");
    ASSERT_EQ(1, s.run_batter, "S1, F0, Fast -> Run Batter");

    // Period < 4, Strikes 1, Field 1 (runner on 2nd/3rd, 1st empty), No Power -> squeeze/advance bunt
    gs.strikes = 1;
    s = calculate_batting_strategy(&gs, 1, 2, 2, 0);
    ASSERT_EQ(0, s.style, "S1, F1, NoPower -> Style 0 (Bunt)");
    ASSERT_EQ(1, s.run_base_runners, "S1, F1, NoPower -> Run Base");
    ASSERT_EQ(0, s.run_batter, "S1, F1, NoPower, Slow -> No Run Batter");

    // Period < 4, Strikes 2, Field 0 (empty), Slow batter -> wounding koppi
    gs.strikes = 2;
    s = calculate_batting_strategy(&gs, 0, 0, 2, 0);
    ASSERT_EQ(2, s.style, "S2, F0, Slow -> Style 2 (Wound)");
    ASSERT_EQ(1, s.run_batter, "S2, F0, Slow -> Run Batter");

    // Period < 4, Strikes 2, Field 1 (runner on 2nd/3rd, 1st empty) -> must try to drive/score runner
    gs.strikes = 2;
    s = calculate_batting_strategy(&gs, 1, 3, 2, 0);
    ASSERT_EQ(1, s.style, "S2, F1, Power -> Style 1 (Normal)");
    ASSERT_EQ(1, s.run_base_runners, "S2, F1, Power -> Run Base (score from 3rd)");
    ASSERT_EQ(1, s.run_batter, "S2, F1, Power -> Run Batter");

    s = calculate_batting_strategy(&gs, 1, 2, 2, 0);
    ASSERT_EQ(0, s.style, "S2, F1, NoPower -> Style 0 (Bunt)");
    ASSERT_EQ(1, s.run_base_runners, "S2, F1, NoPower -> Run Base");
    ASSERT_EQ(1, s.run_batter, "S2, F1, NoPower -> Run Batter");

    // Period < 4, Strikes 2, Field 2 (1st base occupied), Power batter
    gs.strikes = 2;
    s = calculate_batting_strategy(&gs, 2, 3, 0, 0); // power 3
    ASSERT_EQ(1, s.style, "S2, F2, Power -> Style 1");
    ASSERT_EQ(1, s.run_base_runners, "S2, F2, Power -> Run Base");

    // Period >= 4 (homerun contest), Strikes 1 -> tries to score runner on 3rd
    gs.strikes = 1;
    s = calculate_batting_strategy(&gs, 1, 3, 2, 4);
    ASSERT_EQ(1, s.style, "Per4, S1, Power -> Style 1");
    ASSERT_EQ(1, s.run_base_runners, "Per4, S1, Power -> Run Base");
    ASSERT_EQ(0, s.run_batter, "Per4, S1 -> No Run Batter");

    // Period >= 4, Strikes 2, No Power
    gs.strikes = 2;
    s = calculate_batting_strategy(&gs, 0, 2, 0, 4); // period 4
    ASSERT_EQ(0, s.style, "Per4, S2, NoPower -> Style 0");
    ASSERT_EQ(1, s.run_base_runners, "Per4, S2, NoPower -> Run Base");
    ASSERT_EQ(0, s.run_batter, "Per4, S2, NoPower -> No Run Batter");

    return TEST_PASSED;
}

int test_should_change_batter(void)
{
    // Field 0 (empty): prefer fast leadoff runner
    ASSERT_EQ(0, should_change_batter(0, 0, 3), "Field 0, Fast -> 0");
    ASSERT_EQ(1, should_change_batter(0, 0, 2), "Field 0, Slow -> 1");

    // Field 1 (runner on 2nd/3rd, scoring position): prefer power hitter
    ASSERT_EQ(0, should_change_batter(1, 3, 0), "Field 1, Power -> 0");
    ASSERT_EQ(1, should_change_batter(1, 2, 0), "Field 1, NoPower -> 1");

    // Field 2 (runner on 1st): prefer power hitter
    ASSERT_EQ(0, should_change_batter(2, 3, 0), "Field 2, Power -> 0");
    ASSERT_EQ(1, should_change_batter(2, 2, 0), "Field 2, NoPower -> 1");

    return TEST_PASSED;
}

int test_is_wrong_pitch(void)
{
    float gravity = 0.003f;
    float plate_width = 1.5f; // half is 0.75

    // Case 1: High velocity X, causing large offset
    // t = vy*2/g. Let vy = 0.1 -> t = 0.2 / 0.003 = 66.6
    // vx = 0.02. offset = 0.02 * 66.6 = 1.33 > 0.75 -> Wrong Pitch
    ASSERT_EQ(1, is_wrong_pitch(0.02f, 0.1f, gravity, plate_width), "Large offset -> Wrong pitch");

    // Case 2: Low velocity X
    // vx = 0.005. offset = 0.005 * 66.6 = 0.33 < 0.75 -> Good pitch
    ASSERT_EQ(0, is_wrong_pitch(0.005f, 0.1f, gravity, plate_width), "Small offset -> Good pitch");

    return TEST_PASSED;
}

int test_calculate_ai_batting_angle(void)
{
    // Style 1 (normal swing): gap-weighted full-field fan in [-0.38, +0.38], populating all 5 buckets
    // while favoring the left and right seams over dead centre.
    {
        const float bound = 0.38f;
        float minAngle = 1000.0f, maxAngle = -1000.0f;
        int bins[5] = {0};
        for (int r = 0; r < 1000; r++) {
            float a = calculate_ai_batting_angle(1, r);
            ASSERT_TRUE(fabs(a) <= bound + 0.001f, "normal angle must stay within [-0.38, +0.38]");
            if (a < minAngle) minAngle = a;
            if (a > maxAngle) maxAngle = a;
            int bin = (int)((a + bound) / (2.0f * bound) * 5.0f);
            if (bin < 0) bin = 0;
            if (bin > 4) bin = 4;
            bins[bin]++;
        }
        ASSERT_TRUE(minAngle < -0.8f * bound, "normal swing reaches right edge");
        ASSERT_TRUE(maxAngle > 0.8f * bound, "normal swing reaches left edge");
        for (int b = 0; b < 5; b++) {
            ASSERT_TRUE(bins[b] >= 90, "every bucket of the normal fan stays populated");
        }
        ASSERT_TRUE(bins[0] > bins[2] && bins[4] > bins[2], "normal swing weights the side seams over dead centre");
    }

    // Style 0 (bunt): stays within [-0.32, +0.32], avoids straight at the pitcher (|a| >= 0.14),
    // and favors the wide-open front-left pocket (positive bat angle).
    {
        const float bound = 0.32f;
        int leftPocket = 0, rightPocket = 0;
        for (int r = 0; r < 1000; r++) {
            float a = calculate_ai_batting_angle(0, r);
            ASSERT_TRUE(fabs(a) <= bound + 0.001f, "bunt angle must stay within [-0.32, +0.32]");
            ASSERT_TRUE(fabs(a) >= 0.139f, "bunt avoids hitting straight back to the pitcher");
            if (a > 0.0f)
                leftPocket++;
            else
                rightPocket++;
        }
        ASSERT_TRUE(leftPocket > rightPocket, "bunt favors the open front-left pocket");
        ASSERT_TRUE(rightPocket >= 300, "bunt still uses the front-right pocket");
    }

    // Style 2 (wounding / koppi): stays in fair deep outfield [-0.22, +0.22] rather than foul lines.
    for (int r = 0; r < 1000; r += 37) {
        float w = calculate_ai_batting_angle(2, r);
        ASSERT_TRUE(fabs(w) <= 0.221f, "wounding koppi aims into fair deep outfield");
    }

    return TEST_PASSED;
}

int test_decide_swing_profiles(void)
{
    // Style 0 (bunt): always strictly below SWING_BUNT_MAX_POWER and driven downward (vertical < FOCAL).
    for (int rp = 0; rp < 19; rp += 3) {
        for (int rv = 0; rv < 201; rv += 25) {
            SwingDecision d = decide_swing(0, rp, rv);
            ASSERT_TRUE(d.power > 0.0f && d.power < SWING_BUNT_MAX_POWER, "bunt power stays in bunt regime");
            ASSERT_TRUE(d.vertical < SWING_VERTICAL_FOCAL, "bunt vertical stays downward to avoid pop-up koppi");
        }
    }

    // Style 1 (normal): always a real swing above SWING_BUNT_MAX_POWER, spanning grounders and drives.
    int belowFocal = 0, aboveFocal = 0;
    for (int rp = 0; rp < 19; rp++) {
        for (int rv = 0; rv < 201; rv += 10) {
            SwingDecision d = decide_swing(1, rp, rv);
            ASSERT_TRUE(d.power > SWING_BUNT_MAX_POWER && d.power <= 1.0f, "normal swing never accidentally bunts");
            if (d.vertical < SWING_VERTICAL_FOCAL)
                belowFocal++;
            else
                aboveFocal++;
        }
    }
    ASSERT_TRUE(belowFocal > aboveFocal, "normal swing biases toward low line drives/grounders");
    ASSERT_TRUE(aboveFocal > 0, "normal swing still produces lofted drives for variety");

    // Style 2 (wounding / koppi): firm power and deliberately lofted (vertical > FOCAL).
    for (int rv = 0; rv < 201; rv += 25) {
        SwingDecision d = decide_swing(2, 0, rv);
        ASSERT_TRUE(d.power > SWING_BUNT_MAX_POWER, "wounding swing is a real swing");
        ASSERT_TRUE(d.vertical > SWING_VERTICAL_FOCAL, "wounding swing lofts for a koppi");
    }

    return TEST_PASSED;
}
