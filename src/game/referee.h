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
 * @param game The physical world, scanned for player positions — read, never moved
 * @param scoreboard Which mode is being played (the home-run contest starts differently)
 * @param referee Mutable pointer to the referee state to initialize
 */
void initialize_referee(const MatchSession* game, const Scoreboard* scoreboard, RefereeState* referee);

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
void referee_finalize(const MatchSession* game, RefereeState* refereeState, BetweenPitchState* betweenPitchState);

/**
 * @brief Updates the legal world from the physical one, once per tick.
 *
 * It sequentially checks rules and updates the state immediately. The physical world comes in as
 * `const MatchSession*` and `const FieldPositions*`, named, so "the referee may read the physical world
 * but never move it" is the compiler's to hold — a `const StateInfo*` would leave its pointer members
 * writable, which is how the draw code once cleared legal state the referee then read back.
 *
 * @param refereeState, halfInningState, betweenPitchState, scoreboard, homeRunContestState  the legal
 *        world, which the referee alone writes.
 * @param announcement An out-parameter of the tick: what the referee pronounced this frame, for the screen.
 */
void update_referee(
    const MatchSession* game, const FieldPositions* fieldPositions, RefereeState* refereeState,
    HalfInningState* halfInningState, BetweenPitchState* betweenPitchState, Scoreboard* scoreboard,
    HomeRunContestState* homeRunContestState, GameEventType* announcement
);

#endif // REFEREE_H
