#include "test_scenario_chain_reaction.h"
#include "fixtures.h"
#include "../test_helpers.h"
#include "game_setup.h"
#include "common_logic.h"
#include "rules_outs.h"
#include "game_analysis.h"
#include "base_logic.h"

/**
 * Scenario: Bases Loaded (Ajolähtö)
 * Batter hits, ball goes to second base.
 * Runner from first base should be OUT.
 */
static int test_ajolahto_force_out_at_second_scenario() {
    StateInfo* state = setup_test_state();
    unsigned int seed = 0;
    GameSetup setup = {0};
    setup.launchType = GAME_LAUNCH_NEW;
    setup.gameMode = GAME_MODE_NORMAL;
    setup.team1 = 0;
    setup.team2 = 1;
    setup.team1_control = 0;
    setup.team2_control = 2;
    setup.halfInningsInPeriod = 4;
    setup.playsFirst = 0;

    initializeGameFromMenu(state, &setup, &seed);
    loadMutableWorldSettings(state, &seed);

    // Reset batting team on field indices to be sure
    for(int k=0; k<BASE_COUNT; k++) state->localGameInfo->pII.battingTeamOnFieldIndices[k] = -1;
    
    // Setup Ajolähtö (Bases Loaded)
    // Runner on 3rd
    int r3 = 1;
    state->localGameInfo->playerInfo[r3].bTPI.baseId = BASE_THIRD;
    state->localGameInfo->playerInfo[r3].bTPI.originalBase = BASE_THIRD;
    state->localGameInfo->pII.safeOnBaseIndex[3] = r3;
    state->localGameInfo->pII.battingTeamOnFieldIndices[0] = r3;
    
    // Runner on 2nd
    int r2 = 2;
    state->localGameInfo->playerInfo[r2].bTPI.baseId = BASE_SECOND;
    state->localGameInfo->playerInfo[r2].bTPI.originalBase = BASE_SECOND;
    state->localGameInfo->pII.safeOnBaseIndex[2] = r2;
    state->localGameInfo->pII.battingTeamOnFieldIndices[1] = r2;
    
    // Runner on 1st
    int r1 = 3;
    state->localGameInfo->playerInfo[r1].bTPI.baseId = BASE_FIRST;
    state->localGameInfo->playerInfo[r1].bTPI.originalBase = BASE_FIRST;
    state->localGameInfo->pII.safeOnBaseIndex[1] = r1;
    state->localGameInfo->pII.battingTeamOnFieldIndices[2] = r1;
    
    // Batter at home
    int batter = 0;
    state->localGameInfo->pII.batterIndex = batter;
    state->localGameInfo->playerInfo[batter].bTPI.baseId = BASE_HOME;
    state->localGameInfo->playerInfo[batter].bTPI.originalBase = BASE_HOME;
    state->localGameInfo->pII.safeOnBaseIndex[0] = batter;
    state->localGameInfo->pII.battingTeamOnFieldIndices[3] = batter;
    
    state->localGameInfo->playerCounters.battingTeamPlayersOnFieldCount = 4;

    // Simulate a hit
    state->localGameInfo->pRAI.batHit = 1;
    state->localGameInfo->ballInfo.moving = 1;
    
    // All runners start moving
    set_test_player_state(state, r1, PLAYER_STATE_RUNNING);
    set_test_player_state(state, r2, PLAYER_STATE_RUNNING);
    set_test_player_state(state, r3, PLAYER_STATE_RUNNING);
    set_test_player_state(state, batter, PLAYER_STATE_RUNNING);

    // Ball reaches Second Base (i=2)
    int catcherIndex = state->localGameInfo->pII.catcherOnBaseIndex[2];
    state->localGameInfo->ballInfo.location = state->localGameInfo->playerInfo[catcherIndex].tPI.homeLocation;
    state->localGameInfo->pII.hasBallIndex = catcherIndex; 
    
    // Check for outs
    MenuInfo menu = {0};
    gameAnalysis(state, &menu, NULL);

    int outs = state->localGameInfo->gameState.outs;
    cleanup_test_state(state);
    
    ASSERT_EQ(1, outs, "Runner from first should be forced out at second base in ajolähtö");
    return TEST_PASSED;
}

/**
 * Scenario: Runner on Second ONLY.
 * Batter hits, ball goes to third base.
 * Runner from second should NOT be forced out (because they aren't forced to move).
 */
static int test_no_force_chain_scenario() {
    StateInfo* state = setup_test_state();
    
    // Runner on 2nd
    int r2 = 2;
    state->localGameInfo->playerInfo[r2].bTPI.baseId = BASE_SECOND;
    state->localGameInfo->pII.safeOnBaseIndex[2] = r2;
    state->localGameInfo->pII.battingTeamOnFieldIndices[0] = r2;
    
    // Batter at home
    int batter = 0;
    state->localGameInfo->pII.batterIndex = batter;
    state->localGameInfo->playerInfo[batter].bTPI.baseId = BASE_HOME;
    state->localGameInfo->pII.safeOnBaseIndex[0] = batter;
    state->localGameInfo->pII.battingTeamOnFieldIndices[1] = batter;
    
    state->localGameInfo->playerCounters.battingTeamPlayersOnFieldCount = 2;

    // Simulate a hit
    state->localGameInfo->pRAI.batHit = 1;
    state->localGameInfo->ballInfo.moving = 1;
    
    // Runner 2 leads
    set_test_player_state(state, r2, PLAYER_STATE_RUNNING);
    set_test_player_state(state, batter, PLAYER_STATE_RUNNING);

    // Ball reaches Third Base (i=3)
    state->localGameInfo->ballInfo.location = state->fieldPositions->thirdBase;
    state->localGameInfo->pII.hasBallIndex = 15; // Give ball to third baseman
    
    // Check for outs
    MenuInfo menu = {0};
    gameAnalysis(state, &menu, NULL);

    int outs = state->localGameInfo->gameState.outs;
    cleanup_test_state(state);
    
    ASSERT_EQ(0, outs, "Runner from second should NOT be forced out at third base if first base is empty");
    return TEST_PASSED;
}

void run_scenario_chain_reaction_tests() {
    RUN_TEST(test_ajolahto_force_out_at_second_scenario);
    RUN_TEST(test_no_force_chain_scenario);
}
