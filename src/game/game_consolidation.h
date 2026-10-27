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

// Main update function: Reacts to Referee decisions, manages flow, and enforces physical state.
// The legal world is passed const, and the compiler holds it: the referee is its only runtime writer, and
// consolidation enforces what it decided without writing any of it — the reset recipes included.
void consolidation_update(
    MatchSession* match, const FieldPositions* field_positions, const TeamData* team_data,
    GameConclusion* game_conclusion, const GameRulesState* rules, MenuInfo* menuInfo, ConsolidationOutput* output
);

#endif
