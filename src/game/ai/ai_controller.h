#ifndef AI_CONTROLLER_H
#define AI_CONTROLLER_H

#include "globals.h"

// The AI half of the CONTROL stage (pipeline stage 1, at the frame top — game_frame.c), the mirror of
// action_invocations. Reads the settled World const, writes its own private memory and each AI team's
// intent channel, and nothing else: the signature is the whole of what a controller may touch.
void ai_update(
    const MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions,
    AIControllerState* aiController, IntentChannels* channels
);

// A fresh controller for a new match, drawing from its own random stream.
void init_ai_controller(AIControllerState* aiController, unsigned int rngSeed);

#endif /* AI_CONTROLLER_H */
