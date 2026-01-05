#include "test_scenario_overtaking.h"
#include "fixtures.h"
#include "../test_helpers.h"
#include "game_setup.h"
#include "common_logic.h"
#include "game_analysis.h"
#include "base_logic.h"

/**
 * Scenario: §42 Kunniajuoksu Overtaking.
 * Runner A is safe on 3rd base.
 * Batter B hits a kunnari and arrives at 3rd base.
 * Result: Batter B scores a run and is removed from field. Runner A stays safe on 3rd.
 */
static int test_run_of_honor_overtaking_scenario() {
    StateInfo* state = setup_test_state();
    unsigned int seed = 0;
    GameSetup setup = {0};
    setup.launchType = GAME_LAUNCH_NEW;
    setup.gameMode = GAME_MODE_NORMAL;
    setup.team1 = 0;
    setup.team2 = 1;
    setup.halfInningsInPeriod = 4;
    initializeGameFromMenu(state, &setup, &seed);
    loadMutableWorldSettings(state, &seed);

    // Runner A on 3rd (Index 1)
    int runnerA = 1;
    state->localGameInfo->playerInfo[runnerA].bTPI.baseId = BASE_THIRD;
    state->localGameInfo->playerInfo[runnerA].bTPI.originalBase = BASE_THIRD;
    state->localGameInfo->pII.safeOnBaseIndex[3] = runnerA;
    state->localGameInfo->pII.battingTeamOnFieldIndices[0] = runnerA;
    set_test_player_state(state, runnerA, PLAYER_STATE_SAFE_ON_BASE);

    // Batter B (Index 0) hits kunnari
    int batterB = 0;
    state->localGameInfo->pII.batterIndex = batterB;
    state->localGameInfo->playerInfo[batterB].bTPI.baseId = BASE_HOME;
    state->localGameInfo->playerInfo[batterB].bTPI.originalBase = BASE_HOME;
    state->localGameInfo->pII.battingTeamOnFieldIndices[1] = batterB;
    state->localGameInfo->gameModeState.canMakeRunOfHonor = 1;
    
    // Batter B arrives at 3rd base
    state->localGameInfo->playerInfo[batterB].bTPI.baseId = BASE_THIRD;
    state->localGameInfo->gameControl.playerArrivedToBase = 1;
    state->localGameInfo->playerRuntime[batterB].arrivedToBase = 1;
    state->localGameInfo->ballInfo.hasHitGround = 1; // Required for checkForRuns to proceed

    // Process arrival
    extern void gameManipulation(StateInfo* stateInfo); 
    gameManipulation(state);
    
    MenuInfo menu = {0};
    gameAnalysis(state, &menu, &seed);

    // Check results
    // Batter B should have scored and been removed (because they cannot stay on 3rd if occupied)
    int batterFound = 0;
    int runnerFound = 0;
    for(int k=0; k<4; k++) {
        if(state->localGameInfo->pII.battingTeamOnFieldIndices[k] == batterB) batterFound = 1;
        if(state->localGameInfo->pII.battingTeamOnFieldIndices[k] == runnerA) runnerFound = 1;
    }
    
    int runs = state->localGameInfo->gameState.runsInTheInning;
    
    cleanup_test_state(state);
    
    ASSERT_EQ(1, runs, "Run of honor should be scored");
    ASSERT_EQ(0, batterFound, "Batter should be removed from field after overtaking on kunnari");
    ASSERT_EQ(1, runnerFound, "Runner A should remain on field");
    
    return TEST_PASSED;
}

void run_scenario_overtaking_tests() {
    RUN_TEST(test_run_of_honor_overtaking_scenario);
}
