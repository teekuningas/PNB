#include "test_helpers.h"
#include "common_logic.h"
#include "globals.h"

/**
 * CONTRACT: clear_frame_events clears ALL fields in GameEvents.
 *
 * This test serves two purposes:
 * 1. Verifies that every known field is zeroed by clear_frame_events().
 * 2. Detects if GameEvents grows (new field added) without updating
 *    clear_frame_events — the size check will fail, forcing the developer
 *    to update both clear_frame_events() and this test.
 */
int test_clear_frame_events_completeness(void)
{
    GameEvents events;

    // Set every field to non-zero
    events.catchMade = 1;
    events.playerArrivedAtBase = 1;
    events.pitchReleased = 1;
    events.ballHitByBat = 1;
    events.ballMissedByBat = 1;
    events.ballHitGround = 1;
    events.freeWalkAccepted = 1;
    events.freeWalkRejected = 1;
    events.batterEntered = 1;

    clear_frame_events(&events);

    // Verify each field is cleared
    ASSERT_EQ(0, events.catchMade, "catchMade cleared");
    ASSERT_EQ(0, events.playerArrivedAtBase, "playerArrivedAtBase cleared");
    ASSERT_EQ(0, events.pitchReleased, "pitchReleased cleared");
    ASSERT_EQ(0, events.ballHitByBat, "ballHitByBat cleared");
    ASSERT_EQ(0, events.ballMissedByBat, "ballMissedByBat cleared");
    ASSERT_EQ(0, events.ballHitGround, "ballHitGround cleared");
    ASSERT_EQ(0, events.freeWalkAccepted, "freeWalkAccepted cleared");
    ASSERT_EQ(0, events.freeWalkRejected, "freeWalkRejected cleared");
    ASSERT_EQ(0, events.batterEntered, "batterEntered cleared");

    // Guard: if GameEvents gains a new field, this assertion fails.
    // 9 int fields × sizeof(int) = expected size.
    // Update clear_frame_events(), the field checks above, AND this count.
    ASSERT_EQ(9 * sizeof(int), sizeof(GameEvents), "GameEvents size changed — update clear_frame_events and this test");

    return TEST_PASSED;
}
