#ifndef BATTING_AI_H
#define BATTING_AI_H

#include "globals.h"

void init_batting_ai(AIState* aiState);
void update_batting_ai(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions,
    AIControllerState* aiController, IntentChannel* channel
);

#endif
