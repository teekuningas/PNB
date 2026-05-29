#ifndef GAME_CONSOLIDATION_H
#define GAME_CONSOLIDATION_H

#include "globals.h"
#include "menu_types.h"

// Output from consolidation — signals app-level transitions to the caller.
// Consolidation decides "game should go to menu" but doesn't execute the screen change itself.
typedef struct {
    int request_screen_change; // 1 if consolidation wants to leave game screen
    ScreenState target_screen; // Which screen to go to (e.g., SCREEN_MAIN_MENU)
} ConsolidationOutput;

// Initializes flow control counters and state
void consolidation_init(GameFlowState* gameFlowState);

// Main update function: Reacts to Referee decisions, manages flow, and enforces physical state.
// Referee-owned state is passed as const — consolidation READS legal state but never WRITES to it.
// The GameRulesState* is passed for reset recipes that need to read/reset contest state.
void consolidation_update(
    MatchSession* match, const FieldPositions* field_positions, const TeamData* team_data,
    GameConclusion* game_conclusion, GameRulesState* rules, MenuInfo* menuInfo, unsigned int* rng_seed,
    ConsolidationOutput* output
);

#endif
