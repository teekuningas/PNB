#include "test_helpers.h"
#include "batting_ai_strategy.h"
#include <stdio.h>
#include <math.h>

int test_batting_strategy_decision_tree(void)
{

    BattingStrategy s;
    HalfInningState gs = {0};

    // Period < 4, Strikes 0 -> Style 1, No run
    gs.strikes = 0;
    s = calculate_batting_strategy(&gs, 0, 0, 0, 0);
    ASSERT_EQ(1, s.style, "Strikes 0 -> Style 1");
    ASSERT_EQ(0, s.runBaseRunners, "Strikes 0 -> No run base");
    ASSERT_EQ(0, s.runBatter, "Strikes 0 -> No run batter");

    // Period < 4, Strikes 1, Field 0 (empty), Slow batter (speed <= 2)
    gs.strikes = 1;
    s = calculate_batting_strategy(&gs, 0, 0, 2, 0); // speed 2 is slow
    ASSERT_EQ(2, s.style, "S1, F0, Slow -> Style 2 (Wound)");
    ASSERT_EQ(1, s.runBatter, "S1, F0, Slow -> Run Batter");

    // Period < 4, Strikes 1, Field 0, Fast batter (speed > 2)
    gs.strikes = 1;
    s = calculate_batting_strategy(&gs, 0, 0, 3, 0); // speed 3 is fast
    ASSERT_EQ(0, s.style, "S1, F0, Fast -> Style 0 (Bunt)");
    ASSERT_EQ(1, s.runBatter, "S1, F0, Fast -> Run Batter");

    // Period < 4, Strikes 2, Field 2 (1st base occupied), Power batter
    gs.strikes = 2;
    s = calculate_batting_strategy(&gs, 2, 3, 0, 0); // power 3
    ASSERT_EQ(1, s.style, "S2, F2, Power -> Style 1");
    ASSERT_EQ(1, s.runBaseRunners, "S2, F2, Power -> Run Base");

    // Period >= 4 (last inning/super inning), Strikes 2, No Power
    gs.strikes = 2;
    s = calculate_batting_strategy(&gs, 0, 2, 0, 4); // period 4
    ASSERT_EQ(0, s.style, "Per4, S2, NoPower -> Style 0");
    ASSERT_EQ(1, s.runBaseRunners, "Per4, S2, NoPower -> Run Base");
    ASSERT_EQ(0, s.runBatter, "Per4, S2, NoPower -> No Run Batter");

    return TEST_PASSED;
}

int test_should_change_batter(void)
{

    // Field 0, Speed 3 -> No change
    ASSERT_EQ(0, should_change_batter(0, 0, 3), "Field 0, Fast -> 0");
    // Field 0, Speed 2 -> Change
    ASSERT_EQ(1, should_change_batter(0, 0, 2), "Field 0, Slow -> 1");

    // Field 2, Power 3 -> No change
    ASSERT_EQ(0, should_change_batter(2, 3, 0), "Field 2, Power -> 0");
    // Field 2, Power 2 -> Change
    ASSERT_EQ(1, should_change_batter(2, 2, 0), "Field 2, NoPower -> 1");

    // Field 1 -> No change usually
    ASSERT_EQ(0, should_change_batter(1, 0, 0), "Field 1 -> 0");

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

    // The normal swing (style 1) AND the bunt (style 0) must each produce a UNIFORM direction fan
    // within their own bound: a flat, full-width spread — no collapse to centre, no pile at an
    // extreme. Bounds mirror AI_MAX_BATTING_ANGLE (0.38) and AI_BUNT_BATTING_ANGLE (0.32) in
    // batting_ai_strategy.c (the bunt fan is a touch tighter because a short hit fouls more easily).
    struct {
        int style;
        float bound;
    } cases[] = {{1, 0.38f}, {0, 0.32f}};

    for (int c = 0; c < 2; c++) {
        int style = cases[c].style;
        float bound = cases[c].bound;
        float minAngle = 1000.0f, maxAngle = -1000.0f;
        int bins[5] = {0};

        for (int r = 0; r < 1000; r++) {
            float a = calculate_ai_batting_angle(style, r);
            ASSERT_TRUE(fabs(a) <= bound + 0.001f, "angle must stay within the style's reachable fan");
            if (a < minAngle) minAngle = a;
            if (a > maxAngle) maxAngle = a;
            int bin = (int)((a + bound) / (2.0f * bound) * 5.0f);
            if (bin < 0) bin = 0;
            if (bin > 4) bin = 4;
            bins[bin]++;
        }

        // Reaches both edges of its fan (genuine spread, not a timid centre cluster).
        ASSERT_TRUE(minAngle < -0.8f * bound, "should sometimes hit near the right edge");
        ASSERT_TRUE(maxAngle > 0.8f * bound, "should sometimes hit near the left edge");
        // Uniform: every one of the 5 equal buckets is well populated (ideal ~200 each over 1000).
        for (int b = 0; b < 5; b++) {
            ASSERT_TRUE(bins[b] > 120, "direction must be a uniform spread, not collapsed or piled up");
        }
    }

    // Style 2 (Wound): a deliberate fixed extreme angle to draw a fielder (not a uniform fan).
    float wound = calculate_ai_batting_angle(2, 0);
    ASSERT_FLOAT_EQ(-1.5f, wound, 0.001f, "Wound style should hit extreme angle");

    return TEST_PASSED;
}
