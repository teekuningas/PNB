#ifndef CATCHING_AI_H
#define CATCHING_AI_H

#include "globals.h"

void update_catching_ai(
    MatchSession* match, const GameRulesState* rules, const FieldPositions* fieldPositions,
    AIControllerState* aiController, IntentChannel* channel
);
void throw_ball_to_base(MatchSession* match, BaseID base, IntentChannel* channel);

#endif
