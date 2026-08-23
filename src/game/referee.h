#ifndef REFEREE_H
#define REFEREE_H

#include "globals.h"

/**
 * @brief Initialize referee by scanning the physical world.
 *
 * Called during game setup (from menu, return to game) to establish
 * initial legal tracking by inspecting the physical state of players.
 *
 * This is NOT used for runtime transitions (end of inning, next pair)
 * which are handled via state machines within update_referee().
 *
 * @param stateInfo Full game state to scan for player positions
 * @param referee Mutable pointer to the referee state to initialize
 */
void initialize_referee(const StateInfo* stateInfo, RefereeState* referee);

/**
 * @brief Updates the referee state and game state based on the current world state.
 *
 * This function replaces the Analyze/Apply pattern. It sequentially checks rules
 * and updates the state immediately.
 *
 * @param stateInfo Read-only access to the full game state.
 * @param refereeState Mutable pointer to the referee state.
 * @param halfInningState Mutable pointer to the game state (for outs/runs).
 * @param betweenPitchState Mutable pointer to between-pitch sticky flags.
 * @param playerCounters Mutable pointer to player counters.
 * @param scoreboard Mutable pointer to global game info (teams/runs).
 */
void referee_reset_for_new_inning(RefereeState* ref, HalfInningState* his, BetweenPitchState* bps);

/**
 * @brief Post-consolidation referee finalization.
 *
 * Handles RESETTING→NONE transitions for all three state machines AFTER
 * consolidation has performed physical resets. Scans the newly-reset physical
 * world to establish legal tracking for the next cycle.
 *
 * This runs as a separate pipeline stage after consolidation, making the
 * ownership boundary physically impossible to violate by code placement.
 */
void referee_finalize(const StateInfo* stateInfo, RefereeState* refereeState, BetweenPitchState* betweenPitchState);

void update_referee(
    const StateInfo* stateInfo, RefereeState* refereeState, HalfInningState* halfInningState,
    BetweenPitchState* betweenPitchState, PlayerCounters* playerCounters, Scoreboard* scoreboard,
    HomeRunContestState* homeRunContestState
);

#endif // REFEREE_H
