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

    int r3 = 0;
    int r2 = 1;
    int r1 = 2;
    int batter = 3;

    // Runner on 3rd
    state->localGameInfo->playerInfo[r3].bTPI.baseId = BASE_THIRD;
    state->localGameInfo->referee.battingPlayers[r3].baseAtPitchStart = BASE_THIRD;
    state->localGameInfo->referee.battingPlayers[r3].currentSafetyBase = BASE_THIRD;
    set_test_player_state(state, r3, PLAYER_STATE_SAFE_ON_BASE);

    // Runner on 2nd
    state->localGameInfo->playerInfo[r2].bTPI.baseId = BASE_SECOND;
    state->localGameInfo->referee.battingPlayers[r2].baseAtPitchStart = BASE_SECOND;
    state->localGameInfo->referee.battingPlayers[r2].currentSafetyBase = BASE_SECOND;
    set_test_player_state(state, r2, PLAYER_STATE_SAFE_ON_BASE);

    // Runner on 1st
    state->localGameInfo->playerInfo[r1].bTPI.baseId = BASE_FIRST;
    state->localGameInfo->referee.battingPlayers[r1].baseAtPitchStart = BASE_FIRST;
    state->localGameInfo->referee.battingPlayers[r1].currentSafetyBase = BASE_FIRST;
    set_test_player_state(state, r1, PLAYER_STATE_SAFE_ON_BASE);

    // Batter
    state->localGameInfo->pII.batterIndex = batter;
    state->localGameInfo->playerInfo[batter].bTPI.baseId = BASE_HOME;
    state->localGameInfo->referee.battingPlayers[batter].baseAtPitchStart = BASE_HOME;
    state->localGameInfo->referee.battingPlayers[batter].currentSafetyBase = BASE_HOME;
    
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
    state->localGameInfo->referee.battingPlayers[r2].currentSafetyBase = BASE_SECOND;
    
    // Batter at home
    int batter = 0;
    state->localGameInfo->pII.batterIndex = batter;
    state->localGameInfo->playerInfo[batter].bTPI.baseId = BASE_HOME;
    state->localGameInfo->referee.battingPlayers[batter].currentSafetyBase = BASE_HOME;
    
    state->localGameInfo->playerCounters.battingTeamPlayersOnFieldCount = 2;

    // Simulate a hit
    state->localGameInfo->pRAI.batHit = 1;
    state->localGameInfo->ballInfo.moving = 1;
    
    // Runner 2 stays safe (not forced to run)
    set_test_player_state(state, r2, PLAYER_STATE_SAFE_ON_BASE);
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
