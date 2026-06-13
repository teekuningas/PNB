#include "test_helpers.h"
#include "batting_ai_strategy.h"
#include <stdio.h>
#include <math.h>

int test_batting_strategy_decision_tree()
{
    printf("Running test: %s\n", __func__);

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

int test_should_change_batter()
{
    printf("Running test: %s\n", __func__);

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

int test_is_wrong_pitch()
{
    printf("Running test: %s\n", __func__);

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

int test_calculate_ai_batting_angle()
{
    // Style 1 (Normal): direction is randomized across the field, independent of runners.
    // Sweep the random input and assert the output stays within the reachable field range
    // AND genuinely spreads to both sides (it must NOT collapse to center).
    float minAngle = 1000.0f;
    float maxAngle = -1000.0f;
    for (int r = 0; r < 1000; r++) {
        float angle = calculate_ai_batting_angle(1, r);
        ASSERT_TRUE(fabs(angle) <= 0.45f, "Normal style angle must stay within the reachable field");
        if (angle < minAngle) minAngle = angle;
        if (angle > maxAngle) maxAngle = angle;
    }
    ASSERT_TRUE(minAngle < -0.3f, "Normal style should sometimes hit well to the right");
    ASSERT_TRUE(maxAngle > 0.3f, "Normal style should sometimes hit well to the left");

    // Style 0 (Bunt): short hit to a random side (~ +-0.5 with +-0.25 variance)
    float bunt = calculate_ai_batting_angle(0, 0);
    ASSERT_TRUE(fabs(fabs(bunt) - 0.5f) < 0.26f, "Bunt style should hit short to a side");

    // Style 2 (Wound) -> -1.5 (No variance for wounds currently)
    float wound = calculate_ai_batting_angle(2, 0);
    ASSERT_TRUE(fabs(wound - (-1.5f)) < 0.001f, "Wound style should hit extreme angle");

    return TEST_PASSED;
}
