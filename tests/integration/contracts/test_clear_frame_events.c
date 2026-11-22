#include "test_helpers.h"
#include "frame_events.h"
#include "world.h"

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
    events.catch_made = 1;
    events.player_arrived_at_base = 1;
    events.pitch_released = 1;
    events.ball_hit_by_bat = 1;
    events.ball_missed_by_bat = 1;
    events.ball_hit_ground = 1;
    events.free_walk_accepted = 1;
    events.batter_entered = 1;
    events.homeward_runs_started = 2;

    clear_frame_events(&events);

    // Verify each field is cleared
    ASSERT_EQ(0, events.catch_made, "catch_made cleared");
    ASSERT_EQ(0, events.player_arrived_at_base, "player_arrived_at_base cleared");
    ASSERT_EQ(0, events.pitch_released, "pitch_released cleared");
    ASSERT_EQ(0, events.ball_hit_by_bat, "ball_hit_by_bat cleared");
    ASSERT_EQ(0, events.ball_missed_by_bat, "ball_missed_by_bat cleared");
    ASSERT_EQ(0, events.ball_hit_ground, "ball_hit_ground cleared");
    ASSERT_EQ(0, events.free_walk_accepted, "free_walk_accepted cleared");
    ASSERT_EQ(0, events.batter_entered, "batter_entered cleared");
    ASSERT_EQ(0, events.homeward_runs_started, "homeward route starts cleared");

    // Guard: if GameEvents gains a new field, this assertion fails.
    // 9 int fields × sizeof(int) = expected size.
    // Update clear_frame_events(), the field checks above, AND this count.
    ASSERT_EQ(9 * sizeof(int), sizeof(GameEvents), "GameEvents size changed — update clear_frame_events and this test");

    return TEST_PASSED;
}
