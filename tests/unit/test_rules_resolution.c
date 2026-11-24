#include "test_helpers.h"
#include "rules_resolution.h"

// §21 Etenijän pääseminen lopulliseen ratkaisuun — the official pesäpallo rulebook.

int test_player_has_reached_final_resolution(void)
{
    // Active runner who has not met any of §21's four clauses is NOT resolved.
    ASSERT_EQ(0, player_has_reached_final_resolution(0, 0, 0, 0), "active runner has not reached final resolution");

    // Each of §21's four numbered clauses independently resolves the runner:
    //   1) tuo juoksun (scored a regular run),
    //   2) haavoittuu (terminally wounded),
    //   3) palaa (burnt),
    //   4) eteneminen on mitätöity (advance voided).
    ASSERT_EQ(1, player_has_reached_final_resolution(1, 0, 0, 0), "§21(1): regular run resolves runner");
    ASSERT_EQ(1, player_has_reached_final_resolution(0, 1, 0, 0), "§21(2): terminal wound resolves runner");
    ASSERT_EQ(1, player_has_reached_final_resolution(0, 0, 1, 0), "§21(3): burn resolves runner");
    ASSERT_EQ(1, player_has_reached_final_resolution(0, 0, 0, 1), "§21(4): voided advance resolves runner");

    // Compound resolutions (e.g. burnt runner whose advance is also voided) remain 1.
    ASSERT_EQ(1, player_has_reached_final_resolution(0, 0, 1, 1), "compound clauses return 1");
    ASSERT_EQ(1, player_has_reached_final_resolution(1, 1, 1, 1), "all clauses set returns 1");

    return TEST_PASSED;
}
