#include "test_helpers.h"
#include "base_control.h"
#include "field.h"
#include "rules_state.h"
#include "world.h"
#include "session.h"
#include <string.h>

int test_get_base_controller(void)
{
    MatchSession game;
    RefereeState referee;
    memset(&game, 0, sizeof(MatchSession));
    memset(&referee, 0, sizeof(RefereeState));

    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        referee.batting_players[i].current_safety_base = BASE_NONE;
        referee.batting_players[i].base_at_pitch_start = BASE_NONE;
        game.players[i].batting.presence.base_id = BASE_NONE;
    }

    // Base conditions: no one controls anything
    for (int i = 0; i < BASE_COUNT; i++) {
        ASSERT_EQ(-1, get_base_controller(&game, &referee, (BaseID)i), "Empty game should have no base controllers");
    }

    // Invalid base ID
    ASSERT_EQ(-1, get_base_controller(&game, &referee, BASE_NONE), "Invalid base should return -1");

    // Player 2 controls first base
    referee.batting_players[2].current_safety_base = BASE_FIRST;
    game.players[2].batting.presence.base_id = BASE_FIRST;
    referee.batting_players[2].base_at_pitch_start = BASE_HOME;

    ASSERT_EQ(2, get_base_controller(&game, &referee, BASE_FIRST), "Player 2 should control first base");

    // Multiple candidates for second base, prioritize higher base_at_pitch_start
    referee.batting_players[4].current_safety_base = BASE_SECOND;
    game.players[4].batting.presence.base_id = BASE_SECOND;
    referee.batting_players[4].base_at_pitch_start = BASE_FIRST; // Lower priority

    referee.batting_players[5].current_safety_base = BASE_SECOND;
    game.players[5].batting.presence.base_id = BASE_SECOND;
    referee.batting_players[5].base_at_pitch_start = BASE_SECOND; // Higher priority

    ASSERT_EQ(
        5, get_base_controller(&game, &referee, BASE_SECOND),
        "Player 5 should control second base due to higher start base"
    );

    // Player must have both safety and physically be there
    referee.batting_players[6].current_safety_base = BASE_THIRD; // Has safety
    game.players[6].batting.presence.base_id = BASE_SECOND; // But physically at second

    ASSERT_EQ(
        -1, get_base_controller(&game, &referee, BASE_THIRD), "No control if player is not physically at the base"
    );

    return TEST_PASSED;
}

int test_get_ball_at_base_index(void)
{
    StateInfo state;
    MatchSession game;
    FieldPositions fp;

    memset(&state, 0, sizeof(StateInfo));
    memset(&game, 0, sizeof(MatchSession));
    memset(&fp, 0, sizeof(FieldPositions));

    state.match = &game;
    state.field_positions = &fp;

    // No one has ball
    game.ball.holder = -1;
    ASSERT_EQ(-1, get_ball_at_base_index(&game, &fp), "Returns -1 if no one has ball");

    // Someone has ball, setup bases
    game.ball.holder = 14;

    // Set up pitch plate and home radius check
    fp.pitch_plate = (Vector3D){0.0f, 0.0f, 0.0f};
    game.ball.location = (Vector3D){0.0f, 0.0f, 1.0f}; // Inside home area

    // Home base uses HOME_LINE_Z check (usually 0 in game coordinates, assuming 0 here)
    // By setting z=1.0 and x=0, dx=0, dz=1, which should be within HOME_RADIUS
    ASSERT_EQ(0, get_ball_at_base_index(&game, &fp), "Returns 0 for Home Base");

    // Test first base
    fp.first_base = (Vector3D){-20.0f, 0.0f, -20.0f};
    game.ball.location = fp.first_base;
    ASSERT_EQ(1, get_ball_at_base_index(&game, &fp), "Returns 1 for First Base");

    // Test second base
    fp.second_base = (Vector3D){20.0f, 0.0f, -40.0f};
    game.ball.location = fp.second_base;
    ASSERT_EQ(2, get_ball_at_base_index(&game, &fp), "Returns 2 for Second Base");

    // Test third base
    fp.third_base = (Vector3D){30.0f, 0.0f, -20.0f};
    game.ball.location = fp.third_base;
    ASSERT_EQ(3, get_ball_at_base_index(&game, &fp), "Returns 3 for Third Base");

    return TEST_PASSED;
}