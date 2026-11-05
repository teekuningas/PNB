#ifndef ACTUALIZE_H
#define ACTUALIZE_H

#include "ingest.h"

// ACTUALIZE — stage 3 of the tick. The engine's own fixed order of actualization, reading the commands
// INGEST left: the throw's windup, the controlled fielder's walk, control changes, drops, the pitch clock,
// seating a batter, the free-walk answer, base running, the swing. The order is the engine's, never the
// order messages arrived in, so which producer spoke first cannot change the simulation.
void actualize(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions,
    const IngestedCommands* commands, int* playSoundEffect
);

void init_actualization(MatchSession* match);

#endif /* ACTUALIZE_H */
