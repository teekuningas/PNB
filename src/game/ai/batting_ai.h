#ifndef BATTING_AI_H
#define BATTING_AI_H

#include "globals.h"

void update_batting_ai(
    const MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions,
    AIControllerState* aiController, IntentChannel* channel
);

#endif
