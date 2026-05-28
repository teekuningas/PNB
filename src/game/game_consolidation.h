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
// No StateInfo* — consolidation receives only what it needs:
//   - match: mutable game session (physical state, flow control, action state)
//   - field_positions: read-only field geometry
//   - team_data: read-only team roster (for reset recipes)
//   - game_conclusion: output for game-over summary
//   - referee/bps/his/scoreboard: const referee-owned state
void consolidation_update(
    MatchSession* match, const FieldPositions* field_positions, const TeamData* team_data,
    GameConclusion* game_conclusion, const RefereeState* referee, const BetweenPitchState* bps,
    const HalfInningState* his, const Scoreboard* scoreboard, MenuInfo* menuInfo, unsigned int* rng_seed,
    ConsolidationOutput* output
);

#endif
