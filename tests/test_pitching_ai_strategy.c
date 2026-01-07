#include "test_pitching_ai_strategy.h"
#include "test_helpers.h"
#include "pitching_ai_strategy.h"
#include "pitching_physics.h"
#include <stdio.h>

int test_calculate_ai_pitch_targets(void) {

    unsigned int limit1, limit2;

    int animation_freq = 3;

    GameState gs = {0};



    // Test case 1: Standard case

    int onFieldCount = 9;

    gs.strikes = 0;

    gs.balls = 0;

    calculate_ai_pitch_targets(5, 1, 0, onFieldCount, &gs, animation_freq, &limit1, &limit2);

    

    // limit1 = (13 - 9) * 3 + 5 + 5 = 4*3 + 10 = 12 + 10 = 22

    // limit2 = 3 * 9 - 2 + 1 + 0 = 27 - 2 + 1 = 26

    ASSERT_EQ(22, limit1, "Standard case limit1 mismatch");

    ASSERT_EQ(26, limit2, "Standard case limit2 mismatch");



    // Test case 2: Single player on field (rand1 becomes 0)

    onFieldCount = 1;

    gs.strikes = 0;

    gs.balls = 0;

    calculate_ai_pitch_targets(10, 0, 5, onFieldCount, &gs, animation_freq, &limit1, &limit2);

    // limit1 = (13 - 9) * 3 + 5 + 0 = 12 + 5 = 17

    // limit2 = 3 * 9 - 2 + 0 + 0 = 25

    ASSERT_EQ(17, limit1, "Single player case limit1 mismatch");

    ASSERT_EQ(25, limit2, "Single player case limit2 mismatch");



    // Test case 3: 0 balls, some strikes, rand3 = 9 (var = 10)

    onFieldCount = 9;

    gs.strikes = 1;

    gs.balls = 0;

    calculate_ai_pitch_targets(0, 0, 9, onFieldCount, &gs, animation_freq, &limit1, &limit2);

    // limit1 = (4)*3 + 5 + 0 = 17

    // limit2 = 27 - 2 + 0 + 10 = 35

    ASSERT_EQ(17, limit1, "Strike/No ball var=10 case limit1 mismatch");

    ASSERT_EQ(35, limit2, "Strike/No ball var=10 case limit2 mismatch");

    

    // Test case 4: 0 balls, some strikes, rand3 = 8 (var = -10)

    onFieldCount = 9;

    gs.strikes = 1;

    gs.balls = 0;

    calculate_ai_pitch_targets(0, 0, 8, onFieldCount, &gs, animation_freq, &limit1, &limit2);

    // limit1 = 17

    // limit2 = 27 - 2 + 0 - 10 = 15

    ASSERT_EQ(17, limit1, "Strike/No ball var=-10 case limit1 mismatch");

    ASSERT_EQ(15, limit2, "Strike/No ball var=-10 case limit2 mismatch");

    

    return TEST_PASSED;

}
