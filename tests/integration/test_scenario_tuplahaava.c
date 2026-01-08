#include "test_scenario_tuplahaava.h"
#include "fixtures.h"
#include "../test_helpers.h"
#include "game_setup.h"
#include "common_logic.h"
#include "game_analysis.h"
#include "base_logic.h"

static void setup_tuplahaava_scenario(StateInfo* state) {
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
    state->localGameInfo->referee.battingPlayers[runnerB].baseAtPitchStart = BASE_SECOND;
    state->localGameInfo->referee.battingPlayers[runnerB].hadSafetyAtPitchStart = 1;
    state->localGameInfo->referee.battingPlayers[runnerB].currentSafetyBase = BASE_SECOND;
    set_test_player_state(state, runnerB, PLAYER_STATE_SAFE_ON_BASE);

    // Runner A on 1st (Index 0)
    int runnerA = 0;
    state->localGameInfo->playerInfo[runnerA].bTPI.baseId = BASE_FIRST;
    state->localGameInfo->referee.battingPlayers[runnerA].baseAtPitchStart = BASE_FIRST;
    state->localGameInfo->referee.battingPlayers[runnerA].hadSafetyAtPitchStart = 1;
    state->localGameInfo->referee.battingPlayers[runnerA].currentSafetyBase = BASE_FIRST;
    
    // Runner A is OFF-BASE when catch happens
    set_test_player_state(state, runnerA, PLAYER_STATE_RUNNING);
    state->localGameInfo->playerInfo[runnerA].tPI.location.x += 5.0f; // Move away from base
}

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
    setup_tuplahaava_scenario(state);
    
    // Runner A (Index 0) and Runner B (Index 1) are set up by helper
    int runnerA = 0;
    int runnerB = 1;

    // 1. Trigger Wounding Catch
    state->localGameInfo->referee.woundingCatchPending = 1;
    state->localGameInfo->pRAI.batHit = 1;
    
    MenuInfo menu = {0};
    gameAnalysis(state, &menu, NULL); // Marks runnerA as woundingPlayersMarked[runnerA]=1

    // 2. Simulate timer expiry in gameAnalysis
    // Threshold is about 1 second. We'll set counter to be sure.
    state->localGameInfo->referee.woundingCatchTimer = 1000; 
    gameAnalysis(state, &menu, NULL); // Marks runnerA as pendingWound=1 in referee

    ASSERT_EQ(1, state->localGameInfo->referee.battingPlayers[runnerA].hasPendingWound, "Runner A should have pending wound in referee");
    ASSERT_EQ(PLAYER_STATE_RUNNING, state->localGameInfo->playerInfo[runnerA].bTPI.state, "Runner A should still be RUNNING (not WOUNDED yet)");

    // 3. Process arrival in game_manipulation
    state->localGameInfo->playerInfo[runnerA].bTPI.baseId = BASE_SECOND; // He has arrived
    state->localGameInfo->gameControl.playerArrivedToBase = 1;
    state->localGameInfo->playerRuntime[runnerA].arrivedToBase = 1;
    
    extern void gameManipulation(StateInfo* stateInfo); 
    gameManipulation(state);

    // Check results - Both runners should be wounded
    ASSERT_EQ(PLAYER_STATE_WOUNDED, state->localGameInfo->playerInfo[runnerA].bTPI.state, "Runner A should be wounded");
    ASSERT_EQ(PLAYER_STATE_WOUNDED, state->localGameInfo->playerInfo[runnerB].bTPI.state, "Runner B should be wounded (double wounded)");

    int outs = state->localGameInfo->gameState.outs;
    cleanup_test_state(state);
    
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
    // CRITICAL: Update Referee State for get_base_controller
    state->localGameInfo->referee.battingPlayers[runnerB].currentSafetyBase = BASE_SECOND;
    set_test_player_state(state, runnerB, PLAYER_STATE_LEADING);

    // Runner A on 1st (Index 0) - Pending Wound
    int runnerA = 0;
    state->localGameInfo->playerInfo[runnerA].bTPI.baseId = BASE_FIRST;
    state->localGameInfo->referee.battingPlayers[runnerA].hasPendingWound = 1;
    state->localGameInfo->referee.battingPlayers[runnerA].woundingType = WOUNDING_TYPE_NORMAL;
    state->localGameInfo->referee.battingPlayers[runnerA].woundingSourceBase = BASE_FIRST;
    set_test_player_state(state, runnerA, PLAYER_STATE_RUNNING);

    // Process arrival of A at base 2
    state->localGameInfo->playerInfo[runnerA].bTPI.baseId = BASE_SECOND; 
    state->localGameInfo->gameControl.playerArrivedToBase = 1;
    state->localGameInfo->playerRuntime[runnerA].arrivedToBase = 1;
    
    extern void gameManipulation(StateInfo* stateInfo); 
    gameManipulation(state);

    // Check results after arrival of A
    ASSERT_EQ(1, state->localGameInfo->referee.battingPlayers[runnerB].hasPendingWound, "Runner B should be marked for double wound because A arrived at their base first");
    ASSERT_EQ(WOUNDING_TYPE_TUPLAHAAVA, state->localGameInfo->referee.battingPlayers[runnerB].woundingType, "Should be tuplahaava type");
    
    // Now Runner B arrives at base 3
    state->localGameInfo->playerInfo[runnerB].bTPI.baseId = BASE_THIRD;
    state->localGameInfo->gameControl.playerArrivedToBase = 1;
    state->localGameInfo->playerRuntime[runnerB].arrivedToBase = 1;
    set_test_player_state(state, runnerB, PLAYER_STATE_SAFE_ON_BASE); // Simulate physical arrival
    gameManipulation(state);

    // Check final results - both should be wounded
    ASSERT_EQ(PLAYER_STATE_WOUNDED, state->localGameInfo->playerInfo[runnerA].bTPI.state, "Runner A should be wounded");
    ASSERT_EQ(PLAYER_STATE_WOUNDED, state->localGameInfo->playerInfo[runnerB].bTPI.state, "Runner B should be wounded upon arrival at base 3 due to pending double wound");

    cleanup_test_state(state);
    
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
    state->localGameInfo->referee.battingPlayers[runnerA].hasPendingWound = 1;
    state->localGameInfo->referee.battingPlayers[runnerA].baseAtPitchStart = BASE_FIRST; // Needed for foul play restore
    set_test_player_state(state, runnerA, PLAYER_STATE_RUNNING);

    // Trigger Foul Play
    state->localGameInfo->gameState.outOfBounds = 1;
    state->localGameInfo->gameFlowState.outOfBoundsCounter = 1000; // Force immediate reset
    
    MenuInfo menu = {0};
    gameAnalysis(state, &menu, &seed); // This calls foulPlay() which calls initializeTemporaryGameAnalysisInfo()

    ASSERT_EQ(0, state->localGameInfo->referee.battingPlayers[runnerA].hasPendingWound, "pendingWound should be reset by foul play");
    
    cleanup_test_state(state);
    return TEST_PASSED;
}

/**
 * Scenario: Tuplahaava Exception 1 (§36).
 * Runner B (victim) is in-between bases (running).
 * Ball reaches SOURCE base (where they had safety).
 * Result: Safety is revoked. Wounding type becomes NORMAL (must advance).
 */
static int test_tuplahaava_exception_1_loss_of_safety() {
    StateInfo* state = setup_test_state();
    
    // Declare setup helper locally or rely on its definition if it's static in this file.
    // It seems setup_tuplahaava_scenario was defined earlier in the file?
    // Let's check the file content. If it's defined after, we need a forward decl.
    // Assuming it's defined before, checking why it failed. 
    // Ah, previous tests used it.
    
    extern void setup_tuplahaava_scenario(StateInfo* state);
    setup_tuplahaava_scenario(state);
    
    int runnerB = 1; // Victim on 2nd

    // 1. Trigger Tuplahaava
    // Runner A arrives at 2nd
    state->localGameInfo->playerInfo[0].bTPI.baseId = BASE_SECOND;
    state->localGameInfo->referee.battingPlayers[0].hasPendingWound = 1;
    state->localGameInfo->referee.battingPlayers[0].woundingType = WOUNDING_TYPE_NORMAL;
    state->localGameInfo->referee.battingPlayers[0].woundingSourceBase = BASE_FIRST;
    state->localGameInfo->gameControl.playerArrivedToBase = 1;
    state->localGameInfo->playerRuntime[0].arrivedToBase = 1;
    
    // Ensure Runner B is NOT safe on base so they get a PENDING wound
    set_test_player_state(state, runnerB, PLAYER_STATE_LEADING);

    extern void gameManipulation(StateInfo* stateInfo);
    gameManipulation(state); // Applies Tuplahaava to Runner B

    // Verify initial Tuplahaava state
    ASSERT_EQ(1, state->localGameInfo->referee.battingPlayers[runnerB].hasPendingWound, "Runner B should have pending wound");
    ASSERT_EQ(WOUNDING_TYPE_TUPLAHAAVA, state->localGameInfo->referee.battingPlayers[runnerB].woundingType, "Should be TUPLAHAAVA");
    ASSERT_EQ(BASE_SECOND, state->localGameInfo->referee.battingPlayers[runnerB].currentSafetyBase, "Should still have safety at 2nd");

    // 2. Runner B moves "in-between" (RUNNING away from base)
    set_test_player_state(state, runnerB, PLAYER_STATE_RUNNING);
    
    // 3. Ball thrown to SOURCE base (2nd base, index 2)
    state->localGameInfo->pII.hasBallIndex = 14; 
    state->localGameInfo->pII.catcherOnBaseIndex[2] = 14; 
    
    // Set ball location to 2nd base location
    state->localGameInfo->ballInfo.location.x = state->fieldPositions->secondBase.x;
    state->localGameInfo->ballInfo.location.y = state->fieldPositions->secondBase.y;
    state->localGameInfo->ballInfo.location.z = state->fieldPositions->secondBase.z;
    state->localGameInfo->ballInfo.moving = 0; // Ball arrived
    
    // Run Analysis
    MenuInfo menu = {0};
    gameAnalysis(state, &menu, NULL);

    // Verify Safety Loss
    ASSERT_EQ(BASE_NONE, state->localGameInfo->referee.battingPlayers[runnerB].currentSafetyBase, "Safety should be revoked");
    ASSERT_EQ(WOUNDING_TYPE_NORMAL, state->localGameInfo->referee.battingPlayers[runnerB].woundingType, "Should transition to NORMAL wound");
    
    cleanup_test_state(state);
    return TEST_PASSED;
}

void run_scenario_tuplahaava_tests() {
    RUN_TEST(test_tuplahaava_collision_scenario);
    RUN_TEST(test_tuplahaava_collision_if_leading_scenario);
    RUN_TEST(test_foul_play_resets_pending_wound_scenario);
    RUN_TEST(test_tuplahaava_exception_1_loss_of_safety);
}
