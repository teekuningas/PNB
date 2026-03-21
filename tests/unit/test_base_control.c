#include "test_helpers.h"
#include "base_control.h"
#include "globals.h"
#include "vector_math.h"
#include <string.h>

int test_get_base_controller()
{
    MatchSession game;
    memset(&game, 0, sizeof(MatchSession));

    for (int i = 0; i < PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        game.referee.battingPlayers[i].currentSafetyBase = BASE_NONE;
        game.referee.battingPlayers[i].baseAtPitchStart = BASE_NONE;
        game.playerInfo[i].bTPI.baseId = BASE_NONE;
    }

    // Base conditions: no one controls anything
    for (int i = 0; i < BASE_COUNT; i++) {
        ASSERT_EQ(-1, get_base_controller(&game, (BaseID)i), "Empty game should have no base controllers");
    }

    // Invalid base ID
    ASSERT_EQ(-1, get_base_controller(&game, BASE_NONE), "Invalid base should return -1");

    // Player 2 controls first base
    game.referee.battingPlayers[2].currentSafetyBase = BASE_FIRST;
    game.playerInfo[2].bTPI.baseId = BASE_FIRST;
    game.referee.battingPlayers[2].baseAtPitchStart = BASE_HOME;

    ASSERT_EQ(2, get_base_controller(&game, BASE_FIRST), "Player 2 should control first base");

    // Multiple candidates for second base, prioritize higher baseAtPitchStart
    game.referee.battingPlayers[4].currentSafetyBase = BASE_SECOND;
    game.playerInfo[4].bTPI.baseId = BASE_SECOND;
    game.referee.battingPlayers[4].baseAtPitchStart = BASE_FIRST; // Lower priority

    game.referee.battingPlayers[5].currentSafetyBase = BASE_SECOND;
    game.playerInfo[5].bTPI.baseId = BASE_SECOND;
    game.referee.battingPlayers[5].baseAtPitchStart = BASE_SECOND; // Higher priority

    ASSERT_EQ(
        5, get_base_controller(&game, BASE_SECOND), "Player 5 should control second base due to higher start base"
    );

    // Player must have both safety and physically be there
    game.referee.battingPlayers[6].currentSafetyBase = BASE_THIRD; // Has safety
    game.playerInfo[6].bTPI.baseId = BASE_SECOND; // But physically at second

    ASSERT_EQ(-1, get_base_controller(&game, BASE_THIRD), "No control if player is not physically at the base");

    return TEST_PASSED;
}

int test_get_ball_at_base_index()
{
    StateInfo state;
    MatchSession game;
    FieldPositions fp;

    memset(&state, 0, sizeof(StateInfo));
    memset(&game, 0, sizeof(MatchSession));
    memset(&fp, 0, sizeof(FieldPositions));

    state.match = &game;
    state.fieldPositions = &fp;

    // No one has ball
    game.pII.hasBallIndex = -1;
    ASSERT_EQ(-1, get_ball_at_base_index(&state), "Returns -1 if no one has ball");

    // Someone has ball, setup bases
    game.pII.hasBallIndex = 14;

    // Set up pitch plate and home radius check
    fp.pitchPlate = (Vector3D){0.0f, 0.0f, 0.0f};
    game.ballInfo.location = (Vector3D){0.0f, 0.0f, 1.0f}; // Inside home area

    // Home base uses HOME_LINE_Z check (usually 0 in game coordinates, assuming 0 here)
    // By setting z=1.0 and x=0, dx=0, dz=1, which should be within HOME_RADIUS
    ASSERT_EQ(0, get_ball_at_base_index(&state), "Returns 0 for Home Base");

    // Test first base
    fp.firstBase = (Vector3D){-20.0f, 0.0f, -20.0f};
    game.ballInfo.location = fp.firstBase;
    ASSERT_EQ(1, get_ball_at_base_index(&state), "Returns 1 for First Base");

    // Test second base
    fp.secondBase = (Vector3D){20.0f, 0.0f, -40.0f};
    game.ballInfo.location = fp.secondBase;
    ASSERT_EQ(2, get_ball_at_base_index(&state), "Returns 2 for Second Base");

    // Test third base
    fp.thirdBase = (Vector3D){30.0f, 0.0f, -20.0f};
    game.ballInfo.location = fp.thirdBase;
    ASSERT_EQ(3, get_ball_at_base_index(&state), "Returns 3 for Third Base");

    return TEST_PASSED;
}