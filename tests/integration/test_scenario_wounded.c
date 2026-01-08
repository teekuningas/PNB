#include "test_scenario_wounded.h"
#include "fixtures.h"
#include "../test_helpers.h"
#include "game_setup.h"
#include "common_logic.h"
#include "game_analysis.h"
#include "game_manipulation.h"
#include "mutable_world.h"
#include "geometry.h" // Needed for geometry_distance_2d_xz
#include "referee.h" // For is_player_marked_for_wound

// Defined in game_analysis.c
#define WOUNDING_CATCH_THRESHOLD (1.0f * (1 / (UPDATE_INTERVAL*1.0f/1000)))

static int test_runner_wounded_if_off_base_when_ball_caught() {
    // Setup: Runner on first, batter hits, runner advances but ball is caught.
    StateInfo* state = setup_test_state();
    setup_runner_at_first_base(state);
    
    const int runnerIndex = 0; // The runner is player 0 from the fixture
    const int fielderIndex = 14;

    // Batter hits
    state->localGameInfo->pII.batterIndex = 1;
    state->localGameInfo->playerInfo[1].bTPI.baseId = BASE_NONE;
    state->localGameInfo->pRAI.batHit = 1;
    state->localGameInfo->ballInfo.moving = 1;
    
    // Runner on 1st starts running towards 2nd
    state->localGameInfo->playerInfo[runnerIndex].bTPI.baseId = BASE_FIRST;
    state->localGameInfo->referee.battingPlayers[runnerIndex].baseAtPitchStart = BASE_FIRST;
    set_test_player_state(state, runnerIndex, PLAYER_STATE_RUNNING);
    
    // Move away from base 1
    state->localGameInfo->playerInfo[runnerIndex].tPI.location.x += 5.0f;
    
    // Ball is caught by a fielder
    state->localGameInfo->pII.hasBallIndex = fielderIndex;
    state->localGameInfo->gameControl.firstCatchMade = 1;

    // Now trigger the wounding catch
    unsigned int seed = 0;
    MenuInfo menu = {0}; // Dummy menu
    state->localGameInfo->referee.woundingCatchPending = 1;
    state->localGameInfo->referee.woundingCatchHandled = 0;

    for (int i = 0; i < 65; ++i) {
        gameAnalysis(state, &menu, &seed);
    }

    // Now process arrival logic to apply the wound
    state->localGameInfo->playerInfo[runnerIndex].bTPI.baseId = BASE_SECOND;
    state->localGameInfo->gameControl.playerArrivedToBase = 1;
    state->localGameInfo->playerRuntime[runnerIndex].arrivedToBase = 1;
    gameManipulation(state);

    int isWounded = (state->localGameInfo->playerInfo[runnerIndex].bTPI.state == PLAYER_STATE_WOUNDED);
    
    // VERIFY MOVEMENT: Wounded player should start walking off-field
    Vector3D initialLoc = state->localGameInfo->playerInfo[runnerIndex].tPI.location;
    
    // Simulate a few frames of movement
    // Note: gameManipulation updates location based on velocity
    for (int k = 0; k < 10; ++k) {
        state->localGameInfo->ballInfo.moving = 0; // Prevent ball logic interference
        gameManipulation(state);
    }
    
    float dist = geometry_distance_2d_xz(&initialLoc, &state->localGameInfo->playerInfo[runnerIndex].tPI.location);

    cleanup_test_state(state);
    ASSERT_EQ(1, isWounded, "Runner should be wounded when off base and ball is caught");
    ASSERT_TRUE(dist > 0.01f, "Wounded player should move away from base (failed to walk)");
    return TEST_PASSED;
}

static int test_runner_not_wounded_if_ball_hits_ground() {
    StateInfo* state = setup_test_state();
    setup_runner_at_first_base(state);
    
    const int runnerIndex = 0; 
    
    // Runner running between bases
    state->localGameInfo->playerInfo[runnerIndex].bTPI.baseId = BASE_SECOND;
    state->localGameInfo->referee.battingPlayers[runnerIndex].baseAtPitchStart = BASE_FIRST;
    set_test_player_state(state, runnerIndex, PLAYER_STATE_RUNNING);
    
    // Initial stabilization
    state->localGameInfo->gameControl.firstCatchMade = 1;
    unsigned int seed = 0;
    MenuInfo menu = {0};

    // Trigger wounding catch
    state->localGameInfo->referee.woundingCatchPending = 1;
    state->localGameInfo->referee.woundingCatchHandled = 0;
    
    // Run 1 frame to set woundingPlayersMarked = 1
    gameAnalysis(state, &menu, &seed);
    
    if (is_player_marked_for_wound(&state->localGameInfo->referee, runnerIndex) != 1) {
         cleanup_test_state(state);
         ASSERT_EQ(1, 0, "Runner should have been marked for wound initially");
    }

    // Now ball hits ground!
    state->localGameInfo->ballInfo.hitsGroundToUnWound = 1;
    
    // Run loop until threshold
    int steps = (int)WOUNDING_CATCH_THRESHOLD + 5;
    for (int i = 0; i < steps; ++i) {
        gameAnalysis(state, &menu, &seed);
    }

    // Verify NOT wounded
    int isWounded = (state->localGameInfo->playerInfo[runnerIndex].bTPI.state == PLAYER_STATE_WOUNDED);
    int isMarked = is_player_marked_for_wound(&state->localGameInfo->referee, runnerIndex);
    
    cleanup_test_state(state);
    ASSERT_EQ(0, isWounded, "Runner should NOT be wounded if ball hits ground");
    ASSERT_EQ(0, isMarked, "Wound marker should be cleared");
    return TEST_PASSED;
}

static int test_runner_not_wounded_if_starts_running_late() {
    StateInfo* state = setup_test_state();
    setup_runner_at_first_base(state);
    
    const int runnerIndex = 0; 
    
    // Runner is SAFE at original base
    state->localGameInfo->playerInfo[runnerIndex].bTPI.baseId = BASE_FIRST;
    state->localGameInfo->referee.battingPlayers[runnerIndex].baseAtPitchStart = BASE_FIRST;
    set_test_player_state(state, runnerIndex, PLAYER_STATE_ON_BASE);
    
    // Stabilization
    state->localGameInfo->gameControl.firstCatchMade = 1;
    unsigned int seed = 0;
    MenuInfo menu = {0};

    // Trigger wounding catch
    state->localGameInfo->referee.woundingCatchPending = 1;
    state->localGameInfo->referee.woundingCatchHandled = 0;
    
    // Run 1 frame. woundingPlayersMarked should be 0 because runner is safe.
    gameAnalysis(state, &menu, &seed);
    
    if (is_player_marked_for_wound(&state->localGameInfo->referee, runnerIndex) != 0) {
         cleanup_test_state(state);
         ASSERT_EQ(0, 1, "Runner should NOT be marked for wound while safe");
    }

    // Now runner starts running!
    state->localGameInfo->playerInfo[runnerIndex].bTPI.baseId = BASE_SECOND; // Moved towards 2nd (conceptually)
    set_test_player_state(state, runnerIndex, PLAYER_STATE_RUNNING);
    
    // Run loop until threshold
    int steps = (int)WOUNDING_CATCH_THRESHOLD + 5;
    for (int i = 0; i < steps; ++i) {
        gameAnalysis(state, &menu, &seed);
    }

    // Verify NOT wounded
    int isWounded = (state->localGameInfo->playerInfo[runnerIndex].bTPI.state == PLAYER_STATE_WOUNDED);
    
    cleanup_test_state(state);
    ASSERT_EQ(0, isWounded, "Runner should NOT be wounded if they started running after the catch logic check");
    return TEST_PASSED;
}

void run_scenario_wounded_tests() {
    RUN_TEST(test_runner_wounded_if_off_base_when_ball_caught);
    RUN_TEST(test_runner_not_wounded_if_ball_hits_ground);
    RUN_TEST(test_runner_not_wounded_if_starts_running_late);
}
