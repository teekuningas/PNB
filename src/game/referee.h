#ifndef REFEREE_H
#define REFEREE_H

#include "globals.h"

/**
 * @brief Initializes the RefereeState structure.
 * Sets all bases to BASE_NONE (-1) instead of 0.
 */
void initializeRefereeState(RefereeState* referee);

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
 * @brief Query functions for wounding system (Milestone 15 consolidation)
 *
 * These functions provide read-only access to the wounding state,
 * following the same pattern as get_base_controller().
 */

/**
 * @brief Check if wounding evaluation is currently active
 */
int is_wounding_evaluation_active(const RefereeState* ref);

/**
 * @brief Get the current wounding evaluation timer value (-1 if inactive)
 */
int get_wounding_evaluation_timer(const RefereeState* ref);

/**
 * @brief Check if a specific player is marked for wounding
 * @param ref The referee state
 * @param playerIndex The player index to check
 * @return 1 if marked for wounding, 0 otherwise
 */
int is_player_marked_for_wound(const RefereeState* ref, int playerIndex);

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
void Referee_ResetForNewInning(RefereeState* ref, HalfInningState* his, BetweenPitchState* bps);

void update_referee(
    const StateInfo* stateInfo, RefereeState* refereeState, HalfInningState* halfInningState,
    BetweenPitchState* betweenPitchState, PlayerCounters* playerCounters, Scoreboard* scoreboard,
    HomeRunContestState* homeRunContestState
);

#endif // REFEREE_H
