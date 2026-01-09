#include "test_helpers.h"
#include "state_validator.h"
#include "referee.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static StateInfo state;
static LocalGameInfo game;

int test_debug_logging_cycle() {
    // Setup
    memset(&game, 0, sizeof(LocalGameInfo));
    state.localGameInfo = &game;
    
    // Properly initialize referee state (bases to BASE_NONE)
    initializeRefereeState(&game.referee);
    
    // Test File Path
    const char* dumpPath = "test_dump.json";
    remove(dumpPath); // cleanup before test

    // Init Validator
    StateValidator_Init(dumpPath);

    // 1. Capture a clean snapshot
    game.gameState.outs = 1;
    StateValidator_CaptureSnapshot(&state, "SNAPSHOT_1");

    // 2. Advance state
    game.gameState.outs = 2;
    StateValidator_CaptureSnapshot(&state, "SNAPSHOT_2");

    // 3. Induce Fatal State (Controller Mismatch)
    // Base 1 controlled by Player 0, but Player 0 is physically at Base 2
    game.referee.battingPlayers[0].currentSafetyBase = BASE_FIRST; // Player 0 owns Base 1
    game.playerInfo[0].bTPI.baseId = BASE_SECOND; // But is physically at Base 2
    game.playerInfo[0].bTPI.state = PLAYER_STATE_ON_BASE;

    // 4. Run Check (should return 0)
    int isValid = StateValidator_Check(&state);
    ASSERT_EQ(0, isValid, "State should be invalid");

    if (!isValid) {
        StateValidator_Dump(&state, "Base Controller Mismatch");
        game.gameControl.pause = 1;
    }

    // 5. Verify Pause
    ASSERT_EQ(1, game.gameControl.pause, "Game should be paused manually after check");

    // 6. Verify File Exists
    FILE* f = fopen(dumpPath, "r");
    ASSERT_TRUE(f != NULL, "Dump file should exist");
    if (f) {
        // Simple content check
        char buffer[65536]; // Snapshots make it larger
        size_t n = fread(buffer, 1, sizeof(buffer)-1, f);
        buffer[n] = 0;
        fclose(f);

        ASSERT_TRUE(strstr(buffer, "\"failure_reason\": \"Base Controller Mismatch\"") != NULL, "Reason match");
        ASSERT_TRUE(strstr(buffer, "\"label\": \"SNAPSHOT_1\"") != NULL, "Snapshot 1 found");
        ASSERT_TRUE(strstr(buffer, "\"label\": \"SNAPSHOT_2\"") != NULL, "Snapshot 2 found");
        ASSERT_TRUE(strstr(buffer, "\"ref_safetyBaseStr\": \"1ST\"") != NULL, "Snapshot should contain base strings");
    }

    // Cleanup
    remove(dumpPath);
    return TEST_PASSED;
}
