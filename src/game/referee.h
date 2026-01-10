#ifndef REFEREE_H
#define REFEREE_H

#include "globals.h"

/**
 * @brief Initializes the RefereeState structure.
 * Sets all bases to BASE_NONE (-1) instead of 0.
 */
void initializeRefereeState(RefereeState* referee);

/**
 * @brief Query functions for wounding system (Milestone 15 consolidation)
 *
 * These functions provide read-only access to the wounding state,
 * following the same pattern as get_base_controller().
 */

/**
 * @brief Check if a wounding catch is currently pending
 */
int is_wounding_catch_pending(const RefereeState* ref);

/**
 * @brief Check if the wounding catch has been handled (started processing)
 */
int is_wounding_catch_handled(const RefereeState* ref);

/**
 * @brief Get the current wounding catch timer value (-1 if inactive)
 */
int get_wounding_timer(const RefereeState* ref);

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
 * @param gameState Mutable pointer to the game state (for outs/runs).
 * @param gameModeState Mutable pointer to game mode state.
 * @param gameControl Mutable pointer to game control flags.
 * @param playerCounters Mutable pointer to player counters.
 * @param globalGameInfo Mutable pointer to global game info (teams/runs).
 */
void Referee_Update(const StateInfo* stateInfo, RefereeState* refereeState, GameState* gameState, GameModeState* gameModeState, GameControl* gameControl, PlayerCounters* playerCounters, GlobalGameInfo* globalGameInfo);

#endif // REFEREE_H
