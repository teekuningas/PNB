#ifndef GAME_CONSOLIDATION_H
#define GAME_CONSOLIDATION_H

#include "field.h"
#include "tick_outputs.h"
#include "team_data.h"
#include "rules_state.h"
#include "world.h"

// Output from consolidation — signals app-level transitions to the caller, a tick output.
// Consolidation decides "the game should go to the menu, and why" but executes neither the screen change
// nor the menu's routing itself: the session does, from these.
typedef struct {
    int request_screen_change; // 1 if consolidation wants to leave game screen
    ScreenState target_screen; // Which screen to go to (e.g., SCREEN_MAIN_MENU)
    PeriodTransitionType period_transition; // why: which menu the session should open, while a change is requested
} ConsolidationOutput;

// Initializes flow control counters and state

// Main update function: Reacts to Referee decisions, manages flow, and enforces physical state.
// The legal world is passed const, and the compiler holds it: the referee is its only runtime writer, and
// consolidation enforces what it decided without writing any of it — the reset recipes included.
void consolidation_update(
    MatchSession* match, const FieldPositions* field_positions, const TeamData* team_data,
    GameConclusion* game_conclusion, const GameRulesState* rules, ConsolidationOutput* output
);

#endif
