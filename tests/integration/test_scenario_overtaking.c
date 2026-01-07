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

    // Runner A on 3rd
    int runnerA = 0;
    state->localGameInfo->playerInfo[runnerA].bTPI.baseId = BASE_THIRD;
    state->localGameInfo->referee.battingPlayers[runnerA].baseAtPitchStart = BASE_THIRD;
    state->localGameInfo->referee.battingPlayers[runnerA].currentSafetyBase = BASE_THIRD;
    set_test_player_state(state, runnerA, PLAYER_STATE_SAFE_ON_BASE);

    // Batter B
    int batterB = 1;
    state->localGameInfo->pII.batterIndex = batterB;
    state->localGameInfo->playerInfo[batterB].bTPI.baseId = BASE_HOME;
    state->localGameInfo->referee.battingPlayers[batterB].baseAtPitchStart = BASE_HOME;
    state->localGameInfo->referee.battingPlayers[batterB].currentSafetyBase = BASE_HOME;
    set_test_player_state(state, batterB, PLAYER_STATE_AT_BAT);
    
    // Batter B arrives at 3rd base
    state->localGameInfo->playerInfo[batterB].bTPI.baseId = BASE_THIRD;
    state->localGameInfo->gameControl.playerArrivedToBase = 1;
    state->localGameInfo->playerRuntime[batterB].arrivedToBase = 1;
    state->localGameInfo->ballInfo.hasHitGround = 1; // Required for checkForRuns to proceed
    state->localGameInfo->gameModeState.canMakeRunOfHonor = 1; // Required for Kunniajuoksu

    // Process arrival
    extern void gameManipulation(StateInfo* stateInfo); 
    gameManipulation(state);
    
    MenuInfo menu = {0};
    gameAnalysis(state, &menu, &seed);

    // Check results
    int runs = state->localGameInfo->gameState.runsInTheInning;
    
    // Batter should have scored (baseId should be HOME_SCORED or similar)
    ASSERT_EQ(1, runs, "Run of honor should be scored");
    
    // Runner A should still be on field at 3rd base
    ASSERT_EQ(BASE_THIRD, state->localGameInfo->playerInfo[runnerA].bTPI.baseId, "Runner A should remain on 3rd base");
    
    cleanup_test_state(state);
    
    return TEST_PASSED;
}

void run_scenario_overtaking_tests() {
    RUN_TEST(test_run_of_honor_overtaking_scenario);
}
