#include "test_scenario_tuplahaava.h"
#include "fixtures.h"
#include "../test_helpers.h"
#include "game_setup.h"
#include "common_logic.h"
#include "game_analysis.h"
#include "base_logic.h"

/**
 * Scenario: Tuplahaava (§36)
 * Runner A is on 1st, Runner B is on 2nd.
 * Fly ball caught.
 * Runner A was off-base (irti) -> marked for wound.
 * Runner B stayed on-base -> NOT marked for wound.
 * Runner A reaches 2nd base where Runner B is.
 * Result: BOTH should be wounded (Double Wound).
 */
static int test_tuplahaava_collision_scenario() {
    StateInfo* state = setup_test_state();
    unsigned int seed = 0;
    GameSetup setup = {0};
    setup.launchType = GAME_LAUNCH_NEW;
    setup.gameMode = GAME_MODE_NORMAL;
    setup.team1 = 0;
    setup.team2 = 1;
    initializeGameFromMenu(state, &setup, &seed);
    loadMutableWorldSettings(state, &seed);

    // Runner B on 2nd (Index 1)
    int runnerB = 1;
    state->localGameInfo->playerInfo[runnerB].bTPI.baseId = BASE_SECOND;
    state->localGameInfo->playerInfo[runnerB].bTPI.originalBase = BASE_SECOND;
    state->localGameInfo->pII.safeOnBaseIndex[2] = runnerB;
    state->localGameInfo->pII.battingTeamOnFieldIndices[0] = runnerB;
    set_test_player_state(state, runnerB, PLAYER_STATE_SAFE_ON_BASE);

    // Runner A on 1st (Index 0)
    int runnerA = 0;
    state->localGameInfo->playerInfo[runnerA].bTPI.baseId = BASE_FIRST;
    state->localGameInfo->playerInfo[runnerA].bTPI.originalBase = BASE_FIRST;
    state->localGameInfo->pII.safeOnBaseIndex[1] = runnerA;
    state->localGameInfo->pII.battingTeamOnFieldIndices[1] = runnerA;
    
    // Runner A is OFF-BASE when catch happens
    set_test_player_state(state, runnerA, PLAYER_STATE_RUNNING);
    state->localGameInfo->playerInfo[runnerA].tPI.location.x += 5.0f; // Move away from base

    // 1. Trigger Wounding Catch
    state->localGameInfo->woundingState.woundingCatch = 1;
    state->localGameInfo->pRAI.batHit = 1;
    
    MenuInfo menu = {0};
    gameAnalysis(state, &menu, NULL); // Marks runnerA as woundedApply=1

    // 2. Simulate timer expiry in gameAnalysis
    // Threshold is about 1 second. We'll set counter to be sure.
    state->localGameInfo->gameFlowState.woundingCatchCounter = 1000; 
    gameAnalysis(state, &menu, NULL); // Marks runnerA as pendingWound=1

    ASSERT_EQ(1, state->localGameInfo->playerRuntime[runnerA].pendingWound, "Runner A should have pending wound");
    ASSERT_EQ(PLAYER_STATE_RUNNING, state->localGameInfo->playerInfo[runnerA].bTPI.state, "Runner A should still be RUNNING (not WOUNDED yet)");

    // 3. Process arrival in game_manipulation
    state->localGameInfo->playerInfo[runnerA].bTPI.baseId = BASE_SECOND; // He has arrived
    state->localGameInfo->gameControl.playerArrivedToBase = 1;
    state->localGameInfo->playerRuntime[runnerA].arrivedToBase = 1;
    
    extern void gameManipulation(StateInfo* stateInfo); 
    gameManipulation(state);

    // Check results
    // Both should be removed from field indices
    int foundA = 0, foundB = 0;
    for(int k=0; k<4; k++) {
        if(state->localGameInfo->pII.battingTeamOnFieldIndices[k] == runnerA) foundA = 1;
        if(state->localGameInfo->pII.battingTeamOnFieldIndices[k] == runnerB) foundB = 1;
    }

    int outs = state->localGameInfo->gameState.outs;
    cleanup_test_state(state);
    
    ASSERT_EQ(0, foundA, "Runner A should be removed from field (wounded)");
    ASSERT_EQ(0, foundB, "Runner B should be removed from field (double wounded)");
    ASSERT_EQ(0, outs, "Wounds should not count as outs");
    
    return TEST_PASSED;
}

/**
 * Scenario: Runner B is LEADING from 2nd base.
 * Runner A (pending wound) arrives at 2nd base.
 * Result: BOTH wounded eventually. Runner B is marked pendingWound when A arrives.
 */
static int test_tuplahaava_collision_if_leading_scenario() {
    StateInfo* state = setup_test_state();
    unsigned int seed = 0;
    GameSetup setup = {0};
    setup.launchType = GAME_LAUNCH_NEW;
    setup.gameMode = GAME_MODE_NORMAL;
    setup.team1 = 0;
    setup.team2 = 1;
    initializeGameFromMenu(state, &setup, &seed);
    loadMutableWorldSettings(state, &seed);

    // Runner B on 2nd (Index 1) - LEADING
    int runnerB = 1;
    state->localGameInfo->playerInfo[runnerB].bTPI.baseId = BASE_SECOND;
    state->localGameInfo->pII.safeOnBaseIndex[2] = runnerB;
    state->localGameInfo->pII.battingTeamOnFieldIndices[0] = runnerB;
    set_test_player_state(state, runnerB, PLAYER_STATE_LEADING);

    // Runner A on 1st (Index 0) - Pending Wound
    int runnerA = 0;
    state->localGameInfo->playerInfo[runnerA].bTPI.baseId = BASE_FIRST;
    state->localGameInfo->pII.battingTeamOnFieldIndices[1] = runnerA;
    state->localGameInfo->playerRuntime[runnerA].pendingWound = 1;
    set_test_player_state(state, runnerA, PLAYER_STATE_RUNNING);

    // Process arrival of A at base 2
    state->localGameInfo->playerInfo[runnerA].bTPI.baseId = BASE_SECOND; 
    state->localGameInfo->gameControl.playerArrivedToBase = 1;
    state->localGameInfo->playerRuntime[runnerA].arrivedToBase = 1;
    
    extern void gameManipulation(StateInfo* stateInfo); 
    gameManipulation(state);

    // Check results after arrival of A
    ASSERT_EQ(1, state->localGameInfo->playerRuntime[runnerB].pendingWound, "Runner B should be marked for double wound because A arrived at their base first");
    
    // Now Runner B arrives at base 3
    state->localGameInfo->playerInfo[runnerB].bTPI.baseId = BASE_THIRD;
    state->localGameInfo->gameControl.playerArrivedToBase = 1;
    state->localGameInfo->playerRuntime[runnerB].arrivedToBase = 1;
    gameManipulation(state);

    // Check final results
    int foundA = 0, foundB = 0;
    for(int k=0; k<4; k++) {
        if(state->localGameInfo->pII.battingTeamOnFieldIndices[k] == runnerA) foundA = 1;
        if(state->localGameInfo->pII.battingTeamOnFieldIndices[k] == runnerB) foundB = 1;
    }

    cleanup_test_state(state);
    
    ASSERT_EQ(0, foundA, "Runner A should be wounded");
    ASSERT_EQ(0, foundB, "Runner B should be wounded upon arrival at base 3 due to pending double wound");
    
    return TEST_PASSED;
}

/**
 * Scenario: Runner A has a pending wound.
 * Foul play (out of bounds) occurs.
 * Runner A arrives at base.
 * Result: Runner A should NOT be wounded because foul play resets temporary states.
 */
static int test_foul_play_resets_pending_wound_scenario() {
    StateInfo* state = setup_test_state();
    unsigned int seed = 0;
    GameSetup setup = {0};
    setup.launchType = GAME_LAUNCH_NEW;
    setup.gameMode = GAME_MODE_NORMAL;
    setup.team1 = 0;
    setup.team2 = 1;
    initializeGameFromMenu(state, &setup, &seed);
    loadMutableWorldSettings(state, &seed);

    int runnerA = 0;
    state->localGameInfo->pII.battingTeamOnFieldIndices[0] = runnerA;
    state->localGameInfo->playerRuntime[runnerA].pendingWound = 1;
    set_test_player_state(state, runnerA, PLAYER_STATE_RUNNING);

    // Trigger Foul Play
    state->localGameInfo->gameState.outOfBounds = 1;
    state->localGameInfo->gameFlowState.outOfBoundsCounter = 1000; // Force immediate reset
    
    MenuInfo menu = {0};
    gameAnalysis(state, &menu, &seed); // This calls foulPlay() which calls initializeTemporaryGameAnalysisInfo()

    ASSERT_EQ(0, state->localGameInfo->playerRuntime[runnerA].pendingWound, "pendingWound should be reset by foul play");

    cleanup_test_state(state);
    return TEST_PASSED;
}

void run_scenario_tuplahaava_tests() {
    RUN_TEST(test_tuplahaava_collision_scenario);
    RUN_TEST(test_tuplahaava_collision_if_leading_scenario);
    RUN_TEST(test_foul_play_resets_pending_wound_scenario);
}
