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

    // 3. Force a dump manually (don't try to enumerate specific invalid states)
    StateValidator_Dump(&state, "Test Manual Dump");

    // 4. Verify File Exists and contains expected JSON structure
    FILE* f = fopen(dumpPath, "r");
    ASSERT_TRUE(f != NULL, "Dump file should exist");
    if (f) {
        // Simple content check - verify JSON output mechanism works
        char buffer[65536]; // Snapshots make it larger
        size_t n = fread(buffer, 1, sizeof(buffer)-1, f);
        buffer[n] = 0;
        fclose(f);

        ASSERT_TRUE(strstr(buffer, "\"failure_reason\": \"Test Manual Dump\"") != NULL, "Reason match");
        ASSERT_TRUE(strstr(buffer, "\"label\": \"SNAPSHOT_1\"") != NULL, "Snapshot 1 found");
        ASSERT_TRUE(strstr(buffer, "\"label\": \"SNAPSHOT_2\"") != NULL, "Snapshot 2 found");
        ASSERT_TRUE(strstr(buffer, "\"currentState\"") != NULL, "Current state found");
        ASSERT_TRUE(strstr(buffer, "\"history\"") != NULL, "History array found");
    }

    // Cleanup
    remove(dumpPath);
    return TEST_PASSED;
}
