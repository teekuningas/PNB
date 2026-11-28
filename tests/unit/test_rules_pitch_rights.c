#include "test_helpers.h"
#include "rules_pitch_rights.h"

// §24 Syöttöoikeus — strict Home -> Field Base -> Home state machine.

int test_advance_pitch_rights_round_trip(void)
{
    // GRANTED stays GRANTED regardless of where the ball is held.
    for (int base = -1; base <= 3; base++) {
        ASSERT_EQ(
            PITCH_RIGHTS_GRANTED, (int)advance_pitch_rights(PITCH_RIGHTS_GRANTED, base),
            "GRANTED stays GRANTED for every ball position"
        );
    }

    // NEED_HOME requires Home (0) FIRST; a field base (1..3) before Home does NOT advance!
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_HOME, (int)advance_pitch_rights(PITCH_RIGHTS_NEED_HOME, -1),
        "NEED_HOME ignores ball not at a base"
    );
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_HOME, (int)advance_pitch_rights(PITCH_RIGHTS_NEED_HOME, 1),
        "§24: field base 1st before Home does NOT advance NEED_HOME"
    );
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_HOME, (int)advance_pitch_rights(PITCH_RIGHTS_NEED_HOME, 2),
        "§24: field base 2nd before Home does NOT advance NEED_HOME"
    );
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_HOME, (int)advance_pitch_rights(PITCH_RIGHTS_NEED_HOME, 3),
        "§24: field base 3rd before Home does NOT advance NEED_HOME"
    );
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_FIELD_BASE, (int)advance_pitch_rights(PITCH_RIGHTS_NEED_HOME, 0),
        "§24: Home advances NEED_HOME -> NEED_FIELD_BASE"
    );

    // NEED_FIELD_BASE advances on any field base (1st, 2nd, or 3rd), stays put on Home or loose.
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_FIELD_BASE, (int)advance_pitch_rights(PITCH_RIGHTS_NEED_FIELD_BASE, -1),
        "NEED_FIELD_BASE ignores loose/outfield ball"
    );
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_FIELD_BASE, (int)advance_pitch_rights(PITCH_RIGHTS_NEED_FIELD_BASE, 0),
        "NEED_FIELD_BASE stays pending while ball remains at Home"
    );
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_RETURN_HOME, (int)advance_pitch_rights(PITCH_RIGHTS_NEED_FIELD_BASE, 1),
        "§24: 1st base advances NEED_FIELD_BASE -> NEED_RETURN_HOME"
    );
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_RETURN_HOME, (int)advance_pitch_rights(PITCH_RIGHTS_NEED_FIELD_BASE, 2),
        "§24: 2nd base advances NEED_FIELD_BASE -> NEED_RETURN_HOME"
    );
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_RETURN_HOME, (int)advance_pitch_rights(PITCH_RIGHTS_NEED_FIELD_BASE, 3),
        "§24: 3rd base advances NEED_FIELD_BASE -> NEED_RETURN_HOME"
    );

    // NEED_RETURN_HOME grants only on Home (0); extra field-base relay throws preserve progress.
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_RETURN_HOME, (int)advance_pitch_rights(PITCH_RIGHTS_NEED_RETURN_HOME, -1),
        "NEED_RETURN_HOME waits while return throw is in flight"
    );
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_RETURN_HOME, (int)advance_pitch_rights(PITCH_RIGHTS_NEED_RETURN_HOME, 2),
        "NEED_RETURN_HOME preserves progress on relay to another field base"
    );
    ASSERT_EQ(
        PITCH_RIGHTS_GRANTED, (int)advance_pitch_rights(PITCH_RIGHTS_NEED_RETURN_HOME, 0),
        "§24: returning Home advances NEED_RETURN_HOME -> GRANTED"
    );

    return TEST_PASSED;
}

int test_pitch_rights_after_foul_reset(void)
{
    ASSERT_EQ(
        PITCH_RIGHTS_GRANTED, (int)pitch_rights_after_foul_reset(0),
        "1st-strike foul preserves GRANTED for the same batter"
    );
    ASSERT_EQ(
        PITCH_RIGHTS_GRANTED, (int)pitch_rights_after_foul_reset(1),
        "2nd-strike foul preserves GRANTED for the same batter"
    );
    ASSERT_EQ(
        PITCH_RIGHTS_NEED_HOME, (int)pitch_rights_after_foul_reset(2),
        "3rd-strike foul burned the batter (§18(6)), requiring fresh Home -> Field Base -> Home"
    );
    return TEST_PASSED;
}
