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

    // Period < 4, Strikes 0 -> Normal probe, no pre-pitch running
    gs.strikes = 0;
    s = calculate_batting_strategy(&gs, AI_FIELD_EMPTY, 0, 0, 0);
    ASSERT_EQ(AI_STYLE_NORMAL, s.style, "Strikes 0 -> Normal");
    ASSERT_EQ(0, s.run_base_runners, "Strikes 0 -> No run base");
    ASSERT_EQ(0, s.run_batter, "Strikes 0 -> No run batter");

    // Period < 4, Strikes 1, Empty field, Slow batter -> tries a normal hit instead of wounding
    gs.strikes = 1;
    s = calculate_batting_strategy(&gs, AI_FIELD_EMPTY, 0, 2, 0); // speed 2 is slow
    ASSERT_EQ(AI_STYLE_NORMAL, s.style, "S1, Empty, Slow -> Normal");
    ASSERT_EQ(1, s.run_batter, "S1, Empty, Slow -> Run Batter");

    // Period < 4, Strikes 1, Empty field, Fast batter (speed > 2) -> leadoff bunt
    gs.strikes = 1;
    s = calculate_batting_strategy(&gs, AI_FIELD_EMPTY, 0, 3, 0); // speed 3 is fast
    ASSERT_EQ(AI_STYLE_BUNT, s.style, "S1, Empty, Fast -> Bunt");
    ASSERT_EQ(1, s.run_batter, "S1, Empty, Fast -> Run Batter");

    // Period < 4, Strikes 1, Scoring position, Low power -> normal drive (no suicide bunt!)
    gs.strikes = 1;
    s = calculate_batting_strategy(&gs, AI_FIELD_SCORING, 2, 2, 0);
    ASSERT_EQ(AI_STYLE_NORMAL, s.style, "S1, Scoring, LowPower -> Normal (no suicide bunt)");
    ASSERT_EQ(1, s.run_base_runners, "S1, Scoring, LowPower -> Run Base");
    ASSERT_EQ(0, s.run_batter, "S1, Scoring, LowPower, Slow -> No Run Batter");

    // Period < 4, Strikes 2, Empty field, Slow batter -> deliberate wounding koppi
    gs.strikes = 2;
    s = calculate_batting_strategy(&gs, AI_FIELD_EMPTY, 0, 2, 0);
    ASSERT_EQ(AI_STYLE_WOUND, s.style, "S2, Empty, Slow -> Wound");
    ASSERT_EQ(1, s.run_batter, "S2, Empty, Slow -> Run Batter");

    // Period < 4, Strikes 2, Scoring position -> always normal drive to advance/score runners
    gs.strikes = 2;
    s = calculate_batting_strategy(&gs, AI_FIELD_SCORING, 3, 2, 0);
    ASSERT_EQ(AI_STYLE_NORMAL, s.style, "S2, Scoring, Power -> Normal");
    ASSERT_EQ(1, s.run_base_runners, "S2, Scoring, Power -> Run Base");
    ASSERT_EQ(1, s.run_batter, "S2, Scoring, Power -> Run Batter");

    s = calculate_batting_strategy(&gs, AI_FIELD_SCORING, 2, 2, 0);
    ASSERT_EQ(AI_STYLE_NORMAL, s.style, "S2, Scoring, LowPower -> Normal (no suicide bunt)");
    ASSERT_EQ(1, s.run_base_runners, "S2, Scoring, LowPower -> Run Base");
    ASSERT_EQ(1, s.run_batter, "S2, Scoring, LowPower -> Run Batter");

    // Period < 4, Strikes 2, 1st base occupied
    gs.strikes = 2;
    s = calculate_batting_strategy(&gs, AI_FIELD_FIRST_OCCUPIED, 3, 0, 0);
    ASSERT_EQ(AI_STYLE_NORMAL, s.style, "S2, First, Power -> Normal");
    ASSERT_EQ(1, s.run_base_runners, "S2, First, Power -> Run Base");

    // Period >= 4 (homerun contest), Strikes 1 -> normal drive to score runner on 3rd
    gs.strikes = 1;
    s = calculate_batting_strategy(&gs, AI_FIELD_SCORING, 2, 2, 4);
    ASSERT_EQ(AI_STYLE_NORMAL, s.style, "Per4, S1, LowPower -> Normal (no suicide bunt)");
    ASSERT_EQ(1, s.run_base_runners, "Per4, S1 -> Run Base");
    ASSERT_EQ(0, s.run_batter, "Per4, S1 -> No Run Batter");

    // Period >= 4, Strikes 2, Low power -> still drives the runner home with Normal
    gs.strikes = 2;
    s = calculate_batting_strategy(&gs, AI_FIELD_SCORING, 2, 0, 4);
    ASSERT_EQ(AI_STYLE_NORMAL, s.style, "Per4, S2, LowPower -> Normal");
    ASSERT_EQ(1, s.run_base_runners, "Per4, S2 -> Run Base");
    ASSERT_EQ(0, s.run_batter, "Per4, S2, LowPower -> No Run Batter");

    return TEST_PASSED;
}

int test_should_change_batter(void)
{
    // Empty field: prefer fast leadoff runner
    ASSERT_EQ(0, should_change_batter(AI_FIELD_EMPTY, 0, 3), "Empty, Fast -> keep");
    ASSERT_EQ(1, should_change_batter(AI_FIELD_EMPTY, 0, 2), "Empty, Slow -> change");

    // Scoring position: prefer power hitter
    ASSERT_EQ(0, should_change_batter(AI_FIELD_SCORING, 3, 0), "Scoring, Power -> keep");
    ASSERT_EQ(1, should_change_batter(AI_FIELD_SCORING, 2, 0), "Scoring, LowPower -> change");

    // Runner on 1st: prefer power hitter
    ASSERT_EQ(0, should_change_batter(AI_FIELD_FIRST_OCCUPIED, 3, 0), "First, Power -> keep");
    ASSERT_EQ(1, should_change_batter(AI_FIELD_FIRST_OCCUPIED, 2, 0), "First, LowPower -> change");

    return TEST_PASSED;
}

int test_is_wrong_pitch(void)
{
    float gravity = 0.003f;
    float plate_width = 1.5f; // half is 0.75

    // Case 1: At ground level (y=0), t = 2*vy/g = 66.67; vx=0.02 -> landing_x = 1.33 > 0.75 -> wrong.
    ASSERT_EQ(1, is_wrong_pitch(0.0f, 0.0f, 0.02f, 0.1f, gravity, plate_width), "Large offset -> Wrong pitch");

    // Case 2: At ground level (y=0), vx=0.005 -> landing_x = 0.33 < 0.75 -> strike.
    ASSERT_EQ(0, is_wrong_pitch(0.0f, 0.0f, 0.005f, 0.1f, gravity, plate_width), "Small offset -> Good pitch");

    // Case 3: Elevated release (y=1.45, low pitch vy=0.106): ground time is ~82.4 frames rather than
    // 2*vy/g = 70.7 frames, so a pitch with vx=0.0098 (0.69 at waist height, 0.81 at ground) is
    // accurately recognized as väärä instead of underestimated as a strike.
    ASSERT_EQ(
        1, is_wrong_pitch(0.0098f, 1.556f, 0.0098f, 0.103f, gravity, plate_width),
        "Low pitch landing past plate edge due to drop height is recognized as väärä"
    );

    // Case 4: Same low pitch inside the plate at ground (vx=0.008 -> landing_x ≈ 0.66 < 0.75) -> strike.
    ASSERT_EQ(
        0, is_wrong_pitch(0.008f, 1.556f, 0.008f, 0.103f, gravity, plate_width),
        "Low pitch landing inside the plate is recognized as oikea"
    );

    return TEST_PASSED;
}

int test_should_advance_on_loose_ball(void)
{
    // Breakthrough past boundary lines after fair bounce -> advance.
    ASSERT_EQ(
        1, should_advance_on_loose_ball(BAT_OUTCOME_HIT, 0, FOUL_STATE_NONE, BASE_NONE, -1, 1, 1, -20.0f, 1),
        "fair bounced breakthrough advances runners"
    );

    // Fair deep outfield gap hit (past middle fielders, z < -58) -> advance even while in bounds.
    ASSERT_EQ(
        1, should_advance_on_loose_ball(BAT_OUTCOME_HIT, 0, FOUL_STATE_NONE, BASE_NONE, -1, 1, 1, -60.0f, 0),
        "fair bounced deep outfield hit advances runners"
    );

    // Shallow fair grounder in bounds -> hold.
    ASSERT_EQ(
        0, should_advance_on_loose_ball(BAT_OUTCOME_HIT, 0, FOUL_STATE_NONE, BASE_NONE, -1, 1, 1, -35.0f, 0),
        "shallow in-bounds grounder does not trigger opportunistic advance"
    );

    // Koppi-safe: ball still in air (has_ball_hit_ground == 0) or already caught -> never advance.
    ASSERT_EQ(
        0, should_advance_on_loose_ball(BAT_OUTCOME_HIT, 0, FOUL_STATE_NONE, BASE_NONE, -1, 1, 0, -65.0f, 0),
        "airborne ball before bounce must not advance (koppi risk)"
    );
    ASSERT_EQ(
        0, should_advance_on_loose_ball(BAT_OUTCOME_HIT, 1, FOUL_STATE_NONE, BASE_NONE, -1, 1, 1, -65.0f, 1),
        "caught ball must not advance"
    );

    // Foul-safe and possession-safe: foul, held ball, or throw in flight -> never advance.
    ASSERT_EQ(
        0, should_advance_on_loose_ball(BAT_OUTCOME_HIT, 0, FOUL_STATE_DETECTED, BASE_NONE, -1, 1, 1, -65.0f, 1),
        "foul ball must not advance"
    );
    ASSERT_EQ(
        0, should_advance_on_loose_ball(BAT_OUTCOME_HIT, 0, FOUL_STATE_NONE, BASE_NONE, 14, 1, 1, -65.0f, 0),
        "held ball must not advance"
    );
    ASSERT_EQ(
        0, should_advance_on_loose_ball(BAT_OUTCOME_HIT, 0, FOUL_STATE_NONE, BASE_FIRST, -1, 1, 1, -65.0f, 0),
        "thrown ball must not advance"
    );

    return TEST_PASSED;
}

int test_calculate_ai_batting_angle(void)
{
    // Normal swing: gap-weighted full-field fan in [-0.36, +0.36], populating all 5 buckets
    // while favoring the left and right seams over dead centre.
    {
        const float bound = 0.36f;
        float minAngle = 1000.0f, maxAngle = -1000.0f;
        int bins[5] = {0};
        for (int r = 0; r < 1000; r++) {
            float a = calculate_ai_batting_angle(AI_STYLE_NORMAL, r);
            ASSERT_TRUE(fabs(a) <= bound + 0.001f, "normal angle must stay within [-0.36, +0.36]");
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

    // Bunt: stays within [-0.30, +0.32], avoids the lukkari near centre (|a| >= 0.18),
    // and heavily favors the 1st-base line (left pocket) while keeping a small right-pocket mix.
    {
        int leftPocket = 0, rightPocket = 0;
        for (int r = 0; r < 1000; r++) {
            float a = calculate_ai_batting_angle(AI_STYLE_BUNT, r);
            ASSERT_TRUE(a >= -0.301f && a <= 0.321f, "bunt angle stays within its left/right pocket bounds");
            ASSERT_TRUE(fabs(a) >= 0.179f, "bunt avoids hitting straight back to the lukkari");
            if (a > 0.0f)
                leftPocket++;
            else
                rightPocket++;
        }
        ASSERT_TRUE(leftPocket >= 800, "bunt heavily favors the 1st-base line (left pocket)");
        ASSERT_TRUE(rightPocket >= 100, "bunt keeps a small right-pocket variety mix");
    }

    // Wounding / koppi: stays in fair deep outfield [-0.22, +0.22] rather than foul lines.
    for (int r = 0; r < 1000; r += 37) {
        float w = calculate_ai_batting_angle(AI_STYLE_WOUND, r);
        ASSERT_TRUE(fabs(w) <= 0.221f, "wounding koppi aims into fair deep outfield");
    }

    return TEST_PASSED;
}

int test_decide_swing_profiles(void)
{
    // Bunt: always strictly below SWING_BUNT_MAX_POWER and driven downward (vertical < FOCAL).
    for (int rp = 0; rp < 19; rp += 3) {
        for (int rv = 0; rv < 201; rv += 25) {
            SwingDecision d = decide_swing(AI_STYLE_BUNT, rp, rv);
            ASSERT_TRUE(d.power > 0.0f && d.power < SWING_BUNT_MAX_POWER, "bunt power stays in bunt regime");
            ASSERT_TRUE(d.vertical < SWING_VERTICAL_FOCAL, "bunt vertical stays downward to avoid pop-up koppi");
        }
    }

    // Normal: always a real swing above SWING_BUNT_MAX_POWER, biased toward grounders/liners
    // with an explicit upper tail above FOCAL for lofted drives and catchable koppi.
    int belowFocal = 0, aboveFocal = 0;
    for (int rp = 0; rp < 19; rp++) {
        for (int rv = 0; rv < 201; rv += 10) {
            SwingDecision d = decide_swing(AI_STYLE_NORMAL, rp, rv);
            ASSERT_TRUE(d.power > SWING_BUNT_MAX_POWER && d.power <= 1.0f, "normal swing never accidentally bunts");
            if (d.vertical < SWING_VERTICAL_FOCAL)
                belowFocal++;
            else
                aboveFocal++;
        }
    }
    ASSERT_TRUE(belowFocal > aboveFocal, "normal swing biases toward low line drives/grounders");
    ASSERT_TRUE(aboveFocal > 0, "normal swing keeps an upper tail for lofted drives and koppi");

    // Wounding / koppi: firm power and deliberately lofted (vertical > FOCAL).
    for (int rv = 0; rv < 201; rv += 25) {
        SwingDecision d = decide_swing(AI_STYLE_WOUND, 0, rv);
        ASSERT_TRUE(d.power > SWING_BUNT_MAX_POWER, "wounding swing is a real swing");
        ASSERT_TRUE(d.vertical > SWING_VERTICAL_FOCAL, "wounding swing lofts for a koppi");
    }

    return TEST_PASSED;
}
